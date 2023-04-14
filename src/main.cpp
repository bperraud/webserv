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

int main(int argc, char **argv, char ** envp)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv <config_file>" << std::endl;
		return 1;
	}
	std::cout << "server started..." << std::endl;

	CGIExecutor::getCgiInstance().setEnv(envp);

	JsonParser parser(argv[1]);
	ServerConfig config(parser);
	ServerManager serverManager(config);

	serverManager.run();

	return 0;
}
