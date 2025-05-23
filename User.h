#ifndef USER_H
#define USER_H
#include <string>
#include <winsock2.h>


class User {
public:
    User();
    User(std::string id, SOCKET currentSocket);
    ~User();
    std::string getId();
    [[nodiscard]] SOCKET getCurrentSocket() const;
    void setCurrentSocket(SOCKET currentSocket);
    [[nodiscard]] bool isConnected() const;
    void setConnected(bool connected);


private:
    std::string id_;
    SOCKET currentSocket_;
    bool isConnected_ = true;
};



#endif //USER_H
