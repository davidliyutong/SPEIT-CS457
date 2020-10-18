import glob
import json
import os

class FileINdexer:
    def __init__(self, path='./'):
        self.data = dict()
        self.from_path(path)

    @property
    def json(self):
        return json.dumps(self.data)

    def bjson(self):
        return json.dumps(self.data).encode('ascii')

    def clear(self):
        self.data = dict()

    def from_json_string(self, json_string):
        self.data = json.loads(json_string)

    def from_path(self, path):
        self.clear()
        file_list = []
        # dir_list = []
        for root, dirs, files in os.walk(path, topdown=False):
            for name in files:
                relative_path = root[len(path):]
                entry = os.path.join(root[len(path):], name)
                file_list.append(entry)
                it = self.data
                for dir in relative_path.split('/')[1:]:
                    if dir not in it.keys():
                        it[dir] = dict()
                    it = it[dir]
                it[name] = os.path.getmtime(os.path.join(root, name))
            # for name in dirs:
            #     dir_list.append(os.path.join(root[len(path):], name))

        return




if __name__ == '__main__':
    instance = FileINdexer()
    instance.from_path('/home/liyutong/Documents/CS457/Homework2/Files')
    x = instance.json

    print('end')