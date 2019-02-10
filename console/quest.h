//
// Created by loki on 8-2-19.
//

#ifndef T_MAN_QUEST_H
#define T_MAN_QUEST_H

#include <map>
#include <variant>
#include <kitty/file/io_stream.h>
#include <ice_trans.h>
#include "uuid.h"

void send_register(file::io &server);

pj::ICETrans &ice_trans();
uuid_t &uuid();

std::map<uuid_t, file::io> &peers();
file::poll_t<file::io, std::monostate> &poll();

#endif //T_MAN_QUEST_H
