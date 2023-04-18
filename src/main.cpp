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
#include <csignal>
#include "JsonParser.hpp"
#include "ServerConfig.hpp"
#include "ServerManager.hpp"
#include "CGIExecutor.hpp"

void	signalHandler(int signal, ServerManager &serverManager) {
	std::cout << "Signal handler called for signal : " << signal << std::endl;
	if (signal == SIGINT)
		serverManager._client_map.clear();
	system("leaks webserv");
	exit(EXIT_SUCCESS);
}

ServerManager* g_serverManager = 0;

void signalHandler(int signal)
{
    if (g_serverManager)
    {
        std::cout << RED << "\nserver closed with signal " << signal << " (ctrl-c)." << RESET << std::endl;
        delete g_serverManager;
        g_serverManager = 0;
    }
	//system("leaks webserv");
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv, char ** envp)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv <config_file>" << std::endl;
		return 1;
	}

	CGIExecutor::getCgiInstance().setEnv(envp);

	JsonParser parser(argv[1]);
	ServerConfig config(parser);
	g_serverManager = new ServerManager(config);

	std::signal(SIGINT, (void (*)(int))signalHandler);
	g_serverManager->run();

	return 0;
}
