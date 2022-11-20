#pragma once
#include "user.h"
#include "messages.h"

#include "handlers/command_handler.h"

#include <string>
#include <cstring>
#include <map>
#include <queue>
#include <memory>


#define TIRCD_MAX_EVENTS 1000

using namespace std;


class TIrcd {
public:
    static TIrcd& GetInstance(){
        static TIrcd gInstance;
        return gInstance;
    }

    void                Run(int port);
    void                EnqueueMessage(unique_ptr<ResponseMessage> pResponseMessage);
    void                EnqueueMessage(vector<unique_ptr<ResponseMessage> > pResponseMessages);
    bool                IsNicknameInUse(string nickname);
    void                UpdateNickMap(shared_ptr<User> pUser);
    map<int, shared_ptr<User>>  GetConnectionMap();
    int                 GetUsersCount();
    vector<Channel>     GetChannelList();
    Channel&            CreateOrGetChannel(string channelName);
    bool                IsChannelExists(string channelName);

private:
    TIrcd();
    ~TIrcd();

    // read the message from fd, then register the message into the system.
    void Work();
    int  ReceiveMessages(int fd);
    // process the sending queue
    void SendMessages();
    void RegisterConnection(int fd);
    void UnregisterConnetion(int fd);

    int epollFd;
    int socketFd;

    // can improve proformance by using worker model
    unique_ptr<CommandHandler> pCommandHander;

    map<int, shared_ptr<User>> connectionMap;
    map<string, shared_ptr<User>> nickMap;
    queue<unique_ptr<ResponseMessage>> sendingQueue;

    vector<unique_ptr<Channel>> channels;
};