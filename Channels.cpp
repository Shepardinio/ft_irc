#include "Channels.hpp"

bool Channels::channelExists(const std::string &name) const
{
    return _channels.find(name) != _channels.end();
}

bool Channels::createChannel(const std::string &name, int creatorFD, const std::string &password)
{
    if (this->channelExists(name))
        return false;
    
    Channel newChan;
    newChan.name = name;
    newChan.topic = "";
    newChan.password = password;

    newChan.clients.insert(creatorFD);
    newChan.operators.insert(creatorFD);

    _channels[name] = newChan;
    return true;
}

bool Channels::addClientToChannel(const std::string &name, int clientFd)
{
    if (this->_channels[name].clients.count(clientFd))
        return false;
    this->_channels[name].clients.insert(clientFd);
    return true;
}

void Channels::removeClientFromChannel(const std::string &name, int clientFd)
{
    this->_channels[name].clients.erase(clientFd);
    this->_channels[name].operators.erase(clientFd);

    if (this->_channels[name].clients.empty())
        this->_channels.erase(name);
}

void Channels::addOperator(const std::string &name, int clientFd)
{
        this->_channels[name].operators.insert(clientFd);
}

void Channels::removeOperator(const std::string &name, int clientFd)
{
    this->_channels[name].operators.erase(clientFd);
}

bool Channels::isOperator(const std::string &name, int clientFd) const
{
    std::map<std::string, Channel>::const_iterator it = this->_channels.find(name);
    if (it ==  this->_channels.end())
        return false;
    return it->second.operators.count(clientFd) != 0;
}

bool Channels::checkPassword(const std::string &name, const std::string &pass) const
{
    std::map<std::string, Channel>::const_iterator it = this->_channels.find(name);
    if (it == _channels.end())
        return false;
    if (it->second.password == pass)
        return true;
    return false;
}

const std::set<int> &Channels::getClientsInChannel(const std::string &name) const
{
    static const std::set<int> empty;
    std::map<std::string, Channel>::const_iterator it = this->_channels.find(name);
    if (it != this->_channels.end())
        return it->second.clients;
    return empty;
}