#include "User.h"

User::User() : currentSocket_(INVALID_SOCKET) {}

User::User(std::string id, SOCKET currentSocket)
    : id_(std::move(id)), currentSocket_(currentSocket) {}

User::~User() = default;

std::string User::getId() {
    return this->id_;
}

SOCKET User::getCurrentSocket() const {
    return this->currentSocket_;
}

void User::setCurrentSocket(const SOCKET currentSocket) {
    this->currentSocket_ = currentSocket;
}

bool User::isConnected() const {
    return this->isConnected_;
}

void User::setConnected(const bool connected) {
    this->isConnected_ = connected;
}




