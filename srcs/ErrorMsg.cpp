/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorMsg.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: diespino <diespino@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/04 15:00:10 by diespino          #+#    #+#             */
/*   Updated: 2026/11/09 18:58:45 by diespino         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

void	Server::ErrClient(Client& client, int code, const std::string& msg) {

	std::ostringstream	output;
	std::string		nick = client.getNick();

	output << ":irc.serv ";
	if (nick.empty())
		nick = "*";
	if (code < 10)
		output << "00";
	else if (code < 100)
		output << "0";
	
	output << code << " " << nick << " :" << msg;
	
	reply(client, output.str());
}

void	Server::ErrCmd(Client& client, int code, std::string cmd, const std::string& msg) {

	std::ostringstream	output;
	std::string		nick = client.getNick();

	output << ":irc.serv ";
	if (nick.empty())
		nick = "*";
	if (code < 10)
		output << "00";
	else if (code < 100)
		output << "0";
	
	output << code << " " << nick << " " << cmd << " :" << msg;
	
	reply(client, output.str());
}

void	Server::ErrChannel(Client& client, std::string chan, int code, const std::string& msg) {

	std::ostringstream	output;
	std::string		nick = client.getNick();

	output << ":irc.serv ";
	if (nick.empty())
		nick = "*";
	if (code < 10)
		output << "00";
	else if (code < 100)
		output << "0";
	
	if (code == 353)	
		output << code << " " << nick << " = " << chan  << " :" << msg;
	else
		output << code << " " << nick << " " << chan  << " :" << msg;
	
	reply(client, output.str());
}

void	Server::ErrMode(Client& client, std::string chan, char c) {

	std::ostringstream	output;
	std::string		nick = client.getNick();

	output << ":irc.serv 472 ";

	if (nick.empty())
		nick = "*";
	
	output << nick << " " << c << " :" << ERR_UNKNOWNMODE << chan;
	
	reply(client, output.str());
}

void	Server::ErrClientChannel(Client& client, const std::string& chan, const std::string& arg, int code) {

	std::ostringstream	output;
	std::string		nick = client.getNick();

	output << ":irc.serv " << code << " ";

	if (nick.empty())
		nick = "*";

	if (code == 441)
		output << nick << " " << arg << " " << chan << " :" << ERR_USERNOTINCHANNEL;
	else if (code == 443)
		output << nick << " " << arg << " " << chan << " :" << ERR_USERONCHANNEL;
	reply(client, output.str());
}
