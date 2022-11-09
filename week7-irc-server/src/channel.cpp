#include "channel.h"
#include <iostream>
#include <algorithm>


Channel::Channel(string channelName) : channelName(channelName) {
}

Channel::~Channel() {
}

void Channel::JoinUser(shared_ptr<User> pUser) {
    users.push_back(pUser);
}

void Channel::LeaveUser(shared_ptr<User> pUser) {
    auto it = find(users.begin(), users.end(), pUser);
    if (it != users.end()) {
        users.erase(it);
    }
}

const vector<shared_ptr<User>>& Channel::GetActiveUsers() {
    return users;
}

void Channel::SetChannelName(string channelName) {
    this->channelName = channelName;
}

void Channel::SetTopic(string topic) {
    cout << "[DEBUG][Channel::SetTopic()] " << topic << endl;
    this->topic = topic;
}

string Channel::GetChannelName() {
    return channelName;
}

bool Channel::HasTopic() {
    if (topic.size() > 0) {
        return true;
    }
    return false;
}

string Channel::GetTopic() {
    return topic;
}

string Channel::GetUserList() {
    string userList = "";
    for (auto user : users) {
        userList += user->GetNickname() + " ";
    }
    return userList;
}

