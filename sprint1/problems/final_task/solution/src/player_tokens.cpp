#include "player_tokens.h"
#include "request_handler_helper.h"

#include <sstream>
#include <iostream>

namespace model {

Token PlayerTokens::AddPlayer(Player&& player) {
    std::stringstream buffer;

    auto part1 = generator1_();
    auto part2 = generator2_();
    buffer << std::hex << part1 << part2;

    std::string token_str;
    token_str += buffer.str();
    while (token_str.size() < 32) {
        token_str.insert(0, "0");
    }
    // if (token_str.size() == 30) {
    //     token_str.insert(0, "00");
    // } else if (token_str.size() == 31) {
    //     token_str.insert(0, "0");
    // }

    auto result = Token(std::move(token_str));
    
    tokenToPlayer_.emplace(result, player.GetId());
    players_.emplace_back(std::move(player));
    tokens_.push_back(result);
    return result;
}

const Player& PlayerTokens::FindPlayerByToken(const Token& token) const {
    if ((*token).size() != 32) {
        throw http_handler::InvalidTokenException("Invalid token size");
    }

    if (tokenToPlayer_.contains(token)) {
        auto id = tokenToPlayer_.at(token);
        return players_[id];
    } else {
        throw http_handler::UnknownTokenException("Player token has not been found");
    }
}

const Player &PlayerTokens::FindPlayerById(uint32_t id) const {
    if (id >= tokens_.size()) {
        throw http_handler::InvalidArgumentException("Invalid player id");
    }
    auto token = tokens_[id];
    return FindPlayerByToken(token);
}

const Token &PlayerTokens::FindTokenByPlayerId(uint32_t id) {
    if (id >= tokens_.size()) {
        throw http_handler::InvalidArgumentException("Invalid player id");
    }
    return tokens_[id];
}

const std::vector<Player> &PlayerTokens::GetPlayers() const {
    return players_;
}

}