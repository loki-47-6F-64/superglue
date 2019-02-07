//
// Created by loki on 29-1-19.
//

#ifndef T_MAN_PACK_H
#define T_MAN_PACK_H

#include <nlohmann/json.hpp>
#include <ice_trans.h>

std::optional<nlohmann::json> pack(const pj::remote_t &remote);
std::optional<std::vector<pj::remote_buf_t>> unpack(const nlohmann::json &json);

#endif //T_MAN_PACK_H
