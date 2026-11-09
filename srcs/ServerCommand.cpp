/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommand.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: diespino <diespino@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 18:51:17 by diespino          #+#    #+#             */
/*   Updated: 2026/11/04 18:30:29 by diespino         ###   ########.fr       */
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

void    Server::handlePass(Client& client, parsedCmd& cmd)
{
	// Si el cliente aún no tiene nick, el estándar indica que se use "*" en las respuestas
	std::string nick = client.getNick().empty() ? "*" : client.getNick();

	if (client.isRegistered() || client.isPassOk())
	{
		reply(client, ":server 462 " + nick + " :Unauthorized command (already registered)");
		return ;
	}

	if (cmd.params.empty())
	{
		reply(client, ":server 461 " + nick + " PASS :Not enough parameters");
		return ;
	}

	if (cmd.params[0] == _pwd)
	{
		client.setPassOk(true);
	}
	else
	{
		reply(client, ":server 464 " + nick + " :Password incorrect");
		client.markForDisconnect();
	}
}

void	Server::handleJoin(Client& client, parsedCmd& cmd) {

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

	if ( chan == NULL )
	{
		chan = createChannel(cmd.params[0]);
		if (chan == NULL)// MSG_ERROR ???
			return ;
		chan->addOperator(client.getFd());
	}
	if (chan->getLimit() != 0 && chan->memberCount() == chan->getLimit())
	{
		ErrChannel(client, chan->getName(), 471, ERR_CHANNELISFULL);
		return ;
	}
	if (chan->isInviteOnly())
	{
		if (!chan->isInvited(client.getFd()))
		{
			ErrChannel(client, chan->getName(), 473, ERR_INVITEONLYCHAN);
			return ;
		}
		chan->removeInvite(client.getFd());
	}
	if (chan->hasKey())
	{
		if (cmd.params.size() < 2 || cmd.params[1] != chan->getKey())
		{
			ErrChannel(client, chan->getName(), 475, ERR_BADCHANNELKEY);
			return ;
		}
	}
	if (chan->isInvited(client.getFd()))
		chan->removeInvite(client.getFd());
//
//	Enviar mensaje de JOINED a todos los clientes (incluido al nuevo miembro) del CHANNEL
	if (!chan->isMember(client.getFd()))
	{
		std::ostringstream	msg;

		chan->addMember(client.getFd());
//		Flata hacere prefix de client --> :nick!user@hostname
//		msg << clientPrefix(client) << " JOIN " << chan->getName();
		msg << ":nick!user@hostname JOIN " << chan->getName();
		broadcast( *chan, msg.str(), -1 );
	}
	return ;
}

bool Server::isValidNick(const std::string& nick)
{
	if (nick.empty())
		return false;
	if (nick[0] == '#' || nick[0] == ':')
		return false;
	if (nick.find_first_of(" ,*?!@.") != std::string::npos)
		return false;
	return true;
}

void Server::checkRegistration(Client& client)
{
	if (!client.isRegistered() && client.isPassOk() 
		&& !client.getNick().empty() && !client.getUser().empty())
	{
		client.setRegistered(true);

		std::string nick = client.getNick();
		std::string user = client.getUser();
		std::string host = client.getHostname();

		reply(client, ":server 001 " + nick + " :Welcome to the Internet Relay Network " + nick + "!" + user + "@" + host);
		reply(client, ":server 002 " + nick + " :Your host is server, running version 1.0");
		reply(client, ":server 003 " + nick + " :This server was created today");
		reply(client, ":server 004 " + nick + " server 1.0 o o");
	}
}

void Server::handleUser(Client& client, parsedCmd& cmd)
{
	if (client.isRegistered())
	{
		ErrClient(client, 462, ERR_ALREADYREGISTRED);
		return ;
	}

	// USER exige 4 parámetros: <username> <hostname> <servername> :<realname>
	if (cmd.params.size() < 4)
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

	// Si el cliente ya estaba registrado y cambia el nick en mitad de la sesión,
	// el servidor debe avisarle a él mismo para que su cliente IRC actualice la interfaz.
	if (client.isRegistered())
	{
		std::string msg = ":" + oldNick + " NICK :" + newNick;
		reply(client, msg);
	}
	else
		checkRegistration(client);
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

//	Mientras no haya USER, NICK Y PASS comentar lineas 199 <--> 204 para probar CMDs
//	Tambien hacer con comandos como JOIN que comprueban si client.isRegistered()
//
//	if (cmd.cmd != "PASS" && cmd.cmd != "NICK" && cmd.cmd != "USER"
//			&& cmd.cmd != "QUIT" && !client.isRegistered())
//	{
//		ErrClient(client, 451, ERR_NOTREGISTERED);
//		return ;
//	}
	if (cmd.cmd == "PASS")
    	{
        	handlePass(client, cmd);
        	return ;
   	}
    	if (cmd.cmd == "NICK")
    	{
        	handleNick(client, cmd);
        	return ;
    	}
  	if (cmd.cmd == "USER")
    	{
        	handleUser(client, cmd);
        	return ;
	}
	if (cmd.cmd == "JOIN")
	{
		handleJoin(client, cmd);
		return ;
	}
	/*
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
*/
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
