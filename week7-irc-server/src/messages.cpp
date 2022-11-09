#include "messages.h"

#include <iostream>
#include <algorithm>


Message::Message(shared_ptr<User> from, string command, vector<string> params)
    : sender(from), command(command), params(params) {
}

Message::~Message() {
}

shared_ptr<User> Message::GetSender() {
    return sender;
}

string Message::GetCommand() {
    return command;
}

vector<string> Message::GetParams() {
    return params;
}

const string Message::ComposeString() {
    return "";
}

RequestMessage::RequestMessage(shared_ptr<User> from, string command, vector<string> params)
    : Message(from, command, params) {
}

RequestMessage::~RequestMessage() {
}

const string RequestMessage::ComposeString() {
    // TODO: compose the message
    return "";
}

// Server Messages
ResponseMessage::ResponseMessage(shared_ptr<User> sender, ResponseCode code, vector<string> message)
    : Message(sender, string(3 - min((size_t)3, to_string(static_cast<int>(code)).length()), '0')+to_string(static_cast<int>(code)), message) {
    AddRecipient(sender);
}

// Client Messages
ResponseMessage::ResponseMessage(shared_ptr<User> sender, RequestMessage request) 
    : Message(sender, request.GetCommand(), request.GetParams()) {
    AddRecipient(sender);
    isRelay = true;
}

ResponseMessage::ResponseMessage(shared_ptr<User> sender, string command, vector<string> messages)
    : Message(sender, command, messages) {
    AddRecipient(sender);
    isRelay = true;
}

ResponseMessage::ResponseMessage(shared_ptr<User> sender, vector<shared_ptr<User>> toList, string command, vector<string> messages)
    : Message(sender, command, messages) {
    for (auto user : toList) {
        if (user == sender)
            continue;
        AddRecipient(user);
    }
    isRelay = true;
}

ResponseMessage::~ResponseMessage() {
}

void ResponseMessage::AddRecipient(shared_ptr<User> recipient) {
    recipients.push_back(recipient);
}

vector<shared_ptr<User>> ResponseMessage::GetRecipients() {
    return recipients;
}

const string ResponseMessage::ComposeString() {
    // compose string
    string ret;

    if (!isRelay) { // Server Message
        ret += ":tircd " + command + " " + sender->GetNickname();
        for (auto it = params.begin(); it != params.end(); ++it) {
            if (it == params.end() - 1) {
                ret += " :" + *it;
            } else {
                ret += " " + *it;
            }
        }
        ret += "\r\n";
        cout << "[DEBUG][ResponseMessage::ComposeString()] [Server] ->" << ret << endl;
        return ret;
    }

    // Client Message
    ret += ":" + sender->GetNickname() + " " + command;
    for (auto it = params.begin(); it != params.end(); ++it) {
        ret += " " + *it;   
    }
    ret += "\r\n";
    cout << "[DEBUG][ResponseMessage::ComposeString()] [Client] ->" << ret << endl;
    return ret;
}