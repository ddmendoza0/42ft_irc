/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommand.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: diespino <diespino@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 18:51:17 by diespino          #+#    #+#             */
/*   Updated: 2026/11/09 19:42:30 by diespino         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

std::string	Server::clientPrefix(Client& client) {

	std::ostringstream      prefix;

	prefix << ":" << client.getNick() << "!" << client.getUser() << "@" << client.getHostname();
	return (prefix.str());
}

void	Server::welcomeMsg(Client& client, Channel* chan) {

	std::ostringstream	msg;

	msg << clientPrefix(client) << " JOIN " << chan->getName();
	broadcast( *chan, msg.str(), -1 );

	if (chan->hasTopic())
		ErrChannel(client, chan->getName(), 332, chan->getTopic());
	else
		ErrChannel(client, chan->getName(), 331, RPL_NOTOPIC);
	
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

bool Server::isValidNick(const std::string& nick)
{
	if (nick.empty())
		return false;
	if (nick[0] == '#' || nick[0] == ':')
		return false;
//	Debe rechazar (\r, \n, :, #, &) ???
//	Longitud de NICK ???
	if (nick.find_first_of(" ,*?!@.") != std::string::npos)
		return false;
	return true;
}

void Server::checkRegistration(Client& client)
{
//	!client.getRealname().empty() ??
	if (!client.isRegistered() && client.isPassOk() 
		&& !client.getNick().empty() && !client.getUser().empty())
	{
		client.setRegistered(true);

		std::string nick = client.getNick();
		std::string user = client.getUser();
		std::string host = client.getHostname();

		reply(client, ":irc.serv 001 " + nick + " :Welcome to the Internet Relay Network " + nick + "!" + user + "@" + host);
		reply(client, ":irc.serv 002 " + nick + " :Your host is irc.serv, running version 1.0");
		reply(client, ":irc.serv 003 " + nick + " :This server was created today");
		reply(client, ":irc.serv 004 " + nick + " irc.serv 1.0 o itkol");
	}
}

void Server::handleUser(Client& client, parsedCmd& cmd)
{
	if (client.isRegistered())
	{
		ErrClient(client, 462, ERR_ALREADYREGISTRED);
		return ;
	}

//	USER exige 4 parámetros: <username> <hostname> <servername> :<realname>
//	Se debeb comprobar el realname para completar la registration??
	if (cmd.params.size() < 4 || cmd.params[0].empty() || cmd.params[3].empty())
	{
		ErrCmd(client, 461, cmd.cmd, ERR_NEEDMOREPARAMS);
		return ;
	}

	client.setUser(cmd.params[0]);
	client.setRealname(cmd.params[3]);

	checkRegistration(client);
}

void Server::handleNick(Client& client, parsedCmd& cmd)
{
	if (cmd.params.empty() || cmd.params[0].empty())
	{
		ErrClient(client, 431, ERR_NONICKNAMEGIVEN);
		return ;
	}

	std::string newNick = cmd.params[0];

	if (!isValidNick(newNick))
	{
		ErrClient(client, 432, " " + newNick + ERR_ERRONEUSNICKNAME);
		return ;
	}

	Client* existing = getClientByNick(newNick);
	if (existing != NULL && existing->getFd() != client.getFd())
	{
		ErrClient(client, 433, " " + newNick + ERR_NICKNAMEINUSE);
		return ;
	}

	std::string oldNick = client.getNick();
	client.setNick(newNick);

//	Si el cliente ya estaba registrado y cambia el nick en mitad de la sesión,
//	el servidor debe avisarle a él mismo para que su cliente IRC actualice la interfaz.
//
//	Debe mandar aviso a todos los CHANNELs que pertenece
//
	if (client.isRegistered())
	{
		std::string msg = ":" + oldNick + " NICK :" + newNick;
		reply(client, msg);
	}
	else
		checkRegistration(client);
}

void    Server::handlePass(Client& client, parsedCmd& cmd)
{
//	Si el cliente aún no tiene nick, el estándar indica que se use "*" en las respuestas
	std::string nick = client.getNick().empty() ? "*" : client.getNick();

	if (client.isRegistered() || client.isPassOk())
	{
		reply(client, ":irc.serv 462 " + nick + " :Unauthorized command (already registered)");
		return ;
	}

	if (cmd.params.empty())
	{
//		ErrCmd(client, 461, cmd.cmd, ERR_NEEDMOREPARAMS);
		reply(client, ":irc.serv 461 " + nick + " PASS :Not enough parameters");
		return ;
	}

	if (cmd.params[0] == _pwd)
	{
		client.setPassOk(true);
	}
	else
	{
		reply(client, ":irc.serv 464 " + nick + " :Password incorrect");
		client.markForDisconnect();
	}
}

void	Server::handleJoin(Client& client, const parsedCmd& cmd) {

	if (!client.isRegistered())
	{
		ErrClient(client, 451, ERR_NOTREGISTERED);
		return ;
	}
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

void	Server::handlePart(Client& client, const parsedCmd& cmd) {

	if (!client.isRegistered())
	{
		ErrClient(client, 451, ERR_NOTREGISTERED);
		return ;
	}
	if (cmd.params.size() > 2 || cmd.params.size() < 1 || cmd.params[0].empty())
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

	std::ostringstream	msg;
	
	msg << clientPrefix(client) << " PART " << chan->getName();
	
	if (cmd.params.size() == 2 && !cmd.params[1].empty())
		msg << " :" << cmd.params[1];
	broadcast( *chan, msg.str(), -1 );

	chan->removeMember(client.getFd());
	deleteChannel(chan->getName());
}

void	Server::handleInvite(Client& client, const parsedCmd& cmd) {

	if (!client.isRegistered())
	{
		ErrClient(client, 451, ERR_NOTREGISTERED);
		return ;
	}
	if (cmd.params.size() != 2 || cmd.params[0].empty() || cmd.params[1].empty())
	{
		ErrCmd(client, 461, cmd.cmd, ERR_NEEDMOREPARAMS);
		return ;
	}
	if (cmd.params[1].size() < 2 || cmd.params[1][0] != '#')
	{
		ErrChannel(client, cmd.params[1], 403, ERR_NOSUCHCHANNEL);
		return ;
	}

	Channel* chan = getChannel(cmd.params[1]);
	
	if (chan == NULL)
	{
		ErrChannel(client, cmd.params[1], 403, ERR_NOSUCHCHANNEL);
		return ;
	}
	if (!chan->isMember(client.getFd()))
	{
		ErrChannel(client, cmd.params[1], 442, ERR_NOTONCHANNEL);
		return ;
	}
	if (chan->isInviteOnly() && !chan->isOperator(client.getFd()))
	{
		ErrChannel(client, chan->getName(), 482, ERR_CHANOPRIVSNEEDED);
		return ;
	}

	Client*	target = getClientByNick(cmd.params[0]);
	
	if (target == NULL)
	{
		ErrCmd(client, 401, cmd.params[0], ERR_NOSUCHNICK);
		return ;
	}
	if (chan->isMember(target->getFd()))
	{
		ErrClientChannel(client, chan->getName(), cmd.params[0], 443);
		return ;
	}
	chan->addInvite(target->getFd());
	
	std::ostringstream	target_msg;
	std::ostringstream	msg;
	
	target_msg << clientPrefix(client) << " INVITE " << target->getNick() << " " << chan->getName();
	msg << ":irc.serv 341 " << client.getNick() << " " << target->getNick() << " " << chan->getName();

	reply(*target, target_msg.str());
	reply(client, msg.str());
}

void	Server::handleKick(Client& client, const parsedCmd& cmd) {

	if (!client.isRegistered())
	{
		ErrClient(client, 451, ERR_NOTREGISTERED);
		return ;
	}
	if (cmd.params.size() > 3 || cmd.params.size() < 2 || cmd.params[0].empty() || cmd.params[1].empty())
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
	if (!chan->isOperator(client.getFd()))
	{
		ErrChannel(client, chan->getName(), 482, ERR_CHANOPRIVSNEEDED);
		return ;
	}

	Client*	target = getClientByNick(cmd.params[1]);
	
	if (target == NULL)
	{
		ErrCmd(client, 401, cmd.params[1], ERR_NOSUCHNICK);
		return ;
	}
	if (!chan->isMember(target->getFd()))
	{
		ErrClientChannel(client, chan->getName(), cmd.params[1], 441);
		return ;
	}
	
	std::ostringstream	msg;
	
	msg << clientPrefix(client) << " KICK " << chan->getName() << " " << target->getNick();
	
	if (cmd.params.size() == 3 && !cmd.params[2].empty())
		msg << " :" << cmd.params[2];
	broadcast( *chan, msg.str(), -1 );

	chan->removeMember(target->getFd());
	deleteChannel(chan->getName());
}

void	Server::handleTopic(Client& client, const parsedCmd& cmd) {

	if (!client.isRegistered())
	{
		ErrClient(client, 451, ERR_NOTREGISTERED);
		return ;
	}
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

		msg << clientPrefix(client) << " TOPIC " << chan->getName() << " :" << chan->getTopic();
		broadcast( *chan, msg.str(), -1 );
		return ;
	}
}

void	Server::handleMode(Client& client, const parsedCmd& cmd) {

	if (!client.isRegistered())
	{
		ErrClient(client, 451, ERR_NOTREGISTERED);
		return ;
	}
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
		std::stringstream	msg;
		std::string		nick = client.getNick();
		bool			sign = false;

		if (nick.empty())
			nick = '*';
		msg << ":irc.serv 324 " << nick << " " << chan->getName() << " ";

		if (chan->isInviteOnly())
		{
			if (!sign)
			{
				msg << "+";
				sign = true;
			}
			msg << "i";
		}
		if (chan->isTopicRestricted())
		{
			if (!sign)
			{
				msg << "+";
				sign = true;
			}
			msg << "t";
		}
		if (chan->hasKey())
		{
			if (!sign)
			{
				msg << "+";
				sign = true;
			}
			msg << "k";
		}
		if (chan->getLimit() != 0)
		{
			if (!sign)
			{
				msg << "+";
				sign = true;
			}
			msg << "l " << chan->getLimit();
		}
		reply(client, msg.str());
		return ;
	}
	if (cmd.params.size() < 2 || cmd.params[1].empty()
			|| (cmd.params[1][0] != '+' && cmd.params[1][0] != '-'))
	{
		ErrCmd(client, 461, cmd.cmd, ERR_NEEDMOREPARAMS);
		return ;
	}

	size_t	arg_count = 0;
	bool	op = true;

	for (size_t i = 0; i < cmd.params[1].size(); i++)
	{
		if (cmd.params[1][i] == '+' || cmd.params[1][i] == '-')
		{
			if (cmd.params[1][i] == '-')
				op = false;
		}
	
		char mode = cmd.params[1][i];
	
		if (mode == 'k' || mode == 'o' || (mode == 'l' && op == true))
			arg_count++;
	}
	if (arg_count != cmd.params.size() - 2)
	{
		ErrCmd(client, 461, cmd.cmd, ERR_NEEDMOREPARAMS);
		return ;
	}
	if (cmd.params.size() >= 2 && chan->isOperator(client.getFd()))
	{
		arg_count = 1;

		for (size_t i = 0; i < cmd.params[1].size(); i++)
		{
			op = true;// '+'

			if (cmd.params[1][i] == '+' || cmd.params[1][i] == '-')
			{
				if (cmd.params[1][i] == '-')
					op = false;// '-'
				i++;
			}
			if (cmd.params[1][i] == 'i')
			{
				if (!op)
					chan->setInviteOnly(false);
				else
					chan->setInviteOnly(true);
				continue ;
			}
			else if (cmd.params[1][i] == 't')
			{
				if (!op)
					chan->setTopicRestricted(false);
				else
					chan->setTopicRestricted(true);
				continue ;
			}
			else if (cmd.params[1][i] == 'k')
			{
				arg_count++;
				if (op)
					chan->setKey(cmd.params[arg_count]);
				else if (!op && cmd.params[arg_count] == chan->getKey())
					chan->setKey("");
				else
				{
					ErrCmd(client, 461, cmd.cmd, ERR_NEEDMOREPARAMS);
					return ;
				}
				continue ;
			}
			else if (cmd.params[1][i] == 'o')
			{
				arg_count++;
				Client*	target = getClientByNick(cmd.params[arg_count]);

				if (target == NULL)
				{
					ErrCmd(client, 401, cmd.params[arg_count], ERR_NOSUCHNICK);
					return ;
				}
				if (!chan->isMember(target->getFd()))
				{
					ErrClientChannel(client, chan->getName(), cmd.params[arg_count], 441);
					return ;
				}
				if (op)
					chan->addOperator(target->getFd());
				else
					chan->removeOperator(target->getFd());
				continue ;
			}
			else if (cmd.params[1][i] == 'l')
			{
				if (op)
				{
					arg_count++;
					for(size_t j = 0; j < cmd.params[arg_count].size(); j++)
					{
						if (!std::isdigit(cmd.params[arg_count][j]))
						{
							ErrCmd(client, 461, cmd.cmd, ERR_NEEDMOREPARAMS);
							return ;
						}
					}
					std::stringstream	ss(cmd.params[arg_count]);
					size_t			limit;

					ss >> limit;
					chan->setLimit(limit);
				}
				else
					chan->setLimit(0);
				continue ;
			}
			ErrMode(client, chan->getName(), cmd.params[1][i]);//472
			return ;
		}
		return ;
	}
	ErrChannel(client, chan->getName(), 482, ERR_CHANOPRIVSNEEDED);
	return ;
}

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

	if (cmd.cmd != "PASS" && cmd.cmd != "NICK" && cmd.cmd != "USER"
			&& cmd.cmd != "QUIT" && !client.isRegistered())
	{
		ErrClient(client, 451, ERR_NOTREGISTERED);
		return ;
	}
	if (cmd.cmd == "USER")
	{
		handleUser(client, cmd);
		return ;
	}
	if (cmd.cmd == "NICK")
	{
		handleNick(client, cmd);
		return ;
	}
	if (cmd.cmd == "PASS")
	{
		handlePass(client, cmd);
		return ;
	}
	if (cmd.cmd == "JOIN")
	{
		handleJoin(client, cmd);
		return ;
	}
	if (cmd.cmd == "PART")
	{
		handlePart(client, cmd);
		return ;
	}
	if (cmd.cmd == "TOPIC")
	{
		handleTopic(client, cmd);
		return ;
	}
	if (cmd.cmd == "INVITE")
	{
		handleInvite(client, cmd);
		return ;
	}
	if (cmd.cmd == "KICK")
	{
		handleKick(client, cmd);
		return ;
	}
	if (cmd.cmd == "MODE")
	{
		handleMode(client, cmd);
		return ;
	}
//	FALTA:
//		[ ] PART
//		[ ] QUIT
	if ( cmd.cmd == "QUIT" )// No hecho solo test
	{
		reply( client, "ERROR :Closing link" );
		client.markForDisconnect();
		return ;
	}
	else
	{
		ErrCmd(client, 421, cmd.cmd, ERR_UNKNOWNCOMMAND);
		return ;
	}
}

