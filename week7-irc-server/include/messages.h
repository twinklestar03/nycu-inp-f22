#pragma once
#include "channel.h"
#include "user.h"

#include <string>
#include <vector>

using namespace std;


class Message {
public:
    Message(shared_ptr<User> from, string command, vector<string> params);
    ~Message();

    shared_ptr<User> GetSender();
    string& GetCommand();
    vector<string>& GetParams();

    // compose the message into a string
    virtual const string ComposeString();
protected:
    shared_ptr<User> sender;
    string command;
    vector<string> params;
};


class RequestMessage : public Message {
public:
    RequestMessage(shared_ptr<User> from, string command, vector<string> params);
    ~RequestMessage();

    const string ComposeString() override;
};


class ResponseMessage : public Message {
public:
    enum class ResponseCode {
        RPL_WELCOME         = 1,
        RPL_LUSERCLIENT     = 251,
        RPL_LISTSTART       = 321,
        RPL_LIST            = 322,
        RPL_LISTEND         = 323,
        RPL_NOTOPIC         = 331,
        RPL_TOPIC           = 332,
        RPL_NAMREPLY        = 353,
        RPL_ENDOFNAMES      = 366,
        RPL_MOTD            = 372,
        RPL_MOTDSTART       = 375,
        RPL_ENDOFMOTD       = 376,
        RPL_USERSSTART      = 392,
        RPL_USERS           = 393,
        RPL_ENDOFUSERS      = 394,

        ERR_NOSUCHNICK      = 401,
        ERR_NOSUCHCHANNEL   = 403,
        ERR_NORECIPIENT     = 411,
        ERR_NOTEXTTOSEND    = 412,
        ERR_UNKNOWNCOMMAND  = 421,
        ERR_NONICKNAMEGIVEN = 431,
        ERR_NICKCOLLISION   = 436,
        ERR_NOTONCHANNEL    = 442,
        ERR_NOTREGISTERED   = 451,
        ERR_NEEDMOREPARAMS  = 461,
        ERR_ALREADYREGISTRED= 462,
    };

    // As a relay message
    ResponseMessage(shared_ptr<User> sender, RequestMessage request);
    // As a server response
    ResponseMessage(shared_ptr<User> sender, ResponseCode code, vector<string> content);
    ResponseMessage(shared_ptr<User> sender, string command, vector<string> messages);
    ResponseMessage(shared_ptr<User> sender, vector<shared_ptr<User>> toList, string command, vector<string> messages);

    ~ResponseMessage();

    void SetRelay(bool relay);
    void SetResponseCode(ResponseCode response);

    void AddRecipient(shared_ptr<User> recipient);
    vector<shared_ptr<User>> GetRecipients();

    const string ComposeString() override;
private:
    bool isRelay = false;
    ResponseCode responseCode;
    vector<shared_ptr<User>> recipients;
};
