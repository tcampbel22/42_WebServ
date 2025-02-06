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

#include "ServerSettings.hpp"
#include "ConfigParser.hpp"

ServerSettings::ServerSettings() : _isDefaultServer(false), host(""), port(-1), max_client_body_size(-1) {}

ServerSettings::ServerSettings(std::string& key) : _isDefaultServer(false), _key(key), host(""), port(-1), max_client_body_size(-1) {}

ServerSettings::~ServerSettings() 
{
	error_pages.clear();
	locations.clear();
}


void	ServerSettings::parseServerBlock(std::vector<std::string>& serverBlock, std::vector<std::string>::iterator& it, std::vector<std::string>::iterator end)
{
	ConfigUtilities::trimServerBlock(serverBlock, it);
	const std::string	directives[6] = {"host", "port", "client_max_body_size", "location", "error_page", "server_names"};
	bool				isValid = false;
	
	for (; it != end; it++)
	{
		isValid = false;
		for (int i = 0; i < 6; i++)
		{
			if (!it->compare(directives[i]))
			{
				isValid = true;
				switch (i + 1)
				{
				case 1: //host
					parseHost(serverBlock, it);
					break;
				case 2: //port
					parsePort(serverBlock, it);
					break;
				case 3: //body size
					parseMaxBodySize(serverBlock, it);
					break;
				case 4: //location block
					parseLocationBlock(serverBlock, it);
					break;
				case 5: //error page
					parseErrorPages(serverBlock, it);
					break;
				case 6: //error page
					parseServerNames(serverBlock, it);
					break;
				default:
					throw std::logic_error("Something went very wrong....");
				break ;
				}
			}
		}
		if (!isValid)
			throw std::runtime_error("invalid directive in server block: " + *it);
	}
	checkConfigValues(serverBlock, it);
}

void	ServerSettings::parseHost(std::vector<std::string>& directive, std::vector<std::string>::iterator& it) 
{
	ConfigUtilities::checkDuplicates(host, "host:");
	ConfigUtilities::checkVectorEnd(directive, it, "host: empty value");
	ConfigUtilities::checkSemiColon(directive, it, "host: syntax error");
	std::string ip = *it;
	std::regex ip_addr("^((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])\\.){3}(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])$");
	if (std::regex_match(ip, ip_addr))
	{
		setHost(ip);
		it++;
		checkConfigValues(directive, it);
	}
	else
		throw std::runtime_error("host: invalid ip address " + *it);
}

void	ServerSettings::parsePort(std::vector<std::string>& directive, std::vector<std::string>::iterator& it) 
{
	ConfigUtilities::checkDuplicates(port, "port:");
	ConfigUtilities::checkVectorEnd(directive, it, "port: empty value");
	ConfigUtilities::checkSemiColon(directive, it, "port: syntax error");
	int n_port;
	std::string s_port = *it;
	std::regex port_num("^\\d{4,6}$");
	try {
		n_port = stoi(s_port);
	} catch(std::exception& e) {
		throw std::invalid_argument("port: (nan)");
	}
	if (std::regex_match(s_port, port_num) && n_port >= 1024 && n_port <= 65535)
	{
		port = n_port;
		it++;
		checkConfigValues(directive, it);
	}
	else
		throw std::runtime_error("invalid port number/invalid syntax: " + *it);
}


void	ServerSettings::parseServerNames(std::vector<std::string>& directive, std::vector<std::string>::iterator& it)
{
	ConfigUtilities::checkDuplicates(server_names, "server_names:");
	ConfigUtilities::checkVectorEnd(directive, it, "server_names: syntax error");
	std::regex names("^(?!-)(?:[A-Za-z0-9-]{1,63}\\.?)+[A-Za-z]{2,}$");
	int count = 0;
	for (; it != directive.end(); it++)
	{
		if (!it->compare(";"))
			break ;
		if (std::regex_match(*it, names))
		{
			if (it->length() > 50)
				throw std::invalid_argument("server_names: server name has 50 char size limit");
			server_names.push_back(*it);
		}
		else 
			throw std::runtime_error("server_names: invalid server_name/syntax error");
		count++;
	}
	if (count > 3)
		throw std::invalid_argument("server_names: max 3 servers names   allowed");
	ConfigUtilities::checkSemiColon(directive, std::prev(it), "server_names: syntax error");
	ConfigUtilities::checkServerNameDuplicates(server_names);
	checkConfigValues(directive, it);
}

int	checkBodySizeUnit(std::string& num)
{
	if (num.back() == 'm' || num.back() == 'M')
		return 1000000;
	else if (num.back() == 'k' || num.back() == 'K')
		return 1000;
	else if (num.back() >= '0' && num.back() <= '9')
		return 1;
	else
		throw std::runtime_error("max_body_size: syntax error: " + num);
	return 1;
}

void	ServerSettings::parseMaxBodySize(std::vector<std::string>& directive, std::vector<std::string>::iterator& it) 
{
	ConfigUtilities::checkDuplicates(max_client_body_size, "max client body:");
	ConfigUtilities::checkVectorEnd(directive, it, "max_body_size: empty value");
	ConfigUtilities::checkSemiColon(directive, it, "max_body_size: syntax error");
	int size;
	int	unit = checkBodySizeUnit(*it);
	try {
		size = stoi(*it);
	} catch(std::exception& e) {
		throw std::invalid_argument("max_body_size: (nan)");
	}
	if ((size * unit) >= 0 && (size * unit) < MAX_BODY_SIZE)
	{
		max_client_body_size = size * unit;
		it++;
		checkConfigValues(directive, it);
	}
	else
		throw std::runtime_error("invalid client max body size/invalid syntax");
}

void	ServerSettings::parseErrorPages(std::vector<std::string>& directive, std::vector<std::string>::iterator& it)  
{
	ConfigUtilities::checkVectorEnd(directive, it, "error_pages: empty error code value");
	ConfigUtilities::checkVectorEnd(directive, it, "error_pages: empty error page value");
	int error_code;
	try {
		error_code = stoi(*(it - 1));
	} catch(std::exception& e) {
		throw std::invalid_argument("error_pages: (nan)");
	}
	if (ConfigUtilities::checkErrorCode(error_code, true) && (it - 1)->length() == 3)
	{
		ConfigUtilities::checkSemiColon(directive, it, "error_pages: syntax error");
		addErrorPage(error_code, *it);
	}
	else
		throw std::runtime_error("error_pages: invalid error code: " + *(it - 1));
	ConfigUtilities::checkVectorEnd(directive, it, "error_pages: syntax error");
	checkConfigValues(directive, it);
}


void	ServerSettings::parseLocationBlockSettings(std::vector<std::string>& location, std::vector<std::string>::iterator& it)
{
	
	auto it2 = locations.insert({*it, LocationSettings(*it)});
	if (!it2.second)
		throw std::runtime_error("location: duplicate URI");
	std::string directives[9] = {"root", "index", "methods", "autoindex", "redirect", "error_page", "cgi_path", "cgi_script"};
	std::string	key = *it;
	bool		isValid = false;
	ConfigUtilities::checkVectorEnd(location, it, "location: invalid syntax");
	ConfigUtilities::shiftLocationBlock(location, it);
	for (;*it != "}"; it++)
	{
		isValid = false;
		for (int i = 0; i < 8; i++)
		{
			if (!it->compare(directives[i]))
			{
				isValid = true;
				switch(i + 1)
				{
					case 1:
						locations[key].parseRoot(location, it);
						break;
					case 2:
						locations[key].parseDefaultFile(location, it);
						break;
					case 3:
						locations[key].parseMethods(location, it);
						break;
					case 4:
						locations[key].parseAutoIndex(location, it);
						break;
					case 5:
						locations[key].parseRedirect(location, it);
						break;
					case 6:
						locations[key].parseLocationErrorPages(location, it);
						break;
					case 7:
						locations[key].parseCgiPath(location, it);
						break;
					case 8:
						locations[key].parseCgiScript(location, it);
						break;
					default:
						throw std::logic_error("Something went very wrong...");
				}
				break ;
			}
		}
		if (!isValid)
			throw std::runtime_error("location: invalid directive: " + *it);
	}
}

void	ServerSettings::parseLocationBlock(std::vector<std::string>& location, std::vector<std::string>::iterator& it)
{
	
	ConfigUtilities::checkVectorEnd(location, it, "location: invalid URI/path");
	std::regex uri("^(?!.*//)(?!.*--)(?!.*\\.\\.)[a-zA-Z0-9._\\-/]*$");
	if (std::next(it) != location.end() && std::regex_match(*it, uri) && !std::next(it)->compare("{"))
		parseLocationBlockSettings(location, it);
	else
		throw std::runtime_error("location: syntax error");
}
