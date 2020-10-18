//
// Created by 厉宇桐 on 2020/10/4.
//

#ifndef P2P_FILE_SHARE_STORAGEDAEMON_H
#define P2P_FILE_SHARE_STORAGEDAEMON_H

#include <cstring>
#include <cstdio>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <mutex>
#include <vector>
#include <map>
#include <algorithm>
#include <dirent.h>
#include <sys/types.h>
#include <cmath>
#include <sys/stat.h>

#include <fcntl.h>
#include "../common/macros.h"
#include "../common/utils.h"

using namespace std;

class FileDelegate {
    /*
     * 此对象是文件在内存的代理
     */
public:
    string FilePath; //文件的相对路径（相对程序的执行目录）
    int64_t nFileLen; //文件的大小（字节Byte)
    bool bIsReady; // 文件代理是否准备好了（是否知道文件的长度，是否在磁盘上创建占位符
    vector<bool> IsBlockRecved; // 文件的各个块有没有都收到
    vector<bool> IsBlockRecving;

    explicit FileDelegate(string &a_FilePath) {
        FilePath = a_FilePath;
        nFileLen = m_FileExist();
        if (nFileLen > 0) { // 如果文件存在
            if (not m_LockExist()) { // 如果文件的锁不存在
                printf("File exist\n");
                // 修改所有Block的状态为
                for (auto i = 0; i < NumBlocks(); ++i) {
                    IsBlockRecved.push_back(true);
                    IsBlockRecving.push_back(true);
                }
                bIsReady = true;
            } else { // 文件的锁存在，表示有传输被终止，本地文件不完整
                m_RemoveFile();
                m_RemoveLock();
                bIsReady = false;
            }
        } else {
            m_RemoveLock();
            m_RemoveFile();
            bIsReady = false;
        }
    }


    bool IsComplete() {
        // 这里没有更新IsBlockRecving，是个潜在的隐患
        if (IsBlockRecved.empty()) return false;
        bool ret = all_of(IsBlockRecved.begin(), IsBlockRecved.end(), [](bool item) { return item; });
        if (ret) m_RemoveLock();
        return ret;
    }

    string m_GetFileName() const {
        string res;
        int index = FilePath.rfind('/');
        if (index < FilePath.length()) {
            res.assign(FilePath, index + 1);
        }
        return res;

    }

    string m_GetDirName() const {
        string res;
        int index = FilePath.rfind('/');
        if (index < FilePath.length()) {
            res.assign(FilePath, 0, index);
        }
        return res;
    }

    uint64_t NumBlocks() const {
        if (nFileLen % BLOCKSIZE == 0) {
            return (nFileLen / BLOCKSIZE);
        } else {
            return (nFileLen / BLOCKSIZE) + 1;
        }
    }

    bool m_Update(int a_nFileLen) {
        lockbIsReady.lock();
        if (bIsReady) return true;

        bool ret = true;
        nFileLen = a_nFileLen;
        string DirPath = m_GetDirName();
        MakeDirs(DirPath);
        ret = m_CreateLock();
        ret = ret and m_CreateFile();
        for (auto i = 0; i < NumBlocks(); ++i) {
            IsBlockRecved.push_back(false);
            IsBlockRecving.push_back(false);
        }
        bIsReady = true;
        lockbIsReady.unlock();
        return ret;

    }

    bool m_IsBlockAvailable(uint64_t a_nBlockNumber) {
        if (a_nBlockNumber > NumBlocks()) return false;
        if (not bIsReady) return false;
        return (IsBlockRecved[a_nBlockNumber]);
    }

    int m_FileExist() const {
        struct stat info{};
        int ret = stat(FilePath.c_str(), &info);
        if (ret < 0) {
            return -1;
        } else {
            return info.st_size;
        }
    }

    bool m_LockExist() const {
        struct stat info{};
        int ret = stat((FilePath + string(".lock")).c_str(), &info);
        if (ret < 0) return false;
        else return true;
    }

    bool m_CreateLock() const {
        int ret = creat((FilePath + string(".lock")).c_str(), 0755);
        if (ret < 0) return false;
        else return true;
    }

    bool m_RemoveLock() const {
        return remove((FilePath + string(".lock")).c_str());
    }

    bool m_CreateFile() const {
        uint64_t nByteLeft = nFileLen;

        char EmptyBuf[FILEBUFFLEN];
        memset(EmptyBuf, 0, sizeof(char) * FILEBUFFLEN);
        string DirPath = m_GetDirName();
        int ret = MakeDirs(DirPath);
        FILE *pFOUT;
        pFOUT = fopen(FilePath.c_str(), "w");
        int64_t nByteWrite = 0;
        if (pFOUT != nullptr) {
            while (nByteLeft > 0) {
                if (nByteLeft <= FILEBUFFLEN) {
                    nByteWrite += fwrite(EmptyBuf, sizeof(char), nByteLeft, pFOUT);
                    nByteLeft -= nByteLeft;
                } else {
                    nByteWrite += fwrite(EmptyBuf, sizeof(char), FILEBUFFLEN, pFOUT);
                    nByteLeft -= FILEBUFFLEN;
                }
            }
            printf("Created empty file, file len: %ld\n", nByteWrite);
            fclose(pFOUT);
            return true;
        } else {
            return false;
        }
    }

    bool m_RemoveFile() const {
        return remove(FilePath.c_str());
    }

    bool m_SetBlockRecvState(uint64_t a_Index, bool a_Value) {
        lockIsBockRecv.lock();
        IsBlockRecved[a_Index] = a_Value;
        lockIsBockRecv.unlock();
        return true;
    }

    bool m_GetBlockRecvFlag(uint64_t a_Index) {
        // 多线程
        lockIsBockRecv.lock();
        if (IsBlockRecving[a_Index]) {
            lockIsBockRecv.unlock();
            return false;
        } else {
            IsBlockRecving[a_Index] = true;
            lockIsBockRecv.unlock();
            return true;
        }
    }

    bool m_RetBlockRecvFlag(uint64_t a_Index) {
        // 多线程
        lockIsBockRecv.lock();
        if (not IsBlockRecving[a_Index]) {
            lockIsBockRecv.unlock();
            return false;
        } else {
            IsBlockRecving[a_Index] = false;
            lockIsBockRecv.unlock();
            return true;
        }
    }

    uint64_t m_GetBlockLen(uint64_t a_nBlockNumber) const {
        if (nFileLen == 0) {
            return -1;
        } else {
            return min((nFileLen - a_nBlockNumber * BLOCKSIZE), (uint64_t) BLOCKSIZE);
        }
    }

    FileDelegate &operator=(const FileDelegate &rhs) {
        if (this == &rhs) {
            return *this;
        }
        this->FilePath = rhs.FilePath;
        this->nFileLen = rhs.nFileLen;
        memcpy((void *) (&this->lockIsBockRecv), (void *) (&rhs.lockIsBockRecv), sizeof(rhs.lockIsBockRecv));
        this->IsBlockRecved = rhs.IsBlockRecved;
    }

private:
    mutex lockIsBockRecv;
    mutex lockbIsReady;
};

class StorageDaemon {
public:
    string m_ListDirDisp(string &a_DirPath) {
        DIR *dir;
        struct dirent *ptr;
        string res;
        dir = opendir(m_CompletePath(a_DirPath).c_str()); ///open the dir

        if (dir == nullptr) {
            return res;
        }

        while ((ptr = readdir(dir)) != nullptr) ///read the list of this dir
        {
            res += "Type: ";
            switch (ptr->d_type) {
                case 4:
                    res += "DIR";
                    break;
                case 8:
                    res += "FIL";
                    break;
                default :
                    res += "N/A";
            }

            res += "\tName: ";
            res += ptr->d_name;
            res += "\n";
        }
        closedir(dir);
        return res;
    }

    int64_t m_FileExist(string &a_FilePath) {
        struct stat info{};
        int64_t ret = stat(m_CompletePath(a_FilePath).c_str(), &info);
        if (ret < 0) {
            return -1;
        } else {
            return info.st_size;
        }

    }

    bool m_CreateFileDelegate(string &a_FilePath) {
        lockFileDelegates.lock();
        if (m_FileDelegateExist(a_FilePath)) {
            lockFileDelegates.unlock();
            return true;
        } else {
            string Tmp = m_CompletePath(a_FilePath);
            mapFileDelegates.emplace(a_FilePath, Tmp);
            mapFileDelegatesCnt.emplace(a_FilePath, 0);
            mapFileDelegatesTime.emplace(a_FilePath, time(nullptr));
            lockFileDelegates.unlock();
            return true;
        }
    }

    bool m_FileDelegateExist(string &a_FilePath) {
        auto it = mapFileDelegates.find(a_FilePath);
        if (it == mapFileDelegates.end()) {
            return false;
        } else {
            return true;
        }
    }

    FileDelegate *m_GetFileDelegatePtr(string &a_FilePath){
        lockFileDelegates.lock();
        FileDelegate *res = nullptr;
        auto it = mapFileDelegates.find((a_FilePath));
        if (it != mapFileDelegates.end()) {
            res = &it->second;
            mapFileDelegatesCnt[a_FilePath] += 1;
            mapFileDelegatesTime[a_FilePath] = time(nullptr);
        }
        lockFileDelegates.unlock();
        return res;

    }

    bool m_RetFileDelegatePtr(FileDelegate **a_ppFDelegate){
        lockFileDelegates.lock();
        auto it2 = mapFileDelegatesCnt.find((*a_ppFDelegate)->FilePath);
        if (it2 != mapFileDelegatesCnt.end()) {
            (*a_ppFDelegate) = nullptr;
            mapFileDelegatesCnt[(*a_ppFDelegate)->FilePath] -= 1;
            mapFileDelegatesTime[(*a_ppFDelegate)->FilePath] = time(nullptr);
        }
        lockFileDelegates.unlock();
        return true;
    }

    void m_Loop(){
        /* 清除未使用的文件代理
         * 正常退出的进程Get文件代理，使用完毕后归还文件代理
         * mapFileDelegatesCnt记录了文件代理的使用情况 mapFileDelegatesTime 记录了上一次归还/调取时间
         * 如果某个代理被归还了很长时间，可以判断该文件代理已经使用完毕
        */
    }

    string m_CompletePath(string &a_Path) {
        string CompleteFileName = sRootPath + a_Path;
        return CompleteFileName;
    }

    explicit StorageDaemon(string &a_RootPath) {
        sRootPath.assign(a_RootPath);

    }

private:
    string sRootPath;
    map<string, FileDelegate> mapFileDelegates;
    map<string, int> mapFileDelegatesCnt;
    map<string, time_t> mapFileDelegatesTime;
    mutex lockFileDelegates;

};

#endif //P2P_FILE_SHARE_STORAGEDAEMON_H
