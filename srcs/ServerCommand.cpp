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
