#include "user.h"


User::User(int fd, string hostname) {
    this->connFd = fd;
    this->hostname = hostname;
}

User::~User() {
}

void User::SetUsername(string username) {
    this->username = username;
}

void User::SetNickname(string nickname) {
    this->nickname = nickname;
}

void User::SetRealname(string realname) {
    this->realname = realname;
}

void User::SetRegistered(bool registered) {
    this->isRegistered = registered;
}

void User::SetHostname(string hostname) {
    this->hostname = hostname;
}

string User::GetUsername() {
    return this->username;
}

string User::GetNickname() {
    return this->nickname;
}

string User::GetRealname() {
    return this->realname;
}

string User::GetHostname() {
    return this->hostname;
}

int User::GetFd() {
    return this->connFd;
}

bool User::IsRegistered() {
    return this->isRegistered;
}

