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
#include "ConfigUtilities.hpp"
#include "LocationSettings.hpp"
#include <vector>
#include <unordered_map>
#include <variant>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <regex>
#include <iterator>

#define GET 1
#define POST 2
#define DELETE 3

class LocationSettings;

class ServerSettings
{
private:
	bool		_isDefaultServer;
	std::string	_key;
	std::string host;
	int			port;
	int			max_client_body_size;
	std::vector <std::string> server_names;
	std::unordered_map<int, std::string> error_pages;
	std::unordered_map<std::string, LocationSettings> locations;

public:
	int			_fd = 0;
	ServerSettings();
	ServerSettings(std::string& key);
	~ServerSettings();
	//PARSERS
	void						parseServerBlock(std::vector<std::string>& serverBlock, std::vector<std::string>::iterator& it, std::vector<std::string>::iterator end);
	void						parseHost(std::vector<std::string>& directive, std::vector<std::string>::iterator& it);
	void						parsePort(std::vector<std::string>& directive, std::vector<std::string>::iterator& it);
	void						parseServerNames(std::vector<std::string>& directive, std::vector<std::string>::iterator& it);
	void						parseMaxBodySize(std::vector<std::string>& directive, std::vector<std::string>::iterator& it);
	void						parseErrorPages(std::vector<std::string>& serverBlock, std::vector<std::string>::iterator& it);
	void						checkConfigValues(std::vector<std::string>& directive, std::vector<std::string>::iterator& it);
	void						parseLocationBlock(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	void						parseLocationBlockSettings(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	void						checkServerBlock();
	//SETTERS
	void						setDefaultServer(bool val);
	void						setHost(std::string ip);
	void						setPort(int port_num);
	void						setMaxClientBodySize(int size);
	void						addErrorPage(int status, std::string path);
	//GETTERS
	bool												isDefaultServer();
	std::string											getKey();
	int													getPort();
	std::string&										getHost();
	int													getMaxClientBody();
	std::unordered_map<int, std::string>&				getAllErrorPages();
	std::string											getErrorPages(int key);
	LocationSettings*									getLocationBlock(const std::string key);
	std::vector<std::string>							getServerNames();
};