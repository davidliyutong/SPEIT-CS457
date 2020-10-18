import socket
from multiprocessing import Process, Queue
import argparse

PORT = 18888
BUFFLEN = 1024


def handle_client(id, sock_client, addr_client, msgQueue, kwargs, timeout=10, bufflen=4):
    sock_client.settimeout(timeout) # set the time out of sock
    index = kwargs['index']
    while True:
        try:
            msg = sock_client.recv(bufflen)
            print(id, msg)
            msg_decomposed = msg.split(b'\n')
            if msg_decomposed[0][0] == 'S':  # it's a GET message, client want start
                sock_client.send(index.bjson)
        except:
            break

    msgQueue.put(id)
    return


class ConnectionDaemon:
    def __init__(self):
        self.clients = dict()
        self.client_process_pair = []

        self.counter = 0
        self.used_counters = []

        self.msg_queue = Queue()

    def _release_on_signal(self):
        while not self.msg_queue.empty():
            process_id = self.msg_queue.get_nowait()
            self.clients[process_id].terminate()
            self.used_counters.append(process_id)
            print("Kill process: {}".format(str(process_id)))

    def add_connection(self, sock_client, addr_client, **kwargs):
        if sock_client in self.clients.keys():
            pass
        else:
            if len(self.used_counters) > 0:
                id = self.used_counters.pop()
            else:
                id = self.counter
                self.counter += 1
            p = Process(target=handle_client, args=(id, sock_client, addr_client, self.msg_queue, kwargs,))
            p.start()
            self.clients[id] = p

    def loop(self):
        self._release_on_signal()


def main(args):
    connections = ConnectionDaemon()
    root_path = args.path  # root path of the service

    sock_in = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM)  # create socket
    sock_in.bind(('127.0.0.1', PORT))  # bind address
    print("bind ", PORT)

    sock_in.listen(5)
    while True:
        connections.loop()
        sock_client, addr_client = sock_in.accept()
        connections.add_connection(sock_client, addr_client, index=index)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="A TCP server")
    parser.add_argument('-p', '--path', default='.')
    args = parser.parse_args()
    main(args)
