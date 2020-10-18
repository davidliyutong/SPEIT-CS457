//
// Created by liyutong on 2020/9/28.
//

#ifndef P2P_FILE_SHARE_NETWORK_H
#define P2P_FILE_SHARE_NETWORK_H
bool recv_s_in(queue<socketMsg> recv_queue) {
    struct socketMsg msg{};
    int status = read_from_s_in(string_in_tmp, socket_info_src);
    if (!status) return false;
    msg.str = string_in_tmp;
    msg.socket_info = socket_info_src;
    while (recv_queue.size() <= MAX_RECV_QUEUE_SIZE)
    {
        recv_queue.push(msg);
    }
    return true;
}

bool send_s_out(queue<socketMsg>  send_queue) {
    struct socketMsg msg{};
    if (send_queue.empty()) return false;
    msg = send_queue.front();
    try {
        write_to_s_out(msg.str, msg.socket_info);
        send_queue.pop();
        return true;
    } catch (const char * err){
        return false;
    }
}
#endif //P2P_FILE_SHARE_NETWORK_H
