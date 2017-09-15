#!/usr/bin/env python
import argparse
import sys,time,os,base64,re
import socket,hmac
import struct
from hashlib import sha256
"""
struct header {
    char magic[2]; //'s','c'
    char sha256_header_hmac[32]; //of {16byte nonce, header[34:], packet}
    unsigned short header_len;  // big endian; in HMAC
    unsigned char command;  // in HMAC
    unsigned char flags;    // in HMAC
}
struct file_upload_packet {  // in HMAC
    unsigned int filesize; // big endian
    char sha256_file_hash[32];
    unsigned char filename_len;
    //char filename[<filename_len>];
}
struct file_upload_contents { // not in HMAC
    //char data[<filesize>];
}
struct error_packet {
    unsigned char errmes_len;
    //char errmes[<errmes_len>];
}
struct nonce_packet {
    unsigned char errmes_len;
    //char errmes[<errmes_len>];
}
"""
CMD_ERROR_RESPONSE = 0x46 #('E')  error response

CMD_DISCOVERY_REQUEST=0x80  
CMD_DISCOVERY_RESPONSE=0x81
"""
    header_payload: struct discovery_response
"""
CMD_PROPERTY_PUBLISH=0x8a 
"""
    header_payload: struct discovery_response
"""

CMD_FILE_UPLOAD=0xe0
FILE_UPLOAD_FLAG_DOFILE = 0x01
FILE_UPLOAD_FLAG_COMPILE = 0x02
FILE_UPLOAD_FLAG_REMOVE = 0x80  #  (after compile or dofile)
"""
    header_payload: file_upload_packet
"""
CMD_FILE_DELETE=0xe1 # file delete
"""
    header_payload: file_upload_packet
        only filename(len) valid
"""
CMD_REBOOT_TARGET=0xeb  #reboot target
CMD_ACKNOWLEDGE=0xef  #acknowledge
"""
struct discovery_response  {  // in HMAC
    unsigned int num_properties;
    discovery_property properties[];
}
struct discovery_property {  // in HMAC
    unsigned char flags;
    unsigned char datatype;

    unsigned char prop_name_length;
    char prop_name[prop_name_length];

    unsigned char format_length;
    char format[format_length];

    unsigned char unit_length;
    char unit[unit_length];

    unsigned char display_name_length;
    char display_name[display_name_length];

    unsigned char cur_content_length;
    char cur_content[cur_content_length];
}

"""

struct_header = struct.Struct('!2s 32s H BB')
struct_file_upload_packet = struct.Struct('!I 32s B')


def main():
    parser = argparse.ArgumentParser(description='Upload file to nodemcu')
    parser.add_argument('-R', '--remove', metavar='FILE', type=str,
                        help='remove a file')
    parser.add_argument('--remove-after-do', action='store_true',
                        help='remove the file after executing it')
    parser.add_argument('-E', '--exec', dest='dofile', action='store_true',
                        help='execute the file after uploading it')
    parser.add_argument('-S', '--restart', action='store_true',
                        help='reboot controller after upload')
    parser.add_argument('-t', '--target',
                        help='ip or hostname of target device')
    parser.add_argument('-k', '--key',
                        help='authentication key base64 encoded')
    parser.add_argument('-C', '--compile',
                        help='compile file and delete source afterwards on nodemcu')
    parser.add_argument('files', metavar='FILENAME', type=str, nargs='*',
                        help='file to upload')
                    
    args = parser.parse_args()
    flags = 0
    if args.remove_after_do: flags |= 0x80
    if args.dofile: flags |= 0x01
    if args.restart: flags |= 0x40
    if args.compile: flags |= 0x82
    if args.key:
        auth_key = base64.b64decode(args.key)
    else:
        conf = open("sysconfig.lua","r").read()
        match = re.search(r'fromBase64\("([a-zA-Z0-9+/=]+)"\)', conf)
        if match:
            auth_key = base64.b64decode(match.group(1))
        else:
            print("Please specify authentication key")
            sys.exit(1)
            
    
    conn = Connection(args.target, auth_key)

    if args.remove != None:
        destfn = args.remove
        self.sendpacket(CMD_FILE_DELETE, 0, 
            struct_file_upload_packet.pack(0,b'\0'*32,len(destfn))+destfn.encode("ascii"))

    for file in args.files:
        conn.file_upload(file, file, flags, auth_key)

class Connection:
    def __init__(self, ip, auth_key):
        self.auth_key = auth_key
        self.buffer=bytes()
        TCP_PORT = 99
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((ip, TCP_PORT))

        _, command, _, nonce = self.nextframe(False)
        if command != 0x4e or len(nonce)!=16: raise 'no nonce received'

        self.nonce = nonce

    def nextframe(self, check_hmac=True):
        while True:
            if len(self.buffer) >= struct_header.size:
                magic, header_hmac, header_len, command, flags = struct_header.unpack(self.buffer)
                if magic != b'sc': raise "invalid_magic"
                if len(self.buffer) >= struct_header.size+header_len:
                    payload = self.buffer[struct_header.size:struct_header.size+header_len]
                    if check_hmac:
                        hasher = hmac.new(self.auth_key, digestmod="sha256")
                        hasher.update(nonce)
                        hasher.update(self.buffer[34:struct_header.size+header_len])
                        if hasher.digest() != header_hmac: raise "invalid_hmac"
                    self.buffer=self.buffer[struct_header.size+header_len:]
                    return header_hmac, command, flags, payload
            
            chunk = self.sock.recv(4096)
            if not chunk:
                raise "no_data"
            self.buffer.extend(chunk)

    def sendpacket(self, command, flags, payload):
        hashed = struct.pack('!H BB', len(payload), command, flags) + payload
        hmac = hmac.new(auth_key, digestmod="sha256")
        hmac.update(self.nonce)
        hmac.update(hashed)
        return b'su' + hmac.digest() + hashed

    def file_upload(self, localfn, destfn, flags):
        fsize=os.path.getsize(localfn)
        print("Uploading "+destfn+ " ("+str(fsize)+" bytes)")

        hasher = sha256()
        if localfn:
            with open(localfn,"rb") as f:
                for piece in read_in_chunks(f):
                    hasher.update(piece)

        filehash = hasher.digest()

        self.sendpacket(CMD_FILE_UPLOAD, flags, struct_file_upload_packet.pack(fsize,filehash,len(destfn))+destfn.encode("ascii"))
        _, go_cmd, _, go_payload = self.nextframe(False)
        
        if go_cmd != CMD_ACKNOWLEDGE or go_payload != b"go":
            raise "no go"

        with open(localfn,"rb") as f:
            for piece in read_in_chunks(f):
                s.send(piece)
                print(".",end=""); sys.stdout.flush()
                time.sleep(0.2)
        print("")

        _, ack_cmd, _, ack_payload = self.nextframe()
        
        if ack_cmd != CMD_ACKNOWLEDGE:
            raise "nack"
        
def read_in_chunks(file_object, chunk_size=1024):
    """Lazy function (generator) to read a file piece by piece.
    Default chunk size: 1k."""
    while True:
        data = file_object.read(chunk_size)
        if not data:
            break
        yield data

if __name__ == '__main__':
    main()
