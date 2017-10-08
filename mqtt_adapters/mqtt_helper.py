#!/usr/bin/env python
# -*- coding: utf-8 -*-
#source: https://raw.githubusercontent.com/victronenergy/dbus-mqtt/master/mqtt_gobject_bridge.py
#mit license

import errno
import logging
import os
import paho.mqtt.client
import socket
import ssl
import sys
import traceback

class MqttHelper(object):
	def __init__(self, mqtt_server=None, mqtt_port=None, client_id="", ca_cert=None, user=None, passwd=None):
		self._ca_cert = ca_cert
		self._mqtt_user = user
		self._mqtt_passwd = passwd
		self._mqtt_server = mqtt_server or '127.0.0.1'
		self._mqtt_port = mqtt_port
		self._client = paho.mqtt.client.Client(client_id)
		self._client.will_set(self.lwt_topic, payload="false", qos=0, retain=True)
		self._client.on_connect = self._on_connect
		self._client.on_message = self._on_message
		self._client.on_disconnect = self._on_disconnect
		self._socket_watch = None
		self._socket_timer = None
		self._init_mqtt()

	def _init_mqtt(self):
		logging.info('[Init] Connecting to local broker')
		if self._mqtt_user is not None and self._mqtt_passwd is not None:
			self._client.username_pw_set(self._mqtt_user, self._mqtt_passwd)
		if self._ca_cert is None:
			self._client.connect(self._mqtt_server, self._mqtt_port or 1883, 60)
		else:
			self._client.tls_set(self._ca_cert, cert_reqs=ssl.CERT_REQUIRED)
			self._client.connect(self._mqtt_server, self._mqtt_port or 8883, 60)
		self._client.loop_start()

	def _on_connect(self, client, userdata, dict, rc):
		pass

	def _on_message(self, client, userdata, msg):
		pass

	def _on_disconnect(self, client, userdata, rc):
		logging.error('[Disconnected] Lost connection to broker')
		if self._socket_watch is not None:
			gobject.source_remove(self._socket_watch)
			self._socket_watch = None
		logging.info('[Disconnected] Set timer')
		reconnect_timer = Timer(5.0, self._reconnect, args=(self,))

	def _reconnect(self):
		try:
			logging.info('[Reconnect] start')
			self._client.reconnect()
			logging.info('[Reconnect] success')
			return False
		except socket.error as e:
			logging.error('[Reconnect] failed' + traceback.format_exc())
			if e.errno == errno.ECONNREFUSED:
				return True
			raise
