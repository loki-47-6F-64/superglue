//
// Created by loki on 29-1-19.
//

#ifndef T_MAN_PACK_H
#define T_MAN_PACK_H

#include <nlohmann/json.hpp>
#include <ice_trans.h>
#include "uuid.h"

std::optional<nlohmann::json> pack_remote(const pj::remote_t &remote);
std::vector<uuid_t> unpack_peers(const nlohmann::json &json);
std::optional<pj::remote_buf_t> unpack_remote(const nlohmann::json &json);

#endif //T_MAN_PACK_H
