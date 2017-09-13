#!/usr/bin/python3
# -*- coding: utf-8 -*-

import tkinter as tk
from tkinter import ttk, tix
import json,re
import paho.mqtt.client
import time
import configparser

CONFIG_FILE="mqtt_viewer.ini"
config = configparser.ConfigParser()
config.read(CONFIG_FILE)
if not config.has_section('preferences'): config.add_section('preferences')
if not config.has_section('mqtt'): config.add_section('mqtt')

mqttprefix=config.get('mqtt', 'prefix')

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))

    client.subscribe(mqttprefix+"/#")

devices={}
devices_modified=False

def on_message(client, userdata, msg):
    global devices_modified
    payload = msg.payload.decode('utf-8')
    
    topic=msg.topic.split("/")
    if len(topic)<3:
        print("ignoring short topic", msg.topic, payload)
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
            print("ignoring msg", msg.topic, payload)

def mqtt_send(device, prop_name, value):
    print("Publishing ",mqttprefix+'/'+device['$deviceId']+'/'+prop_name+'/set', value)
    client.publish(mqttprefix+'/'+device['$deviceId']+'/'+prop_name+'/set', value)


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

    def mkdeviceheader(self,device):
        container = self
        c1 = tk.Button(container, command=lambda: self.collapse_device(device))
        c1.bind('<ButtonRelease-3>', lambda e: self.show_device_detail(device))

        c2 = tk.Button(container, text=device['attrs'].get('$name','(unnamed)'))

        #c3 = tk.Label(container, text=attrs)
        #c3 = ttk.Combobox(container)
        c3 = tk.Text(container, width=70, height=3)
        device['__header'] = [c1,c2,c3]

    def updatedeviceheader(self,device,row):
        c1,c2,c3 = device['__header']
        c1.configure(bg='red' if device['attrs'].get('$online','false')!='true' else 'lightgreen',
                text=('▶' if device.get('__collapsed') else '▼')+' '+device['$deviceId'])
        c2.configure( text=device['attrs'].get('$name','(unnamed)'))
        attrs=', '.join([k+'='+v for k, v in device['attrs'].items()])
        c3.delete(1.0, tk.END); c3.insert(tk.END, attrs)
        #attrs=[k+'='+v for k, v in device['attrs'].items() if k!='$online' and k!='$name']
        #c3.configure(values=attrs)
        self.rowformat(device, row, device['__header'], False)
        if device.get('__showdetail'):
            c3.grid(row=row+1,column=0,columnspan=4)
        else:
            c3.grid_remove()
        return row+2

    def rowformat(self, device, row, cols, collapse):
        for i,c in enumerate(cols):
            if collapse:
                c.grid_remove()
            else:
                c.grid(row=row, column=i, sticky="W")

    def mkpropertyrow(self,device,prop,propname):
        container = self
        c1 = tk.Label(container, text=propname)
        c2 = tk.Label(container, text=prop.get('$name',''))
        prop['__fieldtype'] = self.getfieldtype(prop)
        def on_change(event=None):
            mqtt_send(device, propname, prop['v'].get())
        if prop['__fieldtype'] == 'Label':
            c3 = tk.Label(container, textvariable=prop['v'])
        elif prop['__fieldtype'] == 'Combobox':
            c3 = ttk.Combobox(container, textvariable=prop['v'])
        elif prop['__fieldtype'] == 'Scale':
            c3 = ttk.Scale(container, orient=tk.HORIZONTAL, length=200, variable=prop['v'])
            c3.bind('<ButtonRelease-1>', on_change)
        elif prop['__fieldtype'] == 'Progressbar':
            c3 = ttk.Progressbar(container, orient=tk.HORIZONTAL, length=200, maximum=1)
        elif prop['__fieldtype'] == 'Checkbutton':
            c3 = ttk.Checkbutton(container, text='Enable', variable=prop['v'], onvalue='true', offvalue='false', 
                    command=on_change)
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
            c3.configure(from_=from_, to=to)
        elif prop['__fieldtype'] == 'Progressbar':
            from_, to=prop.get('$format','').split(':')
            val = (float(prop['v'].get()) - float(from_)) / (float(to) - float(from_))
            c3.configure(value=value)
        
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
        if dt == 'enum':
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

        


    def create_widgets(self):
        #self.cframe = tix.ScrolledWindow(self, width=500, height=700)
        #self.cframe.pack()
        #self.refresh = tk.Button(self, text="REFRESH", fg="blue",
        #                      command=self.create_widgets)
        self.quit = tk.Button(self, text="QUIT", fg="red",
                              command=root.destroy)
        self.status = tk.Label(self, text="status", fg="green")
        
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

app.mainloop()

with open(CONFIG_FILE, 'w') as configfile:
   config.write(configfile)
