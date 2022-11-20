#pragma once
#include "user.h"

#include <vector>
#include <string>
#include <memory>

using namespace std;


class Channel {
public:
    Channel(string channelName);
    ~Channel();

    void JoinUser(shared_ptr<User> pUser);
    void LeaveUser(shared_ptr<User> pUser);
    const vector<shared_ptr<User>>& GetActiveUsers();
    void SetChannelName(string channelName);
    void SetTopic(string topic);
    string GetChannelName();
    string GetUserList();
    string GetTopic();
    bool HasTopic();
    bool IsUserInChannel(shared_ptr<User> pUser);
    
private:
    vector<shared_ptr<User>> users;
    string topic = "";
    string channelName;
};