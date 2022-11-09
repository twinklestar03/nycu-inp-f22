#pragma once
#include "messages.h"

#include <memory>

using namespace std;


class CommandHander {
public:
    CommandHander();
    ~CommandHander();

    void HandleMessage(unique_ptr<RequestMessage> message);
};