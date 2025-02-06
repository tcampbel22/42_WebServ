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

#include <vector>
#include "ConfigParser.hpp"
#include "ServerSettings.hpp"
#include "LocationSettings.hpp"
#include <variant>
#include <algorithm>

class ServerSettings;
class LocationSettings;

class ConfigUtilities
{
public:
	ConfigUtilities();
	~ConfigUtilities();
	static	void	checkBrackets(std::vector<std::string> serverBlock);
	static	void	trimServerBlock(std::vector<std::string>& serverBlock, std::vector<std::string>::iterator& it);
	static	void	shiftLocationBlock(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	static	void	checkVectorEnd(std::vector<std::string>& vec, std::vector<std::string>::iterator& it, std::string msg);
	static	void	checkSemiColon(std::vector<std::string>& vec, std::vector<std::string>::iterator it, std::string msg);
	static	void	checkDuplicates(std::variant<int, bool, std::string, std::vector<int>, std::vector<std::string>, std::pair<int, std::string>> val, std::string msg);
	static	void	printServerBlock(ServerSettings server);
	static	void	printLocationBlock(LocationSettings location);
	static	void	checkMethodDuplicates(std::vector<int>& location);
	static	void	checkServerNameDuplicates(std::vector<std::string> server_names);
	static	void	checkDefaultBlock(ServerSettings block, bool server);
	static	bool	checkErrorCode(int code, bool error);
	static	void	printWhiteSpace(std::string str);
};
