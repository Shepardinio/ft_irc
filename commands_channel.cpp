#include "Clients.hpp"

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

void sendNamesList(Client &client, std::string channelName, std::set<int> clientsInChannel)
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
	(void)client;
	std::istringstream iss(args);
	std::string channelName;
	std::string passw;

	iss >> channelName;
	iss >> passw;

    if (!isValidChannelName(channelName))
    {
        send_msg(client.fd, ":server 403 " + client.nickname + " " + channelName + " :No such channel\r\n");
        return;
    }
    if (!g_channels.channelExists(channelName))
        g_channels.createChannel(channelName, client.fd, passw);
    else if (!g_channels.checkPassword(channelName, passw))
    {
        send_msg(client.fd, ":server 475 " + client.nickname + " " + channelName + " :Cannot join channel (+k)\r\n");
        return;
    }
    if (!g_channels.addClientToChannel(channelName, client.fd))
        return;

    const std::set<int> &clientsInChannel = g_channels.getClientsInChannel(channelName);
    for (std::set<int>::const_iterator it = clientsInChannel.begin(); it != clientsInChannel.end(); it++)
    {
        int fd = *it;
        send_msg(fd, ":" + client.nickname + " JOIN :" + channelName + "\r\n");
    }
    sendNamesList(client, channelName, clientsInChannel);
}