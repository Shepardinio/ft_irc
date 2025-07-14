#include "Clients.hpp"

std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;

    while (std::getline(iss, token, delimiter))
        tokens.push_back(token);

    return tokens;
}

bool isValidChannelName(const std::string &name)
{
    if (name.empty() || name.length() > 200)
        return false;
    if (name[0] != '#' && name[0] != '&')
        return false;
    for (size_t i = 1; i < name.length(); i++)
    {
        if (name[i] == ' ' || name[i] == ',' || name[i] == 7 || name[i] == '\n' || name[i] == '\r' || name[i] == '\t')
            return false;
    }
    return true;
}

void sendNamesList(Client &client, std::string channelName, const std::set<int> clientsInChannel)
{
    std::string nameList;

    for (std::set<int>::const_iterator it = clientsInChannel.begin(); it != clientsInChannel.end(); it++)
    {
        const Client &chanClient = clients_bj.get_client(*it);
        if (!nameList.empty())
            nameList += " ";
        if (g_channels.isOperator(channelName, *it))
            nameList += "@";
        nameList += chanClient.nickname;
    }
    send_msg(client.fd, ":server 353 " + client.nickname + " = " + channelName + " :" + nameList + "\r\n");
    send_msg(client.fd, ":server 366 " + client.nickname + " " + channelName + " :End of NAMES list\r\n");
}

void join(Client &client, std::string args)
{
    std::string channelsStr, keysStr;
    std::istringstream iss(args);
    iss >> channelsStr >> keysStr;

    std::vector<std::string> channels = split(channelsStr, ',');
    std::vector<std::string> keys = split(keysStr, ',');

    if (channels.empty() || channels[0].empty())
    {
        send_msg(client.fd, ":server 461 " + client.nickname + " JOIN :Not enough parameters\r\n");
        return;
    }

    for (size_t i = 0; i < channels.size(); ++i)
    {
        const std::string &channelName = channels[i];
        const std::string passw = (i < keys.size()) ? keys[i] : "";

        if (!isValidChannelName(channelName))
        {
            send_msg(client.fd, ":server 403 " + client.nickname + " " + channelName + " :No such channel\r\n");
            continue;
        }

        if (!g_channels.channelExists(channelName))
            g_channels.createChannel(channelName, client.fd, passw);
        else if (!g_channels.checkPassword(channelName, passw))
        {
            send_msg(client.fd, ":server 475 " + client.nickname + " " + channelName + " :Cannot join channel (+k)\r\n");
            continue;
        }
        else if (g_channels.hasMode(channelName, 'i'))
        {
            send_msg(client.fd, ":server 473 " + client.nickname + " " + channelName + " :Cannot join channel (+i)\r\n");
            continue;
        }
        else
        {
            if (!g_channels.addClientToChannel(channelName, client.fd))
            {
                // send_msg(client.fd, ":server 443 " + client.nickname + " " + channelName + " :is already on channel\r\n"); is not in numeric repli
                continue;
            }
        }
        
        const std::set<int> &clientsInChannel = g_channels.getClientsInChannel(channelName);
        for (std::set<int>::const_iterator it = clientsInChannel.begin(); it != clientsInChannel.end(); ++it)
        {
            int fd = *it;
            send_msg(fd, ":" + client.nickname + " JOIN :" + channelName + "\r\n");
        }

        std::string topic = g_channels.getTopic(channelName);
        if (!topic.empty())
            send_msg(client.fd, ":server 332 " + client.nickname + " " + channelName + " :" + topic + "\r\n");

        sendNamesList(client, channelName, clientsInChannel);
    }
}

void topic(Client &client, std::string args)
{
    std::istringstream iss(args);
    std::string channelName;
    std::string newTopic;

    iss >> channelName;
    std::getline(iss, newTopic);
    if (!newTopic.empty() && newTopic[0] == ' ')
        newTopic.erase(0, 1);

    if (channelName.empty())
    {
        send_msg(client.fd, ":server 461 " + client.nickname + " TOPIC :Not enough parameters\r\n");
        return;
    }
    if (!g_channels.isClientInChannel(channelName, client.fd))
    {
        send_msg(client.fd, ":server 442 " + client.nickname + " " + channelName + " :You're not on that channel\r\n");
        return;
    }
    if (g_channels.hasMode(channelName, 't') && !g_channels.isOperator(channelName, client.fd) && !newTopic.empty())
    {
        send_msg(client.fd, ":server 482 " + client.nickname + " " + channelName + " :You're not channel operator\r\n");
        return;
    }
    if (!newTopic.empty())
    {
        g_channels.setTopic(channelName, newTopic);
        const std::set<int> &clients = g_channels.getClientsInChannel(channelName);

        for (std::set<int>::const_iterator it = clients.begin(); it != clients.end(); ++it)
            send_msg(*it, ":" + client.nickname + " TOPIC " + channelName + " :" + newTopic + "\r\n");
    }
    else
        if(!g_channels.hasTopic(channelName))
            send_msg(client.fd, ":server 331 " + client.nickname + " " + channelName + " :No topic is set\r\n");
        else
            send_msg(client.fd, ":server 332 " + client.nickname + " " + channelName + " :" + g_channels.getTopic(channelName) + "\r\n");
}

// void invite(Client &client, std::string args)
// {
//     std::istringstream iss(args);
//     std::string nick;
//     std::string channelName;
//     int target_fd = 0;

//     iss >> nick >> channelName;

//     if (nick.empty() || channelName.empty())
//        return send_msg(client.fd, ":server 461 " + client.nickname + " INVITE :Not enough parameters\r\n");

//     if (!g_channels.channelExists(channelName))
//         return send_msg(client.fd, ":server 401 " + client.nickname + " " + nick + " :No such channel\r\n");

//     if (!g_channels.isClientInChannel(channelName, client.fd))
//         return send_msg(client.fd, ":server 442 " + client.nickname + " " + channelName + " :You're not on that channel\r\n");

//     if (g_channels.hasMode(channelName, 'i') && !g_channels.isOperator(channelName, client.fd))
//         return send_msg(client.fd, ":server 482 " + client.nickname + " " + channelName + " :You're not channel operator\r\n");
    
//     if (clients_bj.nickExists(nick))
//         target_fd = clients_bj.get_fd_of(nick);
//     else {
//         return send_msg(client.fd, ":server 401 " + client.nickname + " " + nick + " :No such nick\r\n");
//     }

//     if (g_channels.isClientInChannel(channelName, target_fd))
//         return send_msg(client.fd, ":server 443 " + client.nickname + " " + nick + " " + channelName + " :is already on channel\r\n");
    
//     g_channels.addClientToChannel(channelName, target_fd);
//     send_msg(client.fd, ":server 341 " + client.nickname + " " + nick + " " + channelName + "\r\n");
//     send_msg(target_fd, ":" + client.nickname + " INVITE " + nick + " :" + channelName + "\r\n");
// }

/*
JOIN
403 ERR_NOSUCHCHANNEL  "No such channel"
461 ERR_NEEDMOREPARAMS "Not enough parameters"
471 ERR_CHANNELISFULL  "Cannot join channel (+l)" manque lui
473 ERR_INVITEONLYCHAN "Cannot join channel (+i)"
475 ERR_BADCHANNELKEY  "Cannot join channel (+k)"

405 ERR_TOOMANYCHANNELS "You have joined too many channels" optionnal


TOPIC
331 No topic is set
send_msg(client.fd, ":server 331 " + client.nickname + " " + channelName + " :No topic is set\r\n");
332 <topic>
send_msg(client.fd, ":server 332 " + client.nickname + " " + channelName + " :" + topic + "\r\n");
442 You're not on that channel
send_msg(client.fd, ":server 442 " + client.nickname + " " + channelName + " :You're not on that channel\r\n");
461 Not enough parameters
send_msg(client.fd, ":server 461 " + client.nickname + " TOPIC :Not enough parameters\r\n");
482 You're not channel operator
send_msg(client.fd, ":server 482 " + client.nickname + " " + channelName + " :You're not channel operator\r\n");

INVITE
401 ERR_NOSUCHNICK       "<nickname> :No such nick/channel"           X
443 ERR_USERONCHANNEL    "<user> <channel> :is already on channel"    X
442 ERR_NOTONCHANNEL     "<channel> :You're not on that channel"      x
461 ERR_NEEDMOREPARAMS   "<command> :Not enough parameters"           x
482 ERR_CHANOPRIVSNEEDED "<channel> :You're not channel operator"     x
301 RPL_AWAY             "<nick> :<away message>"                     optionnal
341 RPL_INVITING         "<channel> <nick>"                           x

KICK
403 ERR_NOSUCHCHANNEL    "<channel name> :No such channel"
442 ERR_NOTONCHANNEL     "<channel> :You're not on that channel"
461 ERR_NEEDMOREPARAMS   "<command> :Not enough parameters"
482 ERR_CHANOPRIVSNEEDED "<channel> :You're not channel operator"

exemple: send_msg(client.fd, ":" SERVER_NAME " 401 " + client.nickname + " " + reciever + " :No such nick/channel\r\n");

probleme quand /join j'ai la fonction invite
*/