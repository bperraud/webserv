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
#include "Config.hpp"
#include "ServerManager.hpp"
#include "CGIExecutor.hpp"

int main(int argc, char **argv, char ** envp)
{
	//if (argc != 2)
		//return 1;

	Config config(argv[1]);
	ServerManager serverManager(config);

	//CGIExecutor cgi(envp);
	// Set up listen sockets

	serverManager.setupSocket();
	serverManager.handleNewConnections();


	return 0;
	//serverManager.listenAll();

	// Main event loop
	while (true)
	{
		// Wait for events on our file descriptors

		// Process incoming connections

		// Process requests from existing connections
	}

	return 0;
}
