#include "handlers/command_handler.h"
#include "tircd.h"

#include <iostream>


void HandleNick(unique_ptr<RequestMessage> message);
void HandleUser(unique_ptr<RequestMessage> message);
void HandleQuit(unique_ptr<RequestMessage> message);
void HandleJoin(unique_ptr<RequestMessage> message);
void HandlePart(unique_ptr<RequestMessage> message);
void HandleMode(unique_ptr<RequestMessage> message);
void HandleTopic(unique_ptr<RequestMessage> message);
void HandleUsers(unique_ptr<RequestMessage> message);
void HandleList(unique_ptr<RequestMessage> message);
void HandlePrivMsg(unique_ptr<RequestMessage> message);
void HandleDefault(unique_ptr<RequestMessage> message);

CommandHandler::CommandHandler() {
    commandMap["NICK"] = (CommandHandlerFunction) &HandleNick;
    commandMap["USER"] = (CommandHandlerFunction) &HandleUser;
    // commandMap["QUIT"] = (CommandHandlerFunction) &HandleQuit;
    commandMap["JOIN"] = (CommandHandlerFunction) &HandleJoin;
    commandMap["PART"] = (CommandHandlerFunction) &HandlePart;
    // commandMap["MODE"] = (CommandHandlerFunction) &HandleMode;
    commandMap["TOPIC"] = (CommandHandlerFunction) &HandleTopic;
    commandMap["USERS"] = (CommandHandlerFunction) &HandleUsers;
    commandMap["LIST"] = (CommandHandlerFunction) &HandleList;
    commandMap["PRIVMSG"] = (CommandHandlerFunction) &HandlePrivMsg;
}

CommandHandler::~CommandHandler() {
}

void CommandHandler::HandleMessage(unique_ptr<RequestMessage> message) {
    string command = message->GetCommand();
    // vector<string> params = message->GetParams();
    // shared_ptr<User> sender = message->GetSender();

    // cout << "[DEBUG] [CommandHandler] Handling command: " << command << " ";
    // for (auto param : params) {
    //     cout << param << " ";
    // }
    // cout << endl;

    if (commandMap.find(command) != commandMap.end()) {
        CommandHandlerFunction handler = commandMap[command];
        (*handler)(move(message));
    } else {
        HandleDefault(move(message));
    }

    return;
}

void HandleNick(unique_ptr<RequestMessage> message) {
    vector<string> params = message->GetParams();
    shared_ptr<User> sender = message->GetSender();
    unique_ptr<ResponseMessage> response;
    if (params.size() != 1) {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NONICKNAMEGIVEN,
            vector<string>{"NICK", "No nickname given"});
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }

    if (TIrcd::GetInstance().IsNicknameInUse(params[0])) {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NICKCOLLISION,
            vector<string>{"NICK", params[0], "Nickname is already in use"});
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }

    sender->SetNickname(params[0]);
    TIrcd::GetInstance().UpdateNickMap(sender);
    return;
}

void HandleUser(unique_ptr<RequestMessage> message) {
    vector<string> params = message->GetParams();
    shared_ptr<User> sender = message->GetSender();
    unique_ptr<ResponseMessage> response;
    cout << "[DEBUG] [HandleUser] " << sender->GetNickname() << endl;
    if (params.size() != 4) {
            response = make_unique<ResponseMessage>(
                sender,
                ResponseMessage::ResponseCode::ERR_NEEDMOREPARAMS,
                vector<string>{"USER", "Not enough parameters"});
            TIrcd::GetInstance().EnqueueMessage(move(response));
            return;
        }
    sender->SetUsername(params[0]);
    sender->SetHostname(params[2]);
    sender->SetRealname(params[3]);

    vector<unique_ptr<ResponseMessage>> responses;
    responses.push_back(make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_WELCOME,
        vector<string>{"Welcome to the Internet Relay Network " + sender->GetUsername() + "!!"}));
    responses.push_back(make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_LUSERCLIENT,
        vector<string>{string("There are ") + to_string(TIrcd::GetInstance().GetUsersCount()) + string(" users and 0 invisible on 1 servers")}));
    responses.push_back(make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_MOTDSTART,
        vector<string>{"- Message of the day -"}));
    responses.push_back(make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_MOTD,
        vector<string>{"- This is a test motd"}));

    TIrcd::GetInstance().EnqueueMessage(move(responses));
    return;
}

void HandleJoin(unique_ptr<RequestMessage> message) {
    vector<string> params = message->GetParams();
    shared_ptr<User> sender = message->GetSender();
    unique_ptr<ResponseMessage> response;
    if (params.size() != 1) {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NEEDMOREPARAMS,
            vector<string>{"JOIN", "Not enough parameters"});
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }
    
    string channelName = params[0];
    cout << "[DEBUG] [CommandHandler] Joining channel: " << channelName << endl;
    if (channelName[0] != '#') {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NOSUCHCHANNEL,
            vector<string>{channelName, "No such channel"});
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }

    // join channel
    Channel& channel = TIrcd::GetInstance().CreateOrGetChannel(channelName);
    channel.JoinUser(sender);
    vector<unique_ptr<ResponseMessage>> responses;
    responses.push_back(make_unique<ResponseMessage>(
        sender,
        "JOIN",
        vector<string>({channelName})));
    
    // channel topic
    if (channel.HasTopic()) {
        responses.push_back(make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::RPL_TOPIC,
            vector<string>({channelName, channel.GetTopic()})));
    } else {
        responses.push_back(make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::RPL_NOTOPIC,
            vector<string>({channelName, "No topic is set"})));
    }

    // name replies
    responses.push_back(make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_NAMREPLY,
        vector<string>({channelName, channel.GetUserList()})));
    responses.push_back(make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_ENDOFNAMES,
        vector<string>({channelName, "End of users"})));

    TIrcd::GetInstance().EnqueueMessage(move(responses));
    return;
}

void HandlePart(unique_ptr<RequestMessage> message) {
    vector<string> params = message->GetParams();
    shared_ptr<User> sender = message->GetSender();
    unique_ptr<ResponseMessage> response;

    if (params.size() < 1) {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NEEDMOREPARAMS,
            vector<string>{"PART", "Not enough parameters"});
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }
    string channelName = params[0];
    cout << "[DEBUG] [CommandHandler] Leaving channel: " << channelName << endl;
    if (channelName[0] != '#') {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NOSUCHCHANNEL,
            vector<string>{channelName, "No such channel"});
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }

    // leave channel
    if (!TIrcd::GetInstance().IsChannelExists(channelName)) {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NOSUCHCHANNEL,
            vector<string>{channelName, "No such channel"});
            TIrcd::GetInstance().EnqueueMessage(move(response));
            return;
    }
    Channel& channel = TIrcd::GetInstance().CreateOrGetChannel(channelName);
    if (!channel.IsUserInChannel(sender)) {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NOTONCHANNEL,
            vector<string>({channelName, "Not on channel"}));
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }

    channel.LeaveUser(sender);
    vector<unique_ptr<ResponseMessage>> responses;
    responses.push_back(make_unique<ResponseMessage>(
        sender,
        "PART",
        vector<string>({channelName})));
    TIrcd::GetInstance().EnqueueMessage(move(responses));
    return;
}

void HandleTopic(unique_ptr<RequestMessage> message) {
    vector<string> params = message->GetParams();
    shared_ptr<User> sender = message->GetSender();
    unique_ptr<ResponseMessage> response;
    if (params.size() < 1) {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NEEDMOREPARAMS,
            vector<string>{"TOPIC", "Not enough parameters"});
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }
    string channelName = params[0];
    if (channelName[0] != '#') {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NOSUCHCHANNEL,
            vector<string>{channelName, "No such channel"});
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }

    Channel& channel = TIrcd::GetInstance().CreateOrGetChannel(channelName);

    if (!channel.IsUserInChannel(sender)) {
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_NOTONCHANNEL,
            vector<string>({channelName, "Not on channel"}));
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }

    if (params.size() == 1) {   // show topic of channel
        if (channel.HasTopic()) {
                response = make_unique<ResponseMessage>(
                sender,
                ResponseMessage::ResponseCode::RPL_TOPIC,
                vector<string>({channelName, channel.GetTopic()}));
        } else {
            response = make_unique<ResponseMessage>(
                sender,
                ResponseMessage::ResponseCode::RPL_NOTOPIC,
                vector<string>({channelName, "No topic is set"}));
        }
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    }

    // set topic of channel
    string topic = params[1];
    channel.SetTopic(topic);
    response = make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_TOPIC,
        vector<string>({channelName, topic}));

    TIrcd::GetInstance().EnqueueMessage(move(response));
    return;
}

void HandleList(unique_ptr<RequestMessage> message) {
    vector<string> params = message->GetParams();
    shared_ptr<User> sender = message->GetSender();
    vector<unique_ptr<ResponseMessage>> responses;
    vector<Channel> channelsRef = TIrcd::GetInstance().GetChannelList();

    responses.push_back(make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_LISTSTART,
        vector<string>{"Channel: Topic"}));
    
    int channelCount = 0;
    for (Channel& channel : channelsRef) {
        responses.push_back(make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::RPL_LIST,
            vector<string>({channel.GetChannelName(), to_string(channelCount++), channel.GetTopic()})));
    }

    responses.push_back(make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_LISTEND,
        vector<string>{"End of LIST"}));

    TIrcd::GetInstance().EnqueueMessage(move(responses));
    return;
}

void HandleUsers(unique_ptr<RequestMessage> message) {
    vector<string> params = message->GetParams();
    shared_ptr<User> sender = message->GetSender();
    vector<unique_ptr<ResponseMessage>> responses;
    responses.push_back(make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_USERSSTART,
        vector<string>{"UserID   Terminal"}));
    for (auto connectionMap : TIrcd::GetInstance().GetConnectionMap()) {
        shared_ptr<User> user = connectionMap.second;
        responses.push_back(make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::RPL_USERS,
            vector<string>{user->GetNickname() + "  --------  " + user->GetHostname()}));
    }
    responses.push_back(make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::RPL_ENDOFUSERS,
        vector<string>{"End of users"}));
    TIrcd::GetInstance().EnqueueMessage(move(responses));
    return;
}

void HandlePrivMsg(unique_ptr<RequestMessage> message) {
    vector<string> params = message->GetParams();
    shared_ptr<User> sender = message->GetSender();
    unique_ptr<ResponseMessage> response;
    if (params.size() < 2) {
        if (params.size() == 1) {
            response = make_unique<ResponseMessage>(
                sender,
                ResponseMessage::ResponseCode::ERR_NOTEXTTOSEND,
                vector<string>{"PRIVMSG", "No text to send"});
            TIrcd::GetInstance().EnqueueMessage(move(response));
        } else if (params.size() == 0) {
            response = make_unique<ResponseMessage>(
                sender,
                ResponseMessage::ResponseCode::ERR_NORECIPIENT,
                vector<string>{"PRIVMSG", "No recipient given"});
            TIrcd::GetInstance().EnqueueMessage(move(response));
        }
        return;
    }

    string target = params[0];
    string content = params[1];

    if (target[0] == '#') { // send to channel
        if (!TIrcd::GetInstance().IsChannelExists(target)) {
            response = make_unique<ResponseMessage>(
                sender,
                ResponseMessage::ResponseCode::ERR_NOSUCHNICK,
                vector<string>{target, "No such channel/nick"});
            TIrcd::GetInstance().EnqueueMessage(move(response));
            return;
        }

        Channel& channel = TIrcd::GetInstance().CreateOrGetChannel(target);
        unique_ptr<ResponseMessage> response = make_unique<ResponseMessage>(
            sender,
            channel.GetActiveUsers(),
            "PRIVMSG",
            vector<string>({target, ":" + content}));
        TIrcd::GetInstance().EnqueueMessage(move(response));

    } else {    // send to user
        // Not gonna implement this XD
    }
    return;
}

void HandleDefault(unique_ptr<RequestMessage> message) {
    string command = message->GetCommand();
    shared_ptr<User> sender = message->GetSender();
    unique_ptr<ResponseMessage> response;

    response = make_unique<ResponseMessage>(
        sender,
        ResponseMessage::ResponseCode::ERR_UNKNOWNCOMMAND,
        vector<string>{command, "Unknown command"});
    TIrcd::GetInstance().EnqueueMessage(move(response));
    return;
}