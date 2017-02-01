#!/usr/bin/env python3

from led import *

def rainbow():
  x=[]
  for i in range(0,50):
    x.append(i*5)
    x.append(100-i*2)
    x.append(0) #50-i)
  return x

def noesc(x):
  return [v if v != 0x1b else 0x1a for v in x]

a=rainbow()
a = noesc(a)
print(len(a))
a=a+a

import time

for ctr in range(1,500):
  i = ctr % 50
  b = a[3*i:3*i+90]
  print(len(b))
  send_buf(b)
  time.sleep(0.5)


