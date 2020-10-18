//
// Created by liyutong on 2020/10/5.
//

#ifndef P2P_FILE_SHARE_PATH_H
#define P2P_FILE_SHARE_PATH_H

#include <cstring>
#include <iostream>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

using namespace std;

string PathConcat(string &path1, string &path2);

string PathConcat(const vector<string> &paths);

vector<string> PathSplit(string &path);


vector<string> ListDirExt(string &a_DirPath);

vector<string> ListFileExt(string &a_DirPath) ;

#endif //P2P_FILE_SHARE_PATH_H
