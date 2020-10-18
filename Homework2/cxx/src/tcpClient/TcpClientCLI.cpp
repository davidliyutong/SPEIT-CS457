//
// Created by liyutong on 2020/10/10.
//
#include "TcpClientMain.h"

using namespace std;

void cli() {
    /* 非常简陋的CLI界面 !!!!
     * 已知BUG
     * "cd [path]" 只是把 [path] 直接添加到path的末端 如 ./Folder1/ + ../Folder2 = ./Folder1/../Folder2
     * 下载文件无法重命名
    */

    // 输入一些登录信息
    bool bDebug = false;
    string sRemoteHostname;
    int RemotePort;
    cout << "-------- cli interface --------" << endl;
    cout << "server ip: ";
    cin >> sRemoteHostname;
    cout << "server port: ";
    cin >> RemotePort;

    // 生成用于连接的地址
    sockaddr_in RemoteAddr = GenRemoteAddr(sRemoteHostname, RemotePort);
    // 创建Socket
    int ClientSock = GenNewTCPSock();
    // 连接
    int ret = connect(ClientSock, (struct sockaddr *) &RemoteAddr, sizeof(RemoteAddr));
    if (ret < 0) {
        cout << "Connection failed due to port and ip problems" << endl;
        return;
    } else {
        cout << "Successfully connected" << endl;
    }

    string index; // 目录信息
    string command; // 指令
    string CurrDir("./"); // 当前目录信息
    string CurrDirTmp;
    string CurrFile; // 当前文件信息
    bool res;
    cin.ignore(1024, '\n');
    while (true) {
        cout << CurrDir << ">"; // 打印当前目录的位置
        getline(cin, command);
        if (command.substr(0, 2) == "ls") { // 命令-列出目录
            // 从Server获取当前目录下的文件和文件夹名称
            index = GetIndexFromServer(ClientSock, CurrDir, bDebug);
            if (index.length() > 0) {
                cout << index << endl;
            } else {
                cout << "No such directory" << endl;
            }
        } else if (command.substr(0, 2) == "cd") { // 命令-改变目录
            // 修改目录
            CurrDirTmp = CurrDir;
            CurrDirTmp.append(command.substr(3));
            CurrDirTmp.append("/");

            // 向服务器查询当前目录是否存在
            index = GetIndexFromServer(ClientSock, CurrDirTmp, bDebug);

            // 判断目录是否存在
            if (index.length() > 0) {
                // 如果存在，更新目录位置
                CurrDir = CurrDirTmp;
            } else {
                cout << "No such directory" << endl;
            }
        } else if (command.substr(0, 2) == "dl") { // 命令-下载文件
            // 拼接路径
            CurrFile = CurrDir;
            CurrFile.append(command.substr(3));

            // 从服务器下载文件
            res = GetFileFromServer(ClientSock, CurrFile, command.substr(3), bDebug);
            if (res) {
                cout << "Success" << endl;
            } else {
                cout << "Failure" << endl;
            }
        } else if (command.substr(0, 4) == "quit") {
            break;
        } else {
            // 未识别的命令
            if (command.length() > 0) cout << "unrecognized command" << endl;
            else cout << endl;
        }
    }

    cout << "-------- cli quitting ---------" << endl;

}