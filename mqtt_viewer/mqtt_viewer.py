#!/usr/bin/python3
# -*- coding: utf-8 -*-

import tkinter as tk
from tkinter import ttk, tix, simpledialog
import json,re,socket,struct,base64
import paho.mqtt.client
import time
import os,configparser
import tlv_parser as tlv
from threading import Thread

CONFIG_FILE=os.path.join(os.path.expanduser("~"), '.config/mqtt_viewer.ini')
config = configparser.ConfigParser()
config.read(CONFIG_FILE)
if not config.has_section('preferences'): config.add_section('preferences')
if not config.has_section('mqtt'): config.add_section('mqtt')
if not config.has_section('hsc'): config.add_section('hsc')

mqttprefix=config.get('mqtt', 'prefix')

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))

    client.subscribe(mqttprefix+"/#")

hsckey = base64.b64decode(config.get('hsc', 'key'))
devices={}
devices_modified=False


def hsc_discovery_request():
    hsc_sock.sendto(tlv.hmactlv(tlv.gettlv([ [tlv.T_DISCO_REQUEST,b""] ]), hsckey), ("255.255.255.255",UDP_PORT))
    hsc_sock.sendto(tlv.hmactlv(tlv.gettlv([ [tlv.T_DISCO_REQUEST,b""] ]), hsckey), (MCAST_GROUP,UDP_PORT))

def hsc_parse_discovery_response(tdec, sourceip):
    propname = None
    for tag,value in tdec:
        print("%04x = %s"%(tag,value.decode("utf-8")))
        if tag == tlv.T_PROP_NAME:
            propname = (mqttprefix + value.decode("utf-8")).split("/")
        elif propname == None:
            print("ignoring tag, propname is required")
        elif tag == tlv.T_PROP_VALUE:
            handleMessage(propname, value.decode("utf-8"))
        elif tag == tlv.T_PROP_FORMAT:
            handleMessage(propname + ["$format"], value.decode("utf-8"))
        elif tag == tlv.T_PROP_UNIT:
            handleMessage(propname + ["$unit"], value.decode("utf-8"))
        elif tag == tlv.T_PROP_DISPLAYNAME:
            handleMessage(propname + ["$name"], value.decode("utf-8"))
        elif tag == tlv.T_PROP_FLAGS:
            dtype = value[1]
            dflags = value[0]
            print("flags",dtype,dflags)
            if (dflags & 2) == 2:
                handleMessage(propname + ["$settable"], "true")
                handleMessage(propname[:-2] + ["$ip"], sourceip)
                handleMessage(propname + ["$datatype"], "command")
            else:
                handleMessage(propname + ["$datatype"], datatypes[dtype])
                if (dflags & 1) == 1:
                    handleMessage(propname + ["$settable"], "true")
                    handleMessage(propname[:-2] + ["$ip"], sourceip)


datatypes=["unknown","integer","float","boolean","string","enum"]
def hsc_receive_thread_fn(sock):
    while True:
        data, addr = sock.recvfrom(1024)
        tdata = tlv.parsetlv(data)
        tdec = tlv.decodetlv(tdata, [hsckey])
        if not tdec:
            print("decode_failed",tdata)
            
        print("received packet")
        tlv.dumptlv(tdec)
        if tlv.hastag(tdec, tlv.T_DISCO_RESPONSE):
            hsc_parse_discovery_response(tdec, addr[0])

UDP_PORT = config.get('hsc', 'udpport', fallback=19691)
MCAST_GROUP = config.get('hsc', 'multicastgroup', fallback="224.0.0.91")
MCAST_IFACE = config.get('hsc', 'interface', fallback="br0")
addrinfo = socket.getaddrinfo(MCAST_GROUP, None)[0]
hsc_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
hsc_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
hsc_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
hsc_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
hsc_sock.bind(("0.0.0.0", UDP_PORT))
group_bin = socket.inet_pton(addrinfo[0], MCAST_GROUP)
mreq = struct.pack('4sL', group_bin, socket.if_nametoindex(MCAST_IFACE))
hsc_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

addrinfo = socket.getaddrinfo("0.0.0.0", None)[0]
bcast_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
bcast_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
bcast_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
bcast_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
bcast_sock.bind(("0.0.0.0", UDP_PORT))

receive_thread = Thread(target=hsc_receive_thread_fn, args=(hsc_sock,))
receive_thread.setDaemon(True)
receive_thread.start()



def on_message(client, userdata, msg):
    payload = msg.payload.decode('utf-8')
    topic = msg.topic.split("/")
    try:
        handleMessage(topic, payload)
    except Exception as ex:
        print("Error while handling message")
        print("Topic: ",msg.topic)
        print("Payload: ",msg.payload)
        print("Exception: ",ex)

def handleMessage(topic, payload):
    global devices_modified
    print(topic,payload)
    if len(topic)<3:
        print("ignoring short topic", topic, payload)
        return
    if topic[0] == mqttprefix:
        deviceId = topic[1]
        if not deviceId in devices:
            devices[deviceId]={'$deviceId': deviceId, 'attrs':{}, 'properties':{}}
            devices_modified=True
        device = devices[deviceId]

        if topic[2][0] == "$": #device attribute
            attrname = "/".join(topic[2:])
            print("updating device attr", deviceId, attrname, payload)
            device['attrs'][attrname] = payload
            devices_modified=True
        elif len(topic)==5 and topic[-1][0] == "$": # property attribute
            propname = topic[2] + "/" + topic[3]
            attrname = topic[4]
            if not propname in device['properties']:
                device['properties'][propname] = {'v': tk.StringVar()}
                devices_modified=True
            device['properties'][propname][attrname] = payload
        elif len(topic)==4: #property value
            propname = topic[2] + "/" + topic[3]
            if not propname in device['properties']:
                device['properties'][propname] = {'v': tk.StringVar()}
                devices_modified=True
            device['properties'][propname]['v'].set(payload)
            #if '__row' in device['properties'][propname]:
        else:
            print("ignoring msg", topic, payload)

def mqtt_send(device, prop_name, value):
    print("Publishing ",mqttprefix+'/'+device['$deviceId']+'/'+prop_name+'/set', value)
    client.publish(mqttprefix+'/'+device['$deviceId']+'/'+prop_name+'/set', value)

    tlvdata = [ [tlv.T_DISCO_SET,b""],
        [tlv.T_PROP_NAME, ('/'+device['$deviceId']+'/'+prop_name).encode("utf8")],
        [tlv.T_PROP_VALUE, value.encode("utf8")], ]
    tlv.dumptlv(tlvdata)
    hsc_sock.sendto(tlv.hmactlv(tlv.gettlv(tlvdata), hsckey), ("255.255.255.255",UDP_PORT))
    hsc_sock.sendto(tlv.hmactlv(tlv.gettlv(tlvdata), hsckey), (MCAST_GROUP,UDP_PORT))
    if '$ip' in device['attrs']:
        hsc_sock.sendto(tlv.hmactlv(tlv.gettlv(tlvdata), hsckey), (device['attrs']['$ip'],UDP_PORT))



client = paho.mqtt.client.Client()
client.on_connect = on_connect
client.on_message = on_message

if config.has_option('mqtt', 'username'):
    client.username_pw_set(config.get('mqtt', 'username'), config.get('mqtt', 'password'))
client.connect(config.get('mqtt', 'host'), config.getint('mqtt', 'port', fallback=1883), config.getint('mqtt', 'keepalive', fallback=60))

client.loop_start()

class Application(tk.Frame):
    def __init__(self, master=None):
        super().__init__(master)
        self.winfo_toplevel().title("HAm MQTT Viewer/Commander")
        self.pack()
        self.devices_order = []
        if config.has_option('preferences', 'devices_order'):
            for deviceId in config.get('preferences', 'devices_order').split(','):
                if deviceId.startswith('-'):
                    deviceId = deviceId[1:]
                    devices[deviceId]={'$deviceId': deviceId, 'attrs':{}, 'properties':{}, '__collapsed': True}
                self.devices_order.append(deviceId)
        self.create_widgets()
        self.update_clock()

    def store_device_prefs(self):
        out = []
        for dev_id in self.devices_order:
            out.append( ("-" if devices[dev_id].get('__collapsed') else "") + dev_id )
        config["preferences"]["devices_order"] = ','.join(out)

    def collapse_device(self,device):
        device['__collapsed']=not device.get('__collapsed',False)
        self.update_widgets()
        self.store_device_prefs()

    def show_device_detail(self,device):
        print("foo")
        device['__showdetail']=not device.get('__showdetail',False)
        self.update_widgets()

    def change_device_name(self,device):
        attrs=', '.join([k+'='+v for k, v in device['attrs'].items()])
        old_name = device['attrs'].get('$name','(unnamed)')
        new_name = simpledialog.askstring("Device name", attrs, initialvalue=old_name)
        if new_name != None:
            client.publish(mqttprefix+'/'+device['$deviceId']+'/$name', new_name, retain=True)

    def mkdeviceheader(self,device):
        container = self
        c1 = tk.Button(container, command=lambda: self.collapse_device(device))
        c1.bind('<ButtonRelease-3>', lambda e: self.show_device_detail(device))

        c2 = tk.Button(container, text=device['attrs'].get('$name','(unnamed)'))
        c2.bind('<ButtonRelease-3>', lambda e: self.change_device_name(device))

        c3 = tk.Label(container, text="fwInfo")

        detailsbox = tk.Text(container, width=70, height=3)
        device['__header'] = [c1,c2,c3,detailsbox]

    def updatedeviceheader(self,device,row):
        c1,c2,c3,detailsbox = device['__header']
        c1.configure(bg='red' if device['attrs'].get('$online','false')!='true' else 'lightgreen',
                text=('▶' if device.get('__collapsed') else '▼')+' '+device['$deviceId'])
        c2.configure( text=device['attrs'].get('$name','(unnamed)'))

        fwInfo = device['attrs'].get('$fw/name','') + ' ' + device['attrs'].get('$fw/version','')
        c3.configure( text=fwInfo)

        attrs=', '.join([k+'='+v for k, v in device['attrs'].items()])
        detailsbox.delete(1.0, tk.END); detailsbox.insert(tk.END, attrs)
        
        self.rowformat(device, row, device['__header'], False)
        if device.get('__showdetail'):
            detailsbox.grid(row=row+1,column=0,columnspan=4)
        else:
            detailsbox.grid_remove()
        return row+2

    def rowformat(self, device, row, cols, collapse):
        for i,c in enumerate(cols):
            if collapse:
                c.grid_remove()
            else:
                c.grid(row=row, column=i, sticky="W")

    def mkpropertyrow(self,device,prop,propname):
        container = self
        def show_popup(event):
            self.cur_menu_property = (device,prop,propname)
            try:
               self.property_menu.tk_popup(event.x_root, event.y_root, 0)
            finally:
                pass
               #self.property_menu.grab_release()
            
        c1 = tk.Label(container, text=propname, width=14, anchor=tk.W)
        c1.bind('<ButtonRelease-3>', show_popup)
        c2 = tk.Label(container, text=prop.get('$name',''), width=20, anchor=tk.W)
        c2.bind('<ButtonRelease-3>', show_popup)
        prop['__fieldtype'] = self.getfieldtype(prop)
        def on_change(event=None):
            if '__onchange_timer' in prop:
                root.after_cancel(prop['__onchange_timer'])
            def on_change_debounced():
                mqtt_send(device, propname, prop['v'].get())
            prop['__onchange_timer'] = root.after(5, on_change_debounced)
            
        if prop['__fieldtype'] == 'Button':
            c3 = tk.Button(container, textvariable=prop.get('$name','Go!'), command=on_change)
        if prop['__fieldtype'] == 'Label':
            c3 = tk.Label(container, textvariable=prop['v'])
        elif prop['__fieldtype'] == 'Combobox':
            c3 = ttk.Combobox(container, textvariable=prop['v'])
        elif prop['__fieldtype'] == 'Scale':
            c3 = tk.Scale(container, orient=tk.HORIZONTAL, length=200, 
                variable=prop['v'], command=on_change, showvalue=False)
            #c3.bind('<ButtonRelease-1>', on_change)
        elif prop['__fieldtype'] == 'Progressbar':
            c3 = ttk.Progressbar(container, orient=tk.HORIZONTAL, length=200, maximum=1)
        elif prop['__fieldtype'] == 'Checkbutton':
            c3 = ttk.Checkbutton(container, text='Enable', command=on_change,
                    variable=prop['v'], onvalue='true', offvalue='false')
        else:
            c3 = tk.Entry(container, textvariable=prop['v'])
            c3.bind('<Return>', on_change)
        c4 = tk.Label(container, text=prop.get('$unit',''))
        prop['__row'] = [c1,c2,c3,c4]
    
    def updatepropertyrow(self,device,prop,row):
        c1,c2,c3,c4 = prop['__row']
        c2.configure(text=prop.get('$name',''))

        if prop['__fieldtype'] == 'Combobox':
            c3.configure(values=prop.get('$format','').split(','))
        elif prop['__fieldtype'] == 'Scale':
            from_, to=prop.get('$format','').split(':')
            srange = float(to) - float(from_)
            c3.configure(from_=from_, to=to, resolution=srange/255)#, bigIncrement=srange/25)
        elif prop['__fieldtype'] == 'Progressbar':
            from_, to=prop.get('$format','').split(':')
            val = (float(prop['v'].get()) - float(from_)) / (float(to) - float(from_))
            c3.configure(value=val)
        
        if hasattr(c3, 'state'):
            c3.state(['!disabled' if prop.get('$settable','')=='true' else 'disabled'])
        else:
            c3.config(state='normal' if prop.get('$settable','')=='true' else 'disabled')

        self.rowformat(device, row, prop['__row'], device.get('__collapsed'))
        return row+1


    def getfieldtype(self,prop):
        dt = prop.get('$datatype','')
        fmt = prop.get('$format','')
        edit = prop.get('$settable','')=='true'
        if dt == 'command':
            return 'Button'
        elif dt == 'enum':
            if edit:
                values = fmt.split(',')
                if len(values) > 4:
                    return 'Combobox'
                else:
                    return 'Radiobuttons'
            else:
                return 'Label'
        elif dt == 'integer' or dt == 'float':
            if ':' in fmt:
                if edit:
                    return 'Scale'
                else:
                    return 'Progressbar'
            else:
                return 'Entry'
        elif dt == 'boolean':
            return 'Checkbutton'
        else:
            if edit:
                return 'Entry'
            else:
                return 'Label'


    def update_widgets(self):
        for dev_id in devices.keys():
            if not dev_id in self.devices_order:
                self.devices_order.append(dev_id)
        
        row = 1
        for dev_id in self.devices_order:
            if not dev_id in devices: continue
            device = devices[dev_id]
            # skip offline devices with no properties
            if device['attrs'].get('$online','false') == 'false' and len(device['properties'])==0: continue

            if not '__header' in device: self.mkdeviceheader(device)
            row = self.updatedeviceheader(device, row)

            for propname, prop in device['properties'].items():
                if '__row' in prop and self.getfieldtype(prop) != prop['__fieldtype']:
                    for widget in prop['__row']: widget.destroy()
                    del prop['__row']
                if not '__row' in prop: self.mkpropertyrow(device, prop, propname)
                row = self.updatepropertyrow(device, prop, row)
        self.quit.grid(row=row,column=0)
        self.status.grid(row=row, column=1, columnspan=3)

    def property_menu_copy_display_name(self):
        print(self.cur_menu_property)
        device,prop,propname = self.cur_menu_property
        root.clipboard_clear()
        root.clipboard_append(prop['$name'])
    def property_menu_copy_value(self):
        device,prop,propname = self.cur_menu_property
        root.clipboard_clear()
        root.clipboard_append(prop['v'].get())
    def property_menu_copy_topic(self):
        device,prop,propname = self.cur_menu_property
        topic = mqttprefix+'/'+device['$deviceId']+'/'+propname
        root.clipboard_clear()
        root.clipboard_append(topic)
    def property_menu_delete(self):
        device,prop,propname = self.cur_menu_property
        topic = mqttprefix+'/'+device['$deviceId']+'/'+propname
        client.publish(topic, None, retain=True)
        client.publish(topic+'/$settable', None, retain=True)
        client.publish(topic+'/$datatype', None, retain=True)
        client.publish(topic+'/$format', None, retain=True)
        client.publish(topic+'/$name', None, retain=True)



    def create_widgets(self):
        #self.cframe = tix.ScrolledWindow(self, width=500, height=700)
        #self.cframe.pack()
        #self.refresh = tk.Button(self, text="REFRESH", fg="blue",
        #                      command=self.create_widgets)
        self.quit = tk.Button(self, text="QUIT", fg="red",
                              command=root.destroy)
        self.status = tk.Label(self, text="status", fg="green")

        self.property_menu = tk.Menu(self, tearoff=0)
        self.property_menu.add_command(label="Copy display name",
                                    command=self.property_menu_copy_display_name)
        self.property_menu.add_command(label="Copy value",
                                    command=self.property_menu_copy_value)
        self.property_menu.add_command(label="Copy full topic",
                                    command=self.property_menu_copy_topic)
        self.property_menu.add_command(label="Delete",
                                    command=self.property_menu_delete)
        #self.property_menu.bind("<FocusOut>", lambda e:rcmenu.unpost())

        
    def update_clock(self):
        global devices_modified
        now = time.strftime("%H:%M:%S")
        self.status.configure(text=now)
        if devices_modified:
            self.update_widgets()
            devices_modified=False
        root.after(1000, self.update_clock)

root = tk.Tk()
app = Application(master=root)
def save_size(event):
  config.set('preferences','geometry',root.geometry())
root.bind("<Configure>",save_size)

try:
    root.geometry(config['preferences']['geometry'])
except:
    pass

hsc_discovery_request()

app.mainloop()

with open(CONFIG_FILE, 'w') as configfile:
   config.write(configfile)
