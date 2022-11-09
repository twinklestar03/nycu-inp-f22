#pragma once
#include <string>


using namespace std;

class User {
public:
    User(int fd, string hostname);
    ~User();

    void SetUsername(string username);
    void SetNickname(string nickname);
    void SetRealname(string realname);
    void SetHostname(string hostname);
    void SetRegistered(bool registered);

    int GetFd();
    string GetUsername();
    string GetNickname();
    string GetRealname();
    string GetHostname();
    bool   IsRegistered();

private:
    bool isRegistered = false;
    int connFd;
    string username;
    string nickname;
    string realname;
    string hostname;
};