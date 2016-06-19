
device = '/dev/ttyUSB1'

C_SET_COLOR = 0x13
C_SEND_BUF = 0x01
C_LCD = 0x02
C_BGLIGHT = 0x12
C_RELAY = 0x11

def send_cmd(cmd, params):
    with open(device, 'wb') as f:
        f.write(bytes([0x1b,cmd]))
        f.write(params)

def set_color(r,g,b):
    send_cmd(C_SET_COLOR, bytes([r,g,b]))

def send_buf(buf):
    send_cmd(C_SEND_BUF, bytes(buf))

def bglight(state=True):
    send_cmd(C_BGLIGHT, b'1' if state else b'0')

def lcd_str(str):
    str = str + (" "*32)
    str = str[0:32]
    send_cmd(C_LCD, bytes(str, 'ascii'))

def set_relay(timer=5):
    send_cmd(C_RELAY, bytes([timer]))

