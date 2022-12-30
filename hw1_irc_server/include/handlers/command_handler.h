#pragma once
#include "messages.h"

#include <map>
#include <memory>

using namespace std;


typedef void (*CommandHandlerFunction)(unique_ptr<RequestMessage>);

class CommandHandler {
public:
    CommandHandler();
    ~CommandHandler();

    void HandleMessage(unique_ptr<RequestMessage> message);
private: 
    map<string, CommandHandlerFunction> commandMap;
};