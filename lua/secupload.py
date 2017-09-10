#!/usr/bin/env python
import argparse
import sys,time,os,base64,re
import socket,hmac

"""
struct header {
    char magic[2]; //'S','U'
    unsigned char filename_len;
    unsigned char flags;
    unsigned int filesize; // big endian
    char [32] sha256_hmac;
}
//following <filename_len> bytes of filename
//following <filesize> bytes of contents
"""




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
    parser.add_argument('files', metavar='FILENAME', type=str, nargs='*',
                        help='file to upload')
                    
    args = parser.parse_args()
    flags = 0
    if args.remove_after_do: flags |= 0x80
    if args.dofile: flags |= 0x01
    if args.restart: flags |= 0x40
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
            
    if args.remove != None:
        run_upload(None, args.remove, args.target, 0x80)

    for file in args.files:
        run_upload(file, file, args.target, flags, auth_key)
    


def read_in_chunks(file_object, chunk_size=1024):
    """Lazy function (generator) to read a file piece by piece.
    Default chunk size: 1k."""
    while True:
        data = file_object.read(chunk_size)
        if not data:
            break
        yield data

def run_upload(localfn, destfn, target, flags, auth_key):
    if localfn:
        fsize=os.path.getsize(localfn)
    else:
        fsize=0
    print("Uploading "+destfn+ " ("+str(fsize)+" bytes)")

    TCP_IP = target
    TCP_PORT = 99
    BUFFER_SIZE = 1024

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((TCP_IP, TCP_PORT))
    data = s.recv(BUFFER_SIZE)
    print(">"+data.decode("ascii"),end="")
    if data[0:9] != b"SU_NONCE ":
        print("Invalid header received")
        sys.exit(1)
    nonce = data[9:-1]
    
    header = b"SU" + bytes([len(destfn),flags,0,0,fsize>>8,fsize & 0xff])

    hasher = hmac.new(auth_key, digestmod="sha256")
    hasher.update(nonce)
    hasher.update(header)
    hasher.update(destfn.encode("ascii"))

    if localfn:
        with open(localfn,"rb") as f:
            for piece in read_in_chunks(f):
                hasher.update(piece)

    digest = hasher.digest()



    s.send(header + digest + destfn.encode("ascii") )

    data = s.recv(BUFFER_SIZE)
    print(">"+data.decode("ascii"),end="")
    if data != b"ACK\n":
        sys.exit(1)

    if localfn:
        with open(localfn,"rb") as f:
            for piece in read_in_chunks(f):
                s.send(piece)
                print(".",end=""); sys.stdout.flush()
                time.sleep(0.2)
    print("")

    data = s.recv(BUFFER_SIZE)
    print(">"+data.decode("ascii"))
    s.close()

if __name__ == '__main__':
    main()
