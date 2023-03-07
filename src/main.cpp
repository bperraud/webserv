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

int main(int argc, char **argv)
{
	//if (argc != 2)
		//return 1;

	Config config(argv[1]);
	ServerManager serverManager(config);

	// Set up listen sockets

	serverManager.setupSocket();

	serverManager.handleNewConnectionsEpoll();

	return 0;
	//serverManager.listenAll();

	// Main event loop
	while (true)
	{
		// Wait for events on our file descriptors
		serverManager.pollSockets();

		// Process incoming connections
		serverManager.handleNewConnections();

		// Process requests from existing connections
		serverManager.handleRequests(0);
	}

	return 0;
}
