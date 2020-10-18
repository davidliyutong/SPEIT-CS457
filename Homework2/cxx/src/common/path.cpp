//
// Created by liyutong on 2020/10/11.
//

#include <cstring>
#include <iostream>
#include <vector>
#include <dirent.h>
#include "path.h"

using namespace std;

string PathConcat(string &path1, string &path2) {
    /*
     * 将两个路径合并（将路径中的'..'和'.'转义，最终没有完成
     */
    string res;
    return res;
}

string PathConcat(const vector<string> &paths) {
    /*
     * 将多个路径合并
     */
    string res;
    return res;
}

vector<string> PathSplit(string &path) {
    /*
     * 将路径拆分
     */
    vector<string> res;
    return res;
}


vector<string> ListDirExt(string &a_DirPath) {
    /*
     * 利用Linux系统命令列出目录下的文件夹
     */
    DIR *dir;
    struct dirent *ptr;
    vector<string> res;
    // 打开目录
    dir = opendir(a_DirPath.c_str()); ///open the dir

    if (dir == nullptr) { // 目录是空的
        return res;
    }

    while ((ptr = readdir(dir)) != nullptr) ///read the list of this dir
    {
        if (ptr->d_type == 4) { // 是文件夹
            res.emplace_back(ptr->d_name);
        }
    }
    closedir(dir);
    return res;
}

vector<string> ListFileExt(string &a_DirPath) {
    /*
     * 利用Linux系统命令列出目录下的文件
     */
    DIR *dir;
    struct dirent *ptr;
    vector<string> res;
    // 打开目录
    dir = opendir(a_DirPath.c_str()); ///open the dir

    if (dir == nullptr) { // 目录是空的
        return res;
    }

    while ((ptr = readdir(dir)) != nullptr) ///read the list of this dir
    {
        if (ptr->d_type == 8) { //是文件
            res.emplace_back(ptr->d_name);
        }
    }
    closedir(dir);
    return res;
}