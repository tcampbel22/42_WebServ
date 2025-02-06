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

#pragma once

#include "../../include/webserv.hpp"
#include "ServerSettings.hpp"
#include "LocationSettings.hpp"
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <cctype>
#include <algorithm>
#include <iterator>

class ServerSettings;

class ConfigParser
{
private:
	std::string					configFileStr;
	std::vector<std::string>	tokens; 
	bool						defaultServer;
public:
	std::vector<ServerSettings> settings_vec;
	ConfigParser();
	~ConfigParser();
	void			readConfigFile(const std::string& file);
	void			parseConfigFile(const std::string& file);
	void			removeComments();
	std::string		getConfigFileStr();
	void			tokenise(const std::string& config);
	void			splitServerBlocks();
	bool			getDefaultServer();
	ServerSettings*	getServerBlock(const std::string key);
	void			checkHostPortDuplicates();
};
