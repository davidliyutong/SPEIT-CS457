import socket
import sys
import multiprocessing
import argparse

import time
import random

LOCAL_PORT = random.randint(10000, 16000)
REMOTE_PORT = 18888
BUFFLEN = 1024


class TransferAgent:
    def __init__(self, server_addr, server_port, local_port):
        self.server_addr = server_addr
        self.server_port = server_port
        self.local_port = local_port
        self.start()

    def start(self):
        self.sock_out = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM)
        self.sock_out.bind(('127.0.0.1', self.local_port))

    def stop(self):
        self.sock_out.close()

    def connect(self):
        self.sock_out.connect((self.server_addr, self.server_port))

    def get_resource(self, resource):
        if resource == "/$INDEX":
            raise NotImplementedError
        else:
            raise NotImplementedError


if __name__ == '__main__':
    print(LOCAL_PORT)
    print(REMOTE_PORT)
    sock_out = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM)
    sock_out.bind(('127.0.0.1', LOCAL_PORT))

    while True:
        try:
            sock_out.connect_ex(('127.0.0.1', REMOTE_PORT))
            string = "I" * 20 + "\n"
            sock_out.send(string.encode("ascii"))
            print("Success")
            print("Success")
        except ConnectionAbortedError as e:
            print(e)
        except ConnectionRefusedError as e:
            print(e)
        except BrokenPipeError as e:
            print(e)
        time.sleep(1)
