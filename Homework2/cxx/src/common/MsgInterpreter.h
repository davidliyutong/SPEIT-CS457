//
// Created by 厉宇桐 on 2020/10/4.
//

#ifndef P2P_FILE_SHARE_MSGINTERPRETER_H
#define P2P_FILE_SHARE_MSGINTERPRETER_H

#include <queue>
#include "data.h"

using namespace std;

class MsgInterpreter {
    /*
     * 这个Class的设计目的是拆分TCP通讯下收到的字符，重组成有用的信息
     * 在C/S模型中，控制命令的第一个字代表了该命令的类型，紧接着是命令的内容，然后用回车结尾
     * 在P2P(TCP)模型中，控制命令的第一个字代表命令的类型，紧接着是与之相关的信息，最后补零到规定长度，回车结尾
     * 因此该类只在C/S模型中使用过
     */
public:
    struct ClientMsg m_interpret(char *a_Buf, int a_nLen){

        struct ClientMsg LastMsg('N');
        char cTmp;
        bool flag = false;
        // 从接受缓冲中读取字符
        for (auto i = 0; i <= a_nLen; i++) {
            m_qUnprocessed.push(a_Buf[i]);
        }

        // 根据换行符切分数据
        while (not m_qUnprocessed.empty()) {
            cTmp = m_qUnprocessed.front();
            m_qUnprocessed.pop();
            if (cTmp == '\n') {
                flag = true;
                break;
            }
            if (cTmp != 0) {
                m_qInprocess.push(cTmp);
            }
        }

        // 将切分的数据组织成消息结构体，返回处理好的结构体
        if (flag) {
            LastMsg.type = m_qInprocess.front();
            m_qInprocess.pop();
            while (not m_qInprocess.empty()) {
                LastMsg.content.push_back(m_qInprocess.front());
                m_qInprocess.pop();
            }
        } else {
            return LastMsg;
        }

        return LastMsg;

    }


private:
    queue<char> m_qUnprocessed;
    queue<char> m_qInprocess;
};

#endif //P2P_FILE_SHARE_MSGINTERPRETER_H
