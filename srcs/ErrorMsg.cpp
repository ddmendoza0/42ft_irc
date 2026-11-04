/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorMsg.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: diespino <diespino@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/04 15:00:10 by diespino          #+#    #+#             */
/*   Updated: 2026/11/04 18:32:23 by diespino         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

void	Server::ErrClient(Client& client, int code, const std::string& msg) {

	std::ostringstream	output;
	std::string		nick = client.getNick();

	output << ":server ";
	if (nick.empty())
		nick = "*";
	if (code < 10)
		output << "00";
	else if (code < 100)
		output << "0";
	
	output << code << " " << nick << msg;
	
	reply(client, output.str());
}

void	Server::ErrCmd(Client& client, int code, std::string cmd, const std::string& msg) {

	std::ostringstream	output;
	std::string		nick = client.getNick();

	output << ":server ";
	if (nick.empty())
		nick = "*";
	if (code < 10)
		output << "00";
	else if (code < 100)
		output << "0";
	
	output << code << " " << nick << " " << cmd << msg;
	
	reply(client, output.str());
}

void	Server::ErrChannel(Client& client, std::string chan, int code, const std::string& msg) {

	std::ostringstream	output;
	std::string		nick = client.getNick();

	output << ":server ";
	if (nick.empty())
		nick = "*";
	if (code < 10)
		output << "00";
	else if (code < 100)
		output << "0";
	
	output << code << " " << nick << " " << chan << msg;
	
	reply(client, output.str());
}
