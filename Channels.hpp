#ifndef CHANNELS_HPP
#define CHANNELS_HPP

#include "Clients.hpp"
#include <set>

struct Channel
{
    std::string name;
    std::string topic;
    std::set<int> clients;
    std::set<int> operators;
    std::string password;
};

class Channels
{
    private:
        std::map<std::string, Channel> _channels;

    public:
        bool channelExists(const std::string &name) const;
        bool createChannel(const std::string &name, int creatorFD, const std::string &password = "");
        bool addClientToChannel(const std::string &name, int clientFd);
        void removeClientFromChannel(const std::string &name, int clientFd);
        // std::vector<std::string> getChannelsOfClient(int clientFd) const;
        void addOperator(const std::string &name, int clientFd);
        void removeOperator(const std::string &name, int clientFd);
        bool isOperator(const std::string &name, int clientFd) const;
        bool checkPassword(const std::string &name, const std::string &pass) const;

        const std::set<int> &getClientsInChannel(const std::string &name) const;
};

extern Channels g_channels;

#endif