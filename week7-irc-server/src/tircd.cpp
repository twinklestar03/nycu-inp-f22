#include "tircd.h"

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netdb.h>


using namespace std;


TIrcd::TIrcd() {
    pCommandHander = make_unique<CommandHander>();
}

TIrcd::~TIrcd() {
    close(epollFd);
    close(socketFd);
}

void TIrcd::Run(int port) {
    struct sockaddr_in          addr;
    struct epoll_event          ev;
    int                         opt = 1;
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1) {
        cerr << "socket failed" << endl;
        exit(1);
    }
    
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        cerr << "bind failed" << endl;
        exit(1);
    }

    if (listen(socketFd, TIRCD_MAX_EVENTS) == -1) {
        cerr << "listen failed" << endl;
        exit(1);
    }

    epollFd = epoll_create(TIRCD_MAX_EVENTS);
    if (epollFd == -1) {
        cerr << "epoll_create failed" << endl;
        exit(1);
    }

    ev.events = EPOLLIN;
    ev.data.fd = socketFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, socketFd, &ev);
    cout << "TIrcd started on port " << port << endl;

    this->Work();
}

void TIrcd::Work() {
    struct epoll_event events[TIRCD_MAX_EVENTS];
    int nfds;
    while (true) {
        nfds = epoll_wait(epollFd, events, TIRCD_MAX_EVENTS, -1);
        if (nfds == -1) {
            cerr << "epoll_wait failed" << endl;
            exit(1);
        }
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == socketFd) {
                this->RegisterConnection(events[i].data.fd);
            } else {
                if (this->ReceiveMessages(events[i].data.fd) < 0) {
                    this->UnregisterConnetion(events[i].data.fd);
                }
            }
        }
        this->SendMessages();
    }
}

int TIrcd::ReceiveMessages(int fd) {
    if (connectionMap.find(fd) == connectionMap.end()) {
        cerr << "[/] User entry not found, dropping the connection." << endl;
        return -1;
    }

    // read until clrf
    string buffer;
    char c;
    int nread;
    while ( (nread = read(fd, &c, 1)) > 0) {
        if (nread < 0) {
            cerr << "[/] Error reading from socket." << endl;
            return -1;
        }
        if (c == '\r') {
            continue;
        } else if (c == '\n') {
            break;
        } else {
            buffer += c;
        }
    }

    if (buffer.size() < 1) {
        return -1;
    }

    // tokenizing the message
    vector<string> tokens;
    string toProcess;
    size_t pos = buffer.find(' ');
    size_t colonPos = buffer.find(':');
    size_t initialPos = 0;

    tokens.clear();

    if (colonPos != std::string::npos) {
        toProcess = buffer.substr(0, colonPos);
    } else {
        toProcess = buffer;
    }
    
    while( pos != std::string::npos ) {
        tokens.push_back(toProcess.substr(initialPos, pos - initialPos));
        initialPos = pos + 1;
        pos = toProcess.find(' ', initialPos);
    }

    if (colonPos != std::string::npos) {
        tokens.push_back(buffer.substr(colonPos + 1));
    } else {
        tokens.push_back(toProcess.substr(initialPos, std::min(pos, toProcess.size()) - initialPos + 1));
    }

    unique_ptr<RequestMessage> pRequestMessage = make_unique<RequestMessage>(
        connectionMap[fd], tokens[0], vector<string>(tokens.begin() + 1, tokens.end())
    );

    pCommandHander->HandleMessage(move(pRequestMessage));
    return 0;
}

void TIrcd::SendMessages() {
    while (!sendingQueue.empty()) {
        unique_ptr<ResponseMessage> pResponseMessage = move(sendingQueue.front());
        sendingQueue.pop();
        string message = pResponseMessage->ComposeString();

        for (auto user: pResponseMessage->GetRecipients()) {
            int fd = user->GetFd();
            if (connectionMap.find(fd) == connectionMap.end()) {
                cerr << "[/] User entry not found, dropping the connection." << endl;
                continue;
            }
            send(fd, message.c_str(), message.length(), 0);
        }
    }
}

void TIrcd::RegisterConnection(int fd) {
    struct sockaddr_in          addr;
    struct epoll_event          ev;
    socklen_t                   len = sizeof(addr);
    int                         connFd = accept(fd, (struct sockaddr*)&addr, &len);
    if (connFd == -1) {
        cerr << "accept failed" << endl;
        exit(1);
    }
    ev.events = EPOLLIN;
    ev.data.fd = connFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, connFd, &ev);

    // get client ip address
    string hostname = string(inet_ntoa(addr.sin_addr));
    connectionMap[connFd] = make_shared<User>(connFd, string(hostname));
    cout << "[*] New connection from " << hostname << ":" << ntohs(addr.sin_port) << endl;
}

void TIrcd::UnregisterConnetion(int fd) {
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    nickMap.erase(connectionMap[fd]->GetNickname());
    connectionMap.erase(fd);
    cout << "[*] Connection closed" << endl;
}

void TIrcd::EnqueueMessage(unique_ptr<ResponseMessage> pResponseMessage) {
    sendingQueue.push(move(pResponseMessage));
}

void TIrcd::EnqueueMessage(vector<unique_ptr<ResponseMessage>> pResponseMessages) {
    for (auto& pResponseMessage: pResponseMessages) {
        sendingQueue.push(move(pResponseMessage));
    }
}

bool TIrcd::IsNicknameInUse(string nickname) {
    return nickMap.find(nickname) != nickMap.end();
}

int TIrcd::GetUsersCount() {
    return connectionMap.size();
}

map<int, shared_ptr<User>> TIrcd::GetConnectionMap() {
    return connectionMap;
}

Channel& TIrcd::CreateOrGetChannel(string channelName) {
    for (auto& channel: channels) {
        if (channel->GetChannelName() == channelName) {
            return *channel;
        }
    }
    
    auto channel = make_unique<Channel>(channelName);
    channels.push_back(move(channel));
    return CreateOrGetChannel(channelName);
}

vector<Channel> TIrcd::GetChannelList() {
    vector<Channel> channelList;
    for (auto& channel: channels) {
        channelList.push_back(*channel);
    }
    return channelList;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <port>\n";
        exit(0);
    }

    TIrcd::GetInstance().Run(atoi(argv[1]));


    return 0;
}