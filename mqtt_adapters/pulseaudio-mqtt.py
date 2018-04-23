#!/usr/bin/python3
# -*- coding: utf-8 -*-

import paho.mqtt.client
from mqtt_gobject_bridge import MqttGObjectBridge
import time
import configparser
import socket
import dbus
import os
from dbus.mainloop.glib import DBusGMainLoop
import gobject
import logging

def pulse_bus_address():
    if 'PULSE_DBUS_SERVER' in os.environ:
        address = os.environ['PULSE_DBUS_SERVER']
    else:
        bus = dbus.SessionBus()
        server_lookup = bus.get_object("org.PulseAudio1", "/org/pulseaudio/server_lookup1")
        address = server_lookup.Get("org.PulseAudio.ServerLookup1", "Address", dbus_interface="org.freedesktop.DBus.Properties")
        print(address)

    return address

def sig_handler2(state, sender_path):
    vol_l=state[0]
    vol_r=state[1]
    print("Volume update for %s to %f/%f"%(sender_path,vol_l,vol_r))
    adapter.publish_volume(sender_path, vol_l)

def get_prop(proxy_obj, prop_iface, prop_name):
	return proxy_obj.Get(prop_iface, prop_name, dbus_interface='org.freedesktop.DBus.Properties')

def get_props(proxy_obj, prop_iface):
	return proxy_obj.GetAll(prop_iface, dbus_interface='org.freedesktop.DBus.Properties')

def print_dbus_dict(dic, indent=0):
	for k,v in dic.items():
		print("\t"*indent, k,"=", end="\t")
		if isinstance(v, dbus.Dictionary):
			print_dbus_dict(v, 1)
		elif isinstance(v, dbus.Array):
			if str(v.signature)=='y': # byte array
				for b in v:
					if b>=32 and b<=127:
						print("%c"%b, end="")
					else:
						print("\\x%02x"%b,end="")
				print("")
			else:
				print(v)
		else:
			print(v)

def pulse_set_volume(path, volume):
	v = dbus.Array([int(volume), int(volume)], signature='u')
	proxy_obj = pulse_bus.get_object(object_path=path)
	proxy_obj.Set('org.PulseAudio.Core1.Device', 'Volume', v, dbus_interface='org.freedesktop.DBus.Properties')

# setup the glib mainloop

DBusGMainLoop(set_as_default=True)

loop = gobject.MainLoop()

pulse_bus = dbus.connection.Connection(pulse_bus_address())
pulse_core = pulse_bus.get_object(object_path='/org/pulseaudio/core1')
pulse_core.ListenForSignal('org.PulseAudio.Core1.Device.VolumeUpdated', dbus.Array(signature='o'), dbus_interface='org.PulseAudio.Core1')

pulse_bus.add_signal_receiver(sig_handler2, 'VolumeUpdated', path_keyword='sender_path')

devices = {}
for sink_path in get_prop(pulse_core, "org.PulseAudio.Core1", "Sinks"):
	print(sink_path)
	sink = pulse_bus.get_object(object_path=sink_path)
	props = get_props(sink, "org.PulseAudio.Core1.Device")
	print_dbus_dict (props)
	try:
		description = bytes(props['PropertyList']['device.description'][:-1]).decode("ascii")
	except:
		description="<unnamed>"
	devices[sink_path] = {'basevol':int(props['BaseVolume']), 'name':str(props['Name']), 'display_name':description,'volume':int(props['Volume'][0])}

class MqttHomieAdapter(MqttGObjectBridge):
	def __init__(self, devices):
		CONFIG_FILE="mqtt_viewer.ini"
		config = configparser.ConfigParser()
		config.read(CONFIG_FILE)
		if not config.has_section('preferences'): config.add_section('preferences')
		if not config.has_section('mqtt'): config.add_section('mqtt')

		self.prefix=config.get('mqtt', 'prefix')
		self.device_id=config.get('mqtt', 'device_id', fallback=socket.gethostname())
		self.realm=self.prefix+"/"+self.device_id+"/"
		self.lwt_topic = self.realm + "$online"
		
		self.devices = devices
		
		logging.info("Connecting to MQTT...")
		MqttGObjectBridge.__init__(self,
			mqtt_server = config.get('mqtt', 'host'), 
			mqtt_port = config.getint('mqtt', 'port', fallback=1883), 
			client_id = "pulseaudio-mqtt",
			user = config.get('mqtt', 'username', fallback=None), 
			passwd = config.get('mqtt', 'password', fallback=None))

	def _on_message(self, client, userdata, msg):
		global devices_modified
		payload = msg.payload.decode('utf-8')
		print(msg.topic)
		topic=msg.topic.split("/")
		if len(topic) == 5 and topic[0] == self.prefix and topic[1] == self.device_id and topic[4] == 'set':
			node, property = topic[2], topic[3]
			print(node,property)
			if property == "volume":
				for path,dev in self.devices.items():
					if dev['name'] == node:
						pulse_set_volume(path, float(payload)*dev['basevol'])

	def _on_connect(self, client, userdata, dict, rc):
		print("Connected with result code " + str(rc))

		client.publish(self.lwt_topic, 'true', retain=True)
		
		client.subscribe(self.realm+"#")
		for dev_path, dev in self.devices.items():
			print(dev_path)
			client.publish(self.realm+dev['name']+'/volume/$settable', 'true', retain=True)
			client.publish(self.realm+dev['name']+'/volume/$name', dev['display_name'], retain=True)
			client.publish(self.realm+dev['name']+'/volume/$datatype', 'float', retain=True)
			client.publish(self.realm+dev['name']+'/volume/$format', '0:1.5', retain=True)
			self.publish_volume(dev_path, dev['volume'])

	def publish_volume(self, dev_path, vol):
		dev=self.devices[dev_path]
		vol = vol / dev['basevol']
		self._client.publish(self.realm+dev['name']+'/volume', str(vol), retain=True)

adapter = MqttHomieAdapter(devices)

loop.run()


