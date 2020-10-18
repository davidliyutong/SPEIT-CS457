//
// Created by liyutong on 2020/9/24.
//
#include <cstring>
#include <iostream>
#include <map>
#include <filesystem>

using namespace std;
struct ruid_path_entry {
    string ruid;
    string path;
};

class storage_daemon {
public:
    bool query_entry(unsigned int ruid, string &path);

    bool insert_entry(unsigned int ruid, const string &path);

    bool del_entry(unsigned int ruid);

    bool gen_index(const string &path);

    bool load_index(const string &index_path);

    bool save_index(const string &index_path);

    int query_path(unsigned int ruid, const string &path);

    int query_filepath(unsigned int ruid, const string &filepath);

    explicit storage_daemon(map<unsigned int, string> &input_ruid_table){};

private:
    map<unsigned int, string> ruid_table;
    string index_dir = ".share_sync";
    string index_filename = "index.id";
    map<unsigned int, map<string, unsigned int>> file_index_table;
};

bool storage_daemon::query_entry(unsigned int ruid, string &path) {
    auto it = storage_daemon::ruid_table.find(ruid);
    if (it != storage_daemon::ruid_table.end()) {
        path.assign(it->second);
        return true;
    } else {
        return false;
    }
}

bool storage_daemon::insert_entry(const unsigned int ruid, const string &path) {
    try {
        storage_daemon::ruid_table.insert(map<unsigned int, string>::value_type(ruid, path));
//        storage_daemon::entry_num++;
        return true;
    } catch (const char *err) {
        fprintf(stderr, "%s", err);
        return false;
    }

}

// Delete a key-path entry
bool storage_daemon::del_entry(unsigned int ruid) {
    auto it = storage_daemon::ruid_table.find(ruid);
    if (it != storage_daemon::ruid_table.end()) {
        storage_daemon::ruid_table.erase(it);
        return true;
    } else {
        return false;
    }
}

// Index a path, generate index file at /path/{index_dir}/{index_filename}
bool storage_daemon::gen_index(const string &path) {
    return false;
}

// Query info of a path. Must have a ruid. Return last modified time if path exists, -1 if not
int storage_daemon::query_path(unsigned int ruid, const string &path) {
    return 0;
}

// Query info of a file. Must have a ruid. Return last modified time if file exists, -1 if not.
int storage_daemon::query_filepath(unsigned int ruid, const string &filepath) {
    return 0;
}

// Load index file from disk
bool storage_daemon::load_index(const string &index_path) {
    return false;
}

// Save index file to disk
bool storage_daemon::save_index(const string &index_path) {
    return false;
}
