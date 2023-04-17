/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bperraud <bperraud@student.s19.be>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/02/21 14:26:11 by bperraud          #+#    #+#             */
/*   Updated: 2023/02/21 14:26:11 by bperraud         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include "JsonParser.hpp"
#include "ServerConfig.hpp"
#include "ServerManager.hpp"
#include "CGIExecutor.hpp"

void	signalHandler(int signal, ServerManager &serverManager) {
	std::cout << "Signal handler called for signal : " << signal << std::endl;
	if (signal == SIGINT)
		serverManager._client_map.clear();
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv, char ** envp)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv <config_file>" << std::endl;
		return 1;
	}
	std::cout << "Server started" << std::endl;

	JsonParser parser(argv[1]);
	ServerConfig config(parser);
	CGIExecutor cgi(envp);
	ServerManager serverManager(config, cgi);

	std::signal(SIGINT, (void (*)(int))signalHandler);
	serverManager.run();

	return 0;
}
