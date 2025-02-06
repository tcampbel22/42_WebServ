/**********************************************************************************/
/** __          __  _                                                            **/
/** \ \        / / | |                                by:                        **/
/**  \ \  /\  / /__| |__  ___  ___ _ ____   __                                   **/
/**   \ \/  \/ / _ \ '_ \/ __|/ _ \ '__\ \ / /        Eromon Agbomeirele         **/
/**    \  /\  /  __/ |_) \__ \  __/ |   \ V /         Casimir Lundberg           **/
/**     \/  \/ \___|_.__/|___/\___|_|    \_/          Tim Campbell               **/
/**                                                                              **/
/**                                                                              **/
/**                                W E B S E R V                                 **/
/**********************************************************************************/

#include "../../include/webserv.hpp"
#include "../Server/HttpServer.hpp"
#include "../Config/ConfigParser.hpp"
#include "../Config/ServerSettings.hpp"
#include "../HttpParsing/HttpParser.hpp"
#include "../Logger/Logger.hpp"

int	main(int ac, char **av)
{
	Logger log;
	if (ac != 2 || av[1] == nullptr || av[1][0] == '\0')
	{
		Logger::log("expecting only configuration file as argument",  ERROR, true);
		return 1;
	}
	if (std::filesystem::exists((std::string)av[1]) && std::filesystem::file_size((std::string)av[1]) == 0) 
	{
        Logger::log("config file is empty",  ERROR, true);
		return 1;
	}
	try {
		if (opendir(av[1]) != NULL)
			throw std::invalid_argument("argument is a directory");
	} 
	catch (std::exception& e)
	{
		Logger::log(e.what(), ERROR, true);
		return 1; 
	}
	//Program will exit if an error is found with the config file
	ConfigParser config;
	try {
		config.parseConfigFile((std::string)av[1]);
	}
	catch (std::exception& e)
	{
		Logger::log(e.what(), ERROR, true);
		return 1;
	}
	//start server class, calls the socket creation function in constructor, closes the socket in the destructor.
	HttpServer server(config.settings_vec);
	config.~ConfigParser();
	server.startListening();
	Logger::closeLogger();
	return (0);
}