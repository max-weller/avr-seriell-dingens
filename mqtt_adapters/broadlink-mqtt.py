#!/usr/bin/python3
# -*- coding: utf-8 -*-

import paho.mqtt.client
from mqtt_helper import MqttHelper
import time, socket, binascii, os, configparser, logging
from threading import Thread
import broadlink

devices = broadlink.discover(timeout=5)
for device in devices:
    device.auth()
    device.mac_addr_str = binascii.hexlify(device.mac).decode('ascii')
    device.current_state = device.check_power()

def check_device_states():
	while True:
		for device in devices:
			state = device.check_power()
			print(device.mac_addr_str, device.current_state, state)
			if device.current_state != state:
				device.current_state = state
				adapter.publish_state_change(device)
		time.sleep(10)

Thread(target=check_device_states).start()

class MqttHomieAdapter(MqttHelper):
	def __init__(self, devices):
		CONFIG_FILE="mqtt_viewer.ini"
		config = configparser.ConfigParser()
		config.read(CONFIG_FILE)
		if not config.has_section('preferences'): config.add_section('preferences')
		if not config.has_section('mqtt'): config.add_section('mqtt')

		self.prefix=config.get('mqtt', 'prefix')
		self.device_id=config.get('mqtt', 'device_id', fallback='broadlink')
		self.realm=self.prefix+"/"+self.device_id+"/"
		self.lwt_topic = self.realm + "$online"
		
		self.devices = devices
		
		logging.info("Connecting to MQTT...")
		MqttHelper.__init__(self,
			mqtt_server = config.get('mqtt', 'host'), 
			mqtt_port = config.getint('mqtt', 'port', fallback=1883), 
			client_id = "broadlink-mqtt",
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
			if property == "on":
				for dev in self.devices:
					if dev.mac_addr_str == node:
						dev.set_power(payload == "1" or payload == "true")

	def _on_connect(self, client, userdata, dict, rc):
		print("Connected with result code " + str(rc))

		client.publish(self.lwt_topic, 'true', retain=True)
		
		client.subscribe(self.realm+"#")
		for dev in self.devices:
			client.publish(self.realm+dev.mac_addr_str+'/on/$settable', 'true', retain=True)
			#client.publish(self.realm+dev.mac_addr_str+'/on/$name', dev['display_name'], retain=True)
			client.publish(self.realm+dev.mac_addr_str+'/on/$datatype', 'boolean', retain=True)
			self.publish_state_change(dev)

	def publish_state_change(self, dev):
		self._client.publish(self.realm+dev.mac_addr_str+'/on', 
            "true" if dev.current_state else "false", retain=True)

adapter = MqttHomieAdapter(devices)

loop.run()


