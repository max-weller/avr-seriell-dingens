#!/usr/bin/env python3
import sys,os
import time
import readline
import serial
from threading import Thread, Timer
import argparse
from datetime import datetime
import binascii
import socket
import paho.mqtt.client as mqtt

C_SET_OUTPUT                    = 0x05
C_GET_TEMPERATURE               = 0x40
C_SET_DISPLAY                   = 0x41
C_SET_MENU                      = 0x42
C_SET_MENU_ITEM                 = 0x43
C_BUZZER                        = 0x49
C_BMP_FILL_CONST                = 0x50
C_BMP_WRITE_RANGE               = 0x51
C_BMP_RANGE_FILL_CONST          = 0x52
C_BMP_RANGE_ADD_CONST           = 0x53
C_BMP_SCROLL_LEFT               = 0x54
C_BMP_SCROLL_RIGHT              = 0x55
C_BMP_TO_STRIPE_SINGLE          = 0x5e
C_BMP_TO_STRIPE_SET_INTERVAL    = 0x5f
C_PING                          = 0xf1
C_SET_BAUD_RATE                 = 0xfc
C_REBOOT_TO_BOOTLOADER          = 0xbf


class Parser:
    def __init__(self):
        self.reset()

    def reset(self):
        self.recv_idx=-4
        self.recv_escape=False
        self.recv_pkg=bytearray()
        self.length=0
        self.flags =0
        self.from_addr=0
        self.to_addr=0
        self.cksum = 0x2A

    def parse(self, byte):
        if byte == 0xfc:
            self.reset()
            self.recv_idx=-3
        elif byte == 0xfb:
            self.recv_escape = True
        else:
            if self.recv_escape:
                byte += 0x10
                self.recv_escape = False
            self.cksum ^= byte
            if self.recv_idx == -3: #waiting for rcpt addr
                self.to_addr = byte
                self.recv_idx += 1
            elif self.recv_idx == -2:
                self.from_addr = byte
                self.recv_idx += 1
            elif self.recv_idx == -1:
                self.length = byte & 0b00011111
                self.flags = byte & 0b11100000
                self.recv_idx += 1
            elif self.recv_idx >= 0:
                if self.recv_idx == self.length:
                    # done, check the checksum
                    if self.cksum == 0:
                        #print("Checksum valid")
                        self.cksum ^= byte
                        return self.recv_pkg
                    else:
                        self.cksum ^= byte
                        print("Checksum invalid received=%02x, correct=%02x" % (byte, self.cksum))
                    self.recv_idx = -4
                else:
                    try:
                        self.recv_pkg.append(byte)
                    except Exception as ex:
                        print("Error appending byte ",byte)
                        print(ex)
                    self.recv_idx += 1
        return False

    def printpacket(self):
          flags = []
          if self.flags & 0b10000000: flags.append("??????")
          if self.flags & 0b01000000: flags.append("ACK")
          if self.flags & 0b00100000: flags.append("ERROR")

          print("[%s] Received packet, from=%02x to=%02x length=%02x flags=%s command=%02X cksum=%02x" % (
              datetime.now().strftime("%H:%M:%S"),
              self.from_addr, self.to_addr, self.length, ",".join(flags), self.recv_pkg[0], self.cksum
          ))
          print(binascii.hexlify(self.recv_pkg[1:]).decode("ascii"))

    def build(fromadr, toadr, payload):
       # bytes([0xfc
       return []




def udplistenerthread():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", 40485))
    while True:
        data, addr = sock.recvfrom(1024)
        try:
            parts = data.decode("ascii").split("\n")
            for pp in parts:
                pp=pp.strip()
                if len(pp)==0: continue
                print("UDP from ",addr, " : ",pp)
                b=bytearray.fromhex(pp)
                #print("UDP from ",addr, " : ",hexlify_block(b,2))
                p=Parser()
                for c in b:
                    if p.parse(c) != False:
                        p.printpacket()

        except Exception as err:
            print(err)
            sock.sendto(str(err).encode("utf-8"), addr)

udplistenerthread()

