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

#include "ConfigParser.hpp"

ConfigParser::ConfigParser() : defaultServer(false) {}

int	checkConfName(const std::string& file)
{
	int pos = 0;
	std::string temp = file;
	if (temp.length() > 5 && temp.at(temp.length() - 5) == '.')
	{
		pos = temp.length() - 5;
		std::string suffix = temp.substr(pos, temp.length() - pos);
		if (suffix.compare(".conf") != 0)
			return 1;
	}
	return 0;
}
void	ConfigParser::readConfigFile(const std::string& file)
{
	
	std::ostringstream	stream;
	std::ifstream		infile(file, std::ios::in);
	if (!infile)
		throw std::runtime_error("infile failed to open");
	if (checkConfName(file))
		throw std::runtime_error("invalid config file");
	infile.open(file);
	if (!infile.is_open())
		throw std::runtime_error("failed to open config file");
	stream << infile.rdbuf();
	configFileStr = stream.str();
	infile.close();
}

void		ConfigParser::parseConfigFile(const std::string& file) 
{
	readConfigFile(file);
	removeComments();
	tokenise(configFileStr);
	splitServerBlocks();
}

void	ConfigParser::removeComments()
{
	int 	i = -1;
	uint	start = 0;
	while (configFileStr[++i] != '\0')
	{	
		if (configFileStr[i] == '#')
		{
			start = i;
			while (configFileStr[i] != '\n')
				i++;
			configFileStr.erase(start, i - start);
			i = start;
		}
	}
}

void	ConfigParser::tokenise(const std::string& c)
{
	std::string token;
	for (size_t i = 0; i < c.length(); i++)
	{
		if (isspace(c[i]))
		{
			if (!token.empty())
			{
				tokens.push_back(token);
				token.clear();
			}
		}
		else if (c[i] == '{' || c[i] == '}' || c[i] == ';')
		{
			if (!token.empty())
			{
				tokens.push_back(token);
				token.clear();
			}
			tokens.push_back(std::string(1, c[i]));
		}
		else
			token.push_back(c[i]);
	}
	if (!token.empty())
		tokens.push_back(token);
}

std::string	ConfigParser::getConfigFileStr() { return configFileStr; }

void	createKey(std::vector<std::string>::iterator start,  std::vector<std::string>::iterator end, std::string& key)
{
	std::string	_host;
	std::string	_port;
	auto host_it = std::find(start, end, "host");
	if (host_it->compare("host"))
		throw std::runtime_error("create_key: no host in server block");
	if (std::next(host_it) != end)
		_host = *(host_it + 1);
	auto port_it = std::find(start, end, "port");
	if (port_it->compare("port"))
		throw std::runtime_error("create_key: no port in server block");
	if (std::next(port_it) != end)
		_port = *(port_it + 1);
	key = _host + ":" + _port;
}

void	ConfigParser::splitServerBlocks() 
{
	if (tokens.begin()->compare("server") || tokens[1] != "{")
		throw std::runtime_error("splitServerBlock: syntax error: empty value");
	ConfigUtilities::checkBrackets(tokens);
	auto it = tokens.begin();
	int	server_count = -1;
	for (; it != tokens.end(); it++)
	{
		if (!it->compare("server") && !std::next(it)->compare("{"))
		{
			ConfigUtilities::checkVectorEnd(tokens, it, "splitServerBlock: syntax error");
			ConfigUtilities::checkVectorEnd(tokens, it, "splitServerBlock: syntax error");
			auto start = it;
			auto end = start;
			std::string key = "";
			for (; end != tokens.end(); end++)
			{
				if (end != tokens.end() && !end->compare("}") && (std::next(end) == tokens.end() || !std::next(end)->compare("server")))
				{
					createKey(start, end, key);
					ServerSettings temp(key);
					ConfigUtilities::checkDefaultBlock(temp, defaultServer);
					settings_vec.push_back(temp);
					server_count++;
					break ;
				}
			} 
			if (server_count >= 0)
			{
				settings_vec[server_count].parseServerBlock(tokens, it, end);
				settings_vec[server_count].checkServerBlock();
			}
		}
	}
	checkHostPortDuplicates();
	tokens.clear();
}

ServerSettings*	ConfigParser::getServerBlock(const std::string key)
{
	for (size_t i = 0; i < settings_vec.size(); i++)
	{
		if (settings_vec[i].getKey().compare(key) == 0)
			return &settings_vec[i];
	}
	return nullptr;
}

void	ConfigParser::checkHostPortDuplicates()
{
	std::vector<std::string> dup;
	for (auto it = settings_vec.begin(); it != settings_vec.end(); it++)
		dup.push_back(it->getKey());
	std::sort(dup.begin(), dup.end());
	for (auto it = dup.begin()+1; it != dup.end(); it++)
	{
		if (!it->compare(*(it-1)))
			throw std::runtime_error("config: duplicate host and port");
	}
	dup.clear();
}

bool ConfigParser::getDefaultServer() { return defaultServer; }

ConfigParser::~ConfigParser() 
{
	settings_vec.clear();
	tokens.clear();
	tokens.shrink_to_fit();
}