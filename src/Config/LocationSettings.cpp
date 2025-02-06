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

#include "LocationSettings.hpp"

LocationSettings::LocationSettings()
{
	path = "/";
	if (path.back() == '/')
		isDirectory = true;
	else
		isFile = true;
	root = "";
	default_file = "";
	is_default_file = false;
	is_redirect = false;
	isCgi = false;
	autoindex = false;
	redirect.first = -1;
	redirect.second = "";
	cgi_path = "";
	cgi_script = "";
}


LocationSettings::LocationSettings(const std::string& new_path)
{
	path = new_path;
	if (path.back() == '/')
		isDirectory = true;
	else
		isFile = true;
	root = "";
	default_file = "";
	is_default_file = false;
	is_redirect = false;
	isCgi = false;
	autoindex = false;
	redirect.first = -1;
	redirect.second = "";
	cgi_path = "";
	cgi_script = "";
}

LocationSettings::~LocationSettings() 
{
	location_error_pages.clear();
	methods.clear();
	methods.shrink_to_fit();
}

//PARSERS

bool LocationSettings::hasDelete()
{
	for (auto it = methods.begin(); it != methods.end(); it++)
	{
		if (*it == DELETE)
			return true;
	}
	return false;
}

void	LocationSettings::checkLocationValues(std::vector<std::string>::iterator& it)
{
	
	if (!std::next(it)->compare("}"))
	{

		if (isCgi && (cgi_script.empty() || cgi_path.empty()))
			throw std::runtime_error("location: missing directives in cgi block: " + getPath());
		if (isCgi && (!methods.empty() || !default_file.empty()))
			throw std::runtime_error("location: extra directives in cgi block: " + getPath());
		if (!isCgi && redirect.first > 0 && (!root.empty() || autoindex || !methods.empty() || !default_file.empty()))
			throw std::runtime_error("location: extra directives in redirect block");
		if (!isCgi && !path.compare("/"))
		{
			if (default_file.empty() && !autoindex)
				throw std::runtime_error("location: root: index/default file missing");
		}
		if (!isCgi && !is_redirect)
		{
			if (methods.empty())
				throw std::runtime_error("location: methods missing from location block: " + getPath());
		}
		if (!isCgi && !is_redirect && root.empty())
			throw std::runtime_error("location: root path missing from location block: " + getPath());
	}
}

void	LocationSettings::parseRoot(std::vector<std::string>& location, std::vector<std::string>::iterator& it) 
{
	ConfigUtilities::checkDuplicates(root, "root:");
	ConfigUtilities::checkVectorEnd(location, it, "location: root: invalid syntax");
	std::regex root_regex("\\/[a-zA-Z0-9._\\-\\/]+");
	if (std::regex_match(*it, root_regex) && std::next(it)->compare(";") == 0)
		root = *it;
	else
		throw std::runtime_error("location: root: invalid root path/syntax error");
	ConfigUtilities::checkVectorEnd(location, it, "location: root: invalid syntax");
	checkLocationValues(it);
}

void	LocationSettings::parseDefaultFile(std::vector<std::string>& location, std::vector<std::string>::iterator& it) 
{ 
	ConfigUtilities::checkDuplicates(default_file, "default file:");
	ConfigUtilities::checkVectorEnd(location, it, "location: default file: invalid syntax");
	ConfigUtilities::checkSemiColon(location, it, "location: default file: syntax error");
	default_file = *it;
	is_default_file = true;
	ConfigUtilities::checkVectorEnd(location, it, "location: default file: invalid syntax");
	checkLocationValues(it);
}

void	LocationSettings::parseAutoIndex(std::vector<std::string>& location, std::vector<std::string>::iterator& it) 
{
	ConfigUtilities::checkDuplicates(autoindex, "autoindex:");
	ConfigUtilities::checkVectorEnd(location, it, "location: autoindex: invalid syntax");
	ConfigUtilities::checkSemiColon(location, it, "location: autoindex: syntax error");
	if (!it->compare("on"))
		autoindex = true;
	else if (!it->compare("off"))
		autoindex = false;
	else
		throw std::runtime_error("location: autoindex: value must be on or off");
	ConfigUtilities::checkVectorEnd(location, it, "location: autoindex: invalid syntax");
	checkLocationValues(it);
}

void	LocationSettings::parseRedirect(std::vector<std::string>& location, std::vector<std::string>::iterator& it)
{
	ConfigUtilities::checkDuplicates(redirect, "redirect:");
	ConfigUtilities::checkVectorEnd(location, it, "location: redirect: invalid syntax");
	int error_code;
	try {
		error_code = stoi(*it);
	} catch(std::exception& e) {
		throw std::invalid_argument("location_error_pages: (nan)");
	}
	if (ConfigUtilities::checkErrorCode(error_code, false) && it->length() == 3)
	{
		redirect.first = error_code;
		ConfigUtilities::checkVectorEnd(location, it, "location: redirect: invalid syntax");
		redirect.second = *it;
		ConfigUtilities::checkSemiColon(location, it, "location: redirect: syntax error");
		is_redirect = true;
	}
	else
		throw std::runtime_error("location: redirect: invalid status code: " + *it);
	ConfigUtilities::checkVectorEnd(location, it, "location: redirect: invalid syntax");
	checkLocationValues(it);
}

void	LocationSettings::parseMethods(std::vector<std::string>& location, std::vector<std::string>::iterator& it)
{
	ConfigUtilities::checkDuplicates(methods, "methods:");
	ConfigUtilities::checkVectorEnd(location, it, "location: methods: syntax error");
	std::string method_str[3] = {"GET", "POST", "DELETE"};
	for (; it != location.end(); it++)
	{
		if (!it->compare(";"))
			break ;
		if (!it->compare(method_str[0]))
			methods.push_back(GET);
		else if (!it->compare(method_str[1]))
			methods.push_back(POST);
		else if (!it->compare(method_str[2]))
			methods.push_back(DELETE);
		else 
			throw std::runtime_error("location: methods: invalid method type");
	}
	ConfigUtilities::checkSemiColon(location, std::prev(it), "location: methods: syntax error");
	ConfigUtilities::checkMethodDuplicates(methods);
	checkLocationValues(it);
}

void	LocationSettings::parseLocationErrorPages(std::vector<std::string>& location, std::vector<std::string>::iterator& it)  
{
	ConfigUtilities::checkVectorEnd(location, it, "location_error_pages: empty error code value");
	ConfigUtilities::checkVectorEnd(location, it, "location_error_pages: empty error page value");
	int error_code;
	try {
		error_code = stoi(*(it - 1));
	} catch(std::exception& e) {
		throw std::invalid_argument("location_error_pages: (nan)");
	}
	if (ConfigUtilities::checkErrorCode(error_code, true) && (it - 1)->length() == 3)
	{
		ConfigUtilities::checkSemiColon(location, it, "location_error_pages: syntax error");
		addLocationErrorPage(error_code, *it);
	}
	else
		throw std::runtime_error("location_error_pages: invalid error code");
	ConfigUtilities::checkVectorEnd(location, it, "location_error_pages: syntax error");
	checkLocationValues(it);
}


void	LocationSettings::parseCgiPath(std::vector<std::string>& location, std::vector<std::string>::iterator& it) 
{
	ConfigUtilities::checkDuplicates(cgi_path, "cgi_path:");
	ConfigUtilities::checkVectorEnd(location, it, "location: cgi_path: invalid syntax");
	ConfigUtilities::checkSemiColon(location, it, "location: cgi_path: syntax error");
	cgi_path = *it;
	if (isCgi == false)
		isCgi = true;
	ConfigUtilities::checkVectorEnd(location, it, "location: cgi_path: invalid syntax");
	checkLocationValues(it);
}

void	LocationSettings::parseCgiScript(std::vector<std::string>& location, std::vector<std::string>::iterator& it) 
{
	ConfigUtilities::checkDuplicates(cgi_script, "cgi_script:");
	ConfigUtilities::checkVectorEnd(location, it, "location: cgi_script: invalid syntax");
	ConfigUtilities::checkSemiColon(location, it, "location: cgi_script: syntax error");
	cgi_script = *it;
	if (isCgi == false)
		isCgi = true;
	ConfigUtilities::checkVectorEnd(location, it, "location: cgi_script: invalid syntax");
	checkLocationValues(it);
}

void	LocationSettings::addLocationErrorPage(int status, std::string path) 
{ 
	if (location_error_pages.find(status) != location_error_pages.end())
		throw std::runtime_error("error_pages: duplicate path");
	location_error_pages[status] = path;
}

//GETTERS
std::string&				LocationSettings::getPath() { return path;}
std::string&				LocationSettings::getRoot() { if (isCgi == false) return root; else return cgi_path;}
std::string&				LocationSettings::getDefaultFile() { return default_file; }
std::pair<int, std::string>	LocationSettings::getRedirect() { return redirect; }
std::vector<int>&			LocationSettings::getMethods() { return methods; }
bool						LocationSettings::isAutoIndex() { return autoindex; }
bool						LocationSettings::isDefaultFile() { return is_default_file; }
bool						LocationSettings::isRedirect() { return is_redirect; }
bool						LocationSettings::isCgiBlock() { return isCgi; }
std::string&				LocationSettings::getCgiPath() { return  cgi_path; }
std::string&				LocationSettings::getCgiScript() { return cgi_script; }
std::string					LocationSettings::getErrorPagePath(int key) 
{ 
	if (location_error_pages.find(key) != location_error_pages.end())
	{
		auto it = location_error_pages.find(key);
		return it->second;
	}
	else 
		return ""; 
}

int								LocationSettings::getRedirectStatus() { return redirect.first; }
std::string&					LocationSettings::getRedirectPath() { return redirect.second; }