#include "handlers/command_handler.h"
#include "tircd.h"

#include <iostream>


CommandHander::CommandHander() {
}

CommandHander::~CommandHander() {
}

void CommandHander::HandleMessage(unique_ptr<RequestMessage> message) {
    string command = message->GetCommand();
    vector<string> params = message->GetParams();
    shared_ptr<User> sender = message->GetSender();

    cout << "[DEBUG] [CommandHandler] Handling command: " << command << " ";
    for (auto param : params) {
        cout << param << " ";
    }
    cout << endl;

    unique_ptr<ResponseMessage> response;
    if (command == "NICK") {
        if (params.size() != 1) {
            response = make_unique<ResponseMessage>(
                sender,
                ResponseMessage::ResponseCode::ERR_NEEDMOREPARAMS,
                vector<string>{"NICK", "Not enough parameters"});
            TIrcd::GetInstance().EnqueueMessage(move(response));
            return;
        }
        sender->SetNickname(params[0]);
        return;
    } else if (command == "USER") {
        if (params.size() != 4) {
            response = make_unique<ResponseMessage>(
                sender,
                ResponseMessage::ResponseCode::ERR_NEEDMOREPARAMS,
                vector<string>{"USER", "Not enough parameters"});
            return;
        }
        
        sender->SetUsername(params[0]);
        sender->SetRealname(params[3]);
        // send welcome message and motd
        vector<unique_ptr<ResponseMessage>> responses;
        responses.push_back(make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::RPL_WELCOME,
            vector<string>{"Welcome to the Internet Relay Network " + sender->GetUsername() + "! "}));
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
    } else if (command == "JOIN") {
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
    } else if (command == "PART") {
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
        Channel& channel = TIrcd::GetInstance().CreateOrGetChannel(channelName);
        channel.LeaveUser(sender);
        response = make_unique<ResponseMessage>(
            sender,
            "PART",
            vector<string>({":" + channelName}));
        TIrcd::GetInstance().EnqueueMessage(move(response));
        return;
    } else if (command == "TOPIC") {
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
        unique_ptr<ResponseMessage> response;
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
    } else if (command == "LIST") {
        vector<Channel> channelsRef = TIrcd::GetInstance().GetChannelList();
        vector<unique_ptr<ResponseMessage>> responses;

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
    } else if (command == "USERS") {
        vector<unique_ptr<ResponseMessage>> responses;
        responses.push_back(make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::RPL_USERSSTART,
            vector<string>{"UserID   Terminal  Host"}));
        for (auto connectionMap : TIrcd::GetInstance().GetConnectionMap()) {
            shared_ptr<User> user = connectionMap.second;
            responses.push_back(make_unique<ResponseMessage>(
                sender,
                ResponseMessage::ResponseCode::RPL_USERS,
                vector<string>{user->GetNickname() + "  -------- " + user->GetHostname()}));
        }
        responses.push_back(make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::RPL_ENDOFUSERS,
            vector<string>{"End of users"}));
        TIrcd::GetInstance().EnqueueMessage(move(responses));
    } else if (command == "PRIVMSG") {
        if (params.size() < 2) {
            response = make_unique<ResponseMessage>(
                sender,
                ResponseMessage::ResponseCode::ERR_NEEDMOREPARAMS,
                vector<string>{"PRIVMSG", "Not enough parameters"});
            TIrcd::GetInstance().EnqueueMessage(move(response));
            return;
        }

        string target = params[0];
        string message = params[1];

        if (target[0] == '#') { // send to channel
            Channel& channel = TIrcd::GetInstance().CreateOrGetChannel(target);
            
            unique_ptr<ResponseMessage> response = make_unique<ResponseMessage>(
                sender,
                channel.GetActiveUsers(),
                "PRIVMSG",
                vector<string>({target, ":" + message}));
            TIrcd::GetInstance().EnqueueMessage(move(response));

        } else {    // send to user
            // Not gonna implement this XD
        }
        
        return;
    } else { // Unknown command
        
        response = make_unique<ResponseMessage>(
            sender,
            ResponseMessage::ResponseCode::ERR_UNKNOWNCOMMAND,
            vector<string>{command, "Unknown command"});
        TIrcd::GetInstance().EnqueueMessage(move(response));
    }

    return;
}