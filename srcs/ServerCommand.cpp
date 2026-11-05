/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommand.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: diespino <diespino@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 18:51:17 by diespino          #+#    #+#             */
/*   Updated: 2026/11/05 20:01:50 by diespino         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

void	Server::welcomeMsg(Client& client, Channel* chan) {

//	Flata hacere prefix de client --> :nick!user@hostname
//	msg << clientPrefix(client) << " JOIN " << chan->getName();
//	Enviar mensaje de JOINED a todos los clientes (incluido al nuevo miembro) del CHANNEL
//	Hace falta mandar tambien TOPIC y los nombres de los integrante del CHANNEL???

	std::ostringstream	msg;

	msg << ":nick!user@hostname" << " JOIN " << chan->getName();
	broadcast( *chan, msg.str(), -1 );
//
	if (chan->hasTopic())
		ErrChannel(client, chan->getName(), 332, chan->getTopic());
	else
		ErrChannel(client, chan->getName(), 331, RPL_NOTOPIC);
//	
	std::ostringstream      names;
	const std::set<int>&	members = chan->getMembers();
	for (std::set<int>::const_iterator it = members.begin();
			it != members.end(); it++)
	{
		std::map<int, Client*>::iterator clientIt = _clients.find(*it);
		if (clientIt == _clients.end())
			continue ;
		Client* member = clientIt->second;
		if (chan->isOperator(*it))
			names << "@";
		names << member->getNick() << " ";
	}
	ErrChannel(client, chan->getName(), 353, names.str());
	ErrChannel(client, chan->getName(), 366, RPL_ENDOFNAMES);
}

void	Server::handleJoin(Client& client, const parsedCmd& cmd) {

//	if (!client.isRegistered())
//	{
//		ErrClient(client, 451, ERR_NOTREGISTERED);
//		return ;
//	}
	if (cmd.params.empty() || cmd.params[0].empty())
	{
		ErrCmd(client, 461, cmd.cmd, ERR_NEEDMOREPARAMS);
		return ;
	}
	if (cmd.params[0].size() < 2 || cmd.params[0][0] != '#')
	{
		ErrChannel(client, cmd.params[0], 403, ERR_NOSUCHCHANNEL);
		return ;
	}

	Channel* chan = getChannel(cmd.params[0]);

	if (chan != NULL && chan->isMember(client.getFd()))
		return ;
	if ( chan == NULL )
	{
		chan = createChannel(cmd.params[0]);
		if (chan == NULL)
		{
//			MSG_ERROR();
			return ;
		}
		std::ostringstream	msg;
		
		chan->addMember(client.getFd());
		chan->addOperator(client.getFd());
		welcomeMsg(client, chan);
		return ;
	}
	if (chan->hasKey() && (cmd.params.size() < 2 || cmd.params[1] != chan->getKey()))
	{
			ErrChannel(client, chan->getName(), 475, ERR_BADCHANNELKEY);
			return ;
	}
	if (chan->isInviteOnly() && !chan->isInvited(client.getFd()))
	{
			ErrChannel(client, chan->getName(), 473, ERR_INVITEONLYCHAN);
			return ;
	}
	if (chan->getLimit() != 0 && chan->memberCount() >= chan->getLimit())
	{
		ErrChannel(client, chan->getName(), 471, ERR_CHANNELISFULL);
		return ;
	}
	if (!chan->isMember(client.getFd()))
	{
		chan->addMember(client.getFd());
		welcomeMsg(client, chan);
	}
	if (chan->isInvited(client.getFd()))
		chan->removeInvite(client.getFd());
	return ;
}

void	Server::handleTopic(Client& client, const parsedCmd& cmd) {

//	if (!client.isRegistered())
//	{
//		ErrClient(client, 451, ERR_NOTREGISTERED);
//		return ;
//	}
	if (cmd.params.empty() || cmd.params[0].empty() || cmd.params.size() > 2)
	{
		ErrCmd(client, 461, cmd.cmd, ERR_NEEDMOREPARAMS);
		return ;
	}
	if (cmd.params[0].size() < 2 || cmd.params[0][0] != '#')
	{
		ErrChannel(client, cmd.params[0], 403, ERR_NOSUCHCHANNEL);
		return ;
	}

	Channel* chan = getChannel(cmd.params[0]);
	
	if (chan == NULL)
	{
		ErrChannel(client, cmd.params[0], 403, ERR_NOSUCHCHANNEL);
		return ;
	}
	if (!chan->isMember(client.getFd()))
	{
		ErrChannel(client, cmd.params[0], 442, ERR_NOTONCHANNEL);
		return ;
	}
	if (cmd.params.size() == 1)
	{
		if (chan->hasTopic())
		{
			ErrChannel(client, chan->getName(), 332, chan->getTopic());
			return ;
		}
		ErrChannel(client, chan->getName(), 331, RPL_NOTOPIC);
		return ;
	}
	if (cmd.params.size() == 2)
	{
		if (chan->isTopicRestricted() && !chan->isOperator(client.getFd()))
		{
			ErrChannel(client, chan->getName(), 482, ERR_CHANOPRIVSNEEDED);
			return ;
		}
		std::ostringstream	msg;

		chan->setTopic(cmd.params[1]);

		msg << ":nick!user@hostname" << " TOPIC " << chan->getName() << " :" << chan->getTopic();
		broadcast( *chan, msg.str(), -1 );
		return ;
	}
}
/*
void	Server::handleTopic(Client& client, const parsedCmd& cmd) {

//	if (!client.isRegistered())
//	{
//		ErrClient(client, 451, ERR_NOTREGISTERED);
//		return ;
//	}
	if (cmd.params.empty() || cmd.params[0].empty() || cmd.params.size() < 1)
	{
		ErrCmd(client, 461, cmd.cmd, ERR_NEEDMOREPARAMS);
		return ;
	}
	if (cmd.params[0].size() < 2 || cmd.params[0][0] != '#')
	{
		ErrChannel(client, cmd.params[0], 403, ERR_NOSUCHCHANNEL);
		return ;
	}

	Channel* chan = getChannel(cmd.params[0]);
	
	if (chan == NULL)
	{
		ErrChannel(client, cmd.params[0], 403, ERR_NOSUCHCHANNEL);
		return ;
	}
	if (!chan->isMember(client.getFd()))
	{
		ErrChannel(client, cmd.params[0], 442, ERR_NOTONCHANNEL);
		return ;
	}
//
//	MODE #42 +kol secret ALICE 10
//
//	cmd.cmd	      --> COMMAND --> MODE
//	cmd.params[0] --> CHANNEL --> #42
//	cmd.params[1] --> MODE    --> +kol (key|operator|limit)
//	cmd.params[2] --> ARGS    --> secret (key)
//	cmd.params["] --> ARGS    --> ALICE
//	cmd.params["] --> ARGS    --> 10
//
//	if (cmd.paramas.size() == 1) { Mostrar modos actuales del Channel }
	else if (cmd.paramas.size() >= 2 && chan->isOperator(client.getFd()))
	{
		int count = 1;

		for (int i = 0; i < cmd.paramas[1].size(); i++)
		{
			bool	op = true;// '+'

			if (cmd.paramas[i] == '+' || cmd.paramas[i] == '-')
			{
				if (cmd.paramas[i] == '-')
					op = false;// '-'
				i++;
			}
			if (cmd.paramas[i] == 'i')
			{
				if (!op)
					chan->setInviteOnly(false);
				else
					chan->setInviteOnly(true);
			}
			else if (cmd.paramas[i] == 't')
			{
				if (!op)
					chan->setTopicRestricted(false);
				else
					chan->setTopicRestricted(true);
			}
			else if (cmd.paramas[i] == 'k')
			{
//				Se debe introducir la anterior contrasena??
				if (op)
				{
					count++;
					chan->setKey(cmd.params[count]);
				}
				else
					chan->setKey("");
			}
//			else if (cmd.paramas[i] == 'o')
//			{
//				if (op)
//				{
//					count++;
//				}
//				else
//					chan->setKey("");
//			}
			else if (cmd.paramas[i] == 'l')
			{
				if (op)
				{
					count++;
					for(int j = 0; j < cmd.params[count].size(); j++)
					{
						if (!std::isdigit(cmd.params[count][j]))
							return ;// MSG_ERROR
					}
					std::stringstream	ss(cmd.params[count]);
					size_t			limit;

					ss >> limit;
					chan->setlimit(limit);
				}
				else
					chan->setlimit(0);
			}
//			si no es '+'|'-'|'i'|'t'|'k'|'o'|'l' --> MSG_ERROR + return ;
		} 
	}
	ErrChannel(client, chan->getName(), 482, ERR_CHANOPRIVSNEEDED);
	return ;
}*/

parsedCmd	Server::parseCommand(const std::string &line) {

	std::istringstream	input(line);
	std::string		token;
	parsedCmd		command;

	input >> command.cmd;

	while (input >> token)
	{
		if (token[0] == ':')
		{
			std::string	trailing = token.substr(1);
			std::string	tmp;

			std::getline(input, tmp);
			command.params.push_back(trailing + tmp);
			break ;
		}
		command.params.push_back(token);
	}
	return (command);
}

void    Server::handleCommand(Client& client, const std::string& line)
{
	parsedCmd	cmd = parseCommand(line);

//	Mientras no haya USER, NICK Y PASS comentar lineas 199 <--> 204 para probar CMDs
//	Tambien hacer con comandos como JOIN que comprueban si client.isRegistered()
//
//	if (cmd.cmd != "PASS" && cmd.cmd != "NICK" && cmd.cmd != "USER"
//			&& cmd.cmd != "QUIT" && !client.isRegistered())
//	{
//		ErrClient(client, 451, ERR_NOTREGISTERED);
//		return ;
//	}
	if (cmd.cmd == "JOIN")
	{
		handleJoin(client, cmd);
		return ;
	}
	if (cmd.cmd == "TOPIC")
	{
		handleTopic(client, cmd);
		return ;
	}
//	if (cmd.cmd == "MODE")
//	{
//		handleMode(client, cmd);
//		return ;
//	}
	
	if ( cmd.cmd == "QUIT" )// No hecho solo test
	{
		reply( client, "ERROR :Closing link" );
		client.markForDisconnect();
		return ;
	}
	reply( client, "echo: " + line );
}

