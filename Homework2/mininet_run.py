from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import OVSBridge
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNodeConnections, quietRun
from mininet.log import setLogLevel, info
from mininet.cli import CLI
import time
import os
from sys import argv

plt_ready = False

output_info = False

try:
    import matplotlib.pyplot as plt

    plt_ready = True
except ModuleNotFoundError:
    pass


class SingleSwitchTopo(Topo):
    def build(self, num_node=6, bw=10):
        # add switches
        s1 = self.addSwitch('s1', ip='10.10.0.0')

        # add hosts
        hosts = [self.addHost('h{}'.format(i), ip='10.10.0.{}'.format(i)) for i in range(1, num_node + 1)]

        # add links
        for i in range(num_node):
            self.addLink(hosts[i], s1, bw=bw, delay='1ms', loss=0, use_htb=True)


def test_cs_model(num_client=6, file_size=1, bw=10):
    """
    This function tests C/S model
    :param num_client:
    :param file_size:
    :param bw:
    :return:
    """
    print("Testing C/S model")
    topo = SingleSwitchTopo(num_client + 1, bw)
    net = Mininet(topo=topo,
                  host=CPULimitedHost, link=TCLink,
                  autoStaticArp=False)
    net.start()
    if output_info:
        info('* Dumping host connections\n')
        dumpNodeConnections(net.hosts)
    hosts = net.getNodeByName(*['h{}'.format(i) for i in range(1, num_client + 2)])
    print(net.iperf([hosts[0], hosts[1]]))
    hosts[0].cmd('./build/tcpServer ./Files/ > ./Dst/CS/Server.log&')
    for i in range(1, num_client + 1):
        hosts[i].cmd('./build/tcpClient 10.10.0.1 18889 ./File_{}M ./Dst/CS/Downloads/Client{}_File_{}M > ./Dst/CS/Client{}.log&'.format(file_size, i, file_size, i))

    sleep_time = 4 * 8 * file_size * num_client / bw
    print("Sleep {} seconds".format(sleep_time))
    time.sleep(sleep_time)  # Wait until experiment finished
    net.stop()

    # read logs, get executiontime
    tot_time = 0
    try:
        for i in range(1, num_client + 1):
            with open('./Dst/CS/Client{}.log'.format(i), 'r') as fp:
                lines = fp.readlines()
                last_line = lines[-1]
                curr_time = float(last_line)
                tot_time += curr_time
    except IndexError:
        tot_time += 10000
    average_time = tot_time / num_client
    average_speed = 8 * file_size / average_time
    print("C/S Average download time is: {}".format(average_time))
    print("C/S Average download speed is: {:.3f}Mbps/s".format(8 * file_size / average_time))
    return average_time, average_speed


def test_p2p_model(num_client=6, file_size=1, bw=1, time_delay=0):
    """
    This function tests P2P model
    :param num_client:
    :param file_size:
    :param bw:
    :param time_delay:
    :return:
    """
    print("Testing P2P model")
    topo = SingleSwitchTopo(num_client + 2, bw)
    net = Mininet(topo=topo,
                  host=CPULimitedHost, link=TCLink,
                  autoStaticArp=False)
    net.start()
    if output_info:
        info('* Dumping host connections\n')
        dumpNodeConnections(net.hosts)
    hosts = net.getNodeByName(*['h{}'.format(i) for i in range(1, num_client + 3)])
    # CLI(net)
    print(net.iperf([hosts[0], hosts[1]]))
    hosts[0].cmd('./build/tcpTracker > ./Dst/P2P/Tracker.log&')
    # h1 ./build/tcpTracker > ./Dst/P2P/Tracker.log&
    # h2 ./build/tcpNode 0 10.10.0.1 18888 ./Dst/P2P/Node1/ File_10M  > ./Dst/P2P/Node1.log&
    # h3 ./build/tcpNode 0 10.10.0.1 18888 ./Dst/P2P/Node2/ File_10M
    for i in range(1, num_client + 2):
        cmd_string = './build/tcpNode 0 10.10.0.1 18888 ./Dst/P2P/Node{}/ File_{}M  > ./Dst/P2P/Node{}.log&'.format(i, file_size, i)
        hosts[i].cmd(cmd_string)
        time.sleep(time_delay)

    sleep_time = 3 * 8 * file_size * num_client / bw
    print("Sleep {} seconds".format(sleep_time))
    time.sleep(sleep_time)  # Wait until experiment finished
    net.stop()

    # read logs, get execution time
    tot_time = 0
    try:
        for i in range(1, num_client + 2):
            with open('./Dst/P2P/Node{}.log'.format(i), 'r') as fp:
                lines = fp.readlines()
                for line in lines:
                    if len(line) > 0:
                        if line[0] == '#':
                            curr_time = float(line[1:])
                            tot_time += curr_time
    except IndexError:
        tot_time += 10000

    average_time = tot_time / num_client
    average_speed = 8 * file_size / average_time
    print("P2P Average download time is: {}".format(average_time))
    print("P2P Average download speed is: {:.3f}Mbps/s".format(8 * file_size / average_time))
    return average_time, average_speed


def build_cxx_executable():
    """
    This function build CXX_executables
    :return:
    """
    os.system("mkdir build")
    print("Build CXX executable")
    ret = os.system("cd build && cmake ../cxx")
    ret = ret or os.system("cd build && make > /dev/null")
    ret = ret or os.system("chmod -R 777 build")
    if ret: print("Build Complete")
    return ret


def create_test_folders():
    """
    This function create test folders
    :return:
    """
    os.system("mkdir Files")  # Origin Files
    os.system("mkdir Dst")  # Download destination
    os.system("chmod 777 Files")
    os.system("chmod 777 Dst")


def create_test_files(file_size):
    """
    This function use dd to generate empty files
    :return:
    """
    os.system("dd if=/dev/urandom of=./Files/File_{}M bs=1M count={} > /dev/null 2>&1".format(file_size, file_size))
    os.system("chmod -R 777 ./Files")


def prepare_cs_test_folders():
    """
    This function create prepare C/S test folders
    :param number_client:
    :return:
    """
    os.system("mkdir ./Dst/CS")
    os.system("mkdir ./Dst/CS/Downloads")
    os.system("chmod -R 777 ./Dst")


def prepare_p2p_test_folders(number_client: int, file_size):
    """
    This function creates the folders for P2P nodes
    :param number_node: number of nodes
    :return:
    """
    os.system("mkdir ./Dst/P2P")

    for i in range(1, number_client + 2):
        os.system("mkdir ./Dst/P2P/Node{}".format(i))
        os.system("chmod 777 ./Dst/P2P/Node{}".format(i))
    os.system("dd if=/dev/urandom of=./Dst/P2P/Node1/File_{}M bs=1M count={} > /dev/null 2>&1".format(file_size, file_size))
    os.system("chmod -R 777 ./Dst/P2P")


def chk_cs_md5_sum(number_client, file_size):
    f = os.popen("md5sum ./Files/File_{}M".format(file_size))
    src_md5 = f.readline().split(' ')[0]
    if output_info: print("c/s src", src_md5)

    flag = True

    for i in range(1, number_client + 1):
        dst_md5 = os.popen("md5sum ./Dst/CS/Downloads/Client{}_File_{}M".format(i, file_size)).readline().split(" ")[0]
        if output_info: print("c/s dst", i, dst_md5)
        if dst_md5 != src_md5:
            flag = False

    return flag


def chk_p2p_md5_sum(number_client, file_size):
    f = os.popen("md5sum ./Dst/P2P/Node1/File_{}M".format(file_size))
    src_md5 = f.readline().split(' ')[0]
    if output_info: print("p2p src", src_md5)

    flag = True

    for i in range(2, number_client + 2):
        dst_md5 = os.popen("md5sum ./Dst/P2P/Node{}/File_{}M".format(i, file_size)).readline().split(" ")[0]
        if output_info: print("p2p node", i, dst_md5)
        if dst_md5 != src_md5:
            flag = False

    return flag


def clean_up():
    os.system('rm -rf ./Dst')
    os.system('rm -rf ./Files')
    # os.system('rm -rf ./build')


def clean_up_build():
    os.system('rm -rf ./build')


def main(args):
    global output_info

    # Hyper parameters
    bw = args.bw
    max_number_client = args.max_client
    min_number_client = args.min_client
    file_size = args.fs
    time_delay = args.delay
    output_info = args.output_info

    assert min_number_client >= 1
    assert max_number_client >= min_number_client
    # set up experiment parameters
    number_client_list = [i for i in range(min_number_client, max_number_client + 1)]
    print(number_client_list)
    # results
    average_speed_cs_list = []
    average_speed_p2p_list = []

    # Delete possible output / build
    clean_up()
    clean_up_build()

    # Build CXX executable
    ret = build_cxx_executable()
    if ret != 0:
        # Failed to build
        exit(-1)

    for number_client in number_client_list:
        # Delete possible output
        clean_up()

        # Create test folders
        create_test_folders()
        create_test_files(file_size)
        prepare_cs_test_folders()
        prepare_p2p_test_folders(number_client, file_size)

        # clean up mininet
        quietRun('sudo mn -c')
        setLogLevel('warning')
        if output_info: setLogLevel('info')
        print("Number of nodes: ", number_client)
        # run test0
        _, average_time_cs = test_cs_model(number_client, file_size=file_size, bw=bw)
        _, average_time_p2p = test_p2p_model(number_client, file_size=file_size, bw=bw, time_delay=time_delay)

        # save result
        average_speed_cs_list.append(average_time_cs)
        average_speed_p2p_list.append(average_time_p2p)
        print("C/S MD5 check: {}".format(chk_cs_md5_sum(number_client, file_size)))
        print("P2P MD5 check: {}".format(chk_p2p_md5_sum(number_client, file_size)))

    print(average_speed_cs_list)
    print(average_speed_p2p_list)
    # plot
    if plt_ready:
        plt.figure(figsize=(8, 6))
        plt.title("Relationship between average download speed and number of peers")
        plt.xlabel("Number of peers")
        plt.ylabel("Speed (Mbps)")
        plt.legend()
        p1 = plt.plot(number_client_list, average_speed_cs_list, color='r', label="C/S")
        p2 = plt.plot(number_client_list, average_speed_p2p_list, color='b', label="P2P, delay:{}s".format(time_delay))
        plt.legend()

        # [5.38233410300711, 5.71299192564998, 5.185853199904788, 5.306346757191173, 3.8796290011656542, 3.825504673128916, 3.7453599087218343, 3.6437973556302152, 3.3665051765043468, 3.272029930272102, 2.907112602147141, 2.6635758458563132, 1.8586914092950522, 0.7367730609024072, 0.6800636670164552, 0.650414389017527, 0.6093320888657338, 0.5581565063029482, 0.546315900038337]
        # [8.22570512286633, 4.353409808232298, 3.045839890349764, 2.321651797212422, 1.8327597558947282, 0.04758286717406194, 0.05574256134182698, 0.064, 0.0710217934022786, 0.08, 0.088, 0.096, 0.104, 0.11199999999999999, 0.12000000000000001, 0.1268325796670609, 0.136, 0.14400000000000002, 0.15200000000000002]
        plt.savefig('plot.svg')
        plt.xticks(range(1, max_number_client + 1, 1))
        plt.show()


if __name__ == '__main__':
    import sys
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--min_client', type=int, default=1, help='Min number of client')
    parser.add_argument('--max_client', type=int, default=5, help='Max number of client')
    parser.add_argument('--fs', type=int, default=10, help='File size')
    parser.add_argument('--delay', type=float, default=3.0, help='P2P node launch delay')
    parser.add_argument('--bw', type=float, default=10.0, help='Mininet bandwidth')
    parser.add_argument('--output_info', type=bool, default=False, help='Output detail info')
    args = parser.parse_args()
    main(args)
