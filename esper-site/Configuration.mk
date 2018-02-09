## Sming configuration
ESP_HOME = /opt/esp-open-sdk
SMING_HOME = /home/mw/Projektarchive/Sming/Sming

include $(SITEDIR)/secrets.mk

## WiFi configuration to use
WIFI_SSID = z3:iot

## MQTT Server to use
MQTT_HOST = 10.83.42.11
MQTT_PORT = 1883
#MQTT_HOST = 82.195.85.50
#MQTT_PORT = 23456
MQTT_USERNAME = iot

MQTT_REALM = 

## Set to enable the HTTP server
HTTP_PORT = 80

## Remote UDP logging (redirects debug* calls from serial port to UDP)
REMOTE_UDP_LOG_IP = 224.0.0.69
REMOTE_UDP_LOG_PORT = 7890


CFLAGS += -O2

## If set, a message each 60 seconds is expected or a reboot is triggered
HEARTBEAT_TOPIC = $(MQTT_REALM)/$broadcast/heartbeat

## The URL prefix to fetch updates from (without trailing slash)
UPDATE_ENABLED = true
UPDATE_URL = http://$(MQTT_HOST):80/~mw/firmware

## If set, a message on this topic will trigger a update request
UPDATE_TOPIC = $(MQTT_REALM)/$broadcast/update

## If set, an update request is triggered at the given interval (in seconds)
UPDATE_INTERVAL = 86400

## ESP_HOME sets the path where ESP tools and SDK are located.
# ESP_HOME = /opt/esp-open-sdk

## SMING_HOME sets the path where Sming framework is located.
# SMING_HOME = /opt/sming/Sming

## COM port parameter is reqruied to flash firmware correctly.
# COM_PORT = /dev/tty.usbserial
COM_SPEED	= 76800
COM_SPEED_SERIAL = 76800
#COM_SPEED_ESPTOOL = 921600
COM_SPEED_ESPTOOL = 460800


SPI_MODE=dio

## Configure flash parameters (for ESP12-E and other new boards):
# SPI_MODE = dio
TERMINAL     = cbcom -d $(COM_PORT) -b $(COM_SPEED_SERIAL)

DEBUG_PRINT_FILENAME_AND_LINE = 1

PUBLISH_TARGET = /data/mw/Web/firmware
PUBLISH_TARGET = mw@10.83.42.11:/var/www/html/firmware

DEBUG_VERBOSE_LEVEL = 3
