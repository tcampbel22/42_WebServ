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

#include "ServerSettings.hpp"
#include <unordered_map>
#include <iostream>
#include <vector>
#include <filesystem>

class LocationSettings
{
private:
	std::string								path; //URI
	std::string 							root; //directory
	std::vector<int>						methods; //allowed methods
	std::string								default_file; //index index.html
	bool									is_default_file;
	std::unordered_map<int, std::string>	location_error_pages;
	bool									autoindex; //enable or disable directory listing
	std::pair<int, std::string>				redirect;
	bool									is_redirect;
	bool									isDirectory = false;
	bool									isFile = false;
	bool									isCgi;
	std::string								cgi_path;
	std::string								cgi_script;

public:
	LocationSettings();
	LocationSettings(const std::string& new_path);
	~LocationSettings();
	void	checkLocationValues(std::vector<std::string>::iterator& it);
	void	parseRoot(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	void	parseDefaultFile(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	void	parseAutoIndex(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	void	parseMethods(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	void	parseRedirect(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	void	parseLocationErrorPages(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	void	addLocationErrorPage(int status, std::string path);
	void	parseCgiPath(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	void	parseCgiScript(std::vector<std::string>& location, std::vector<std::string>::iterator& it);
	void	parseCgiUpload(std::vector<std::string>& location, std::vector<std::string>::iterator& it);

	std::string&					getPath(); //URI
	std::string&					getRoot(); //root
	std::string&					getDefaultFile(); //default homepage
	std::vector<int>&				getMethods(); //Methods POST GET DELETE
	bool							isAutoIndex(); //directory listing
	std::pair<int, std::string>		getRedirect();
	int								getRedirectStatus();
	std::string&					getRedirectPath();
	bool							isDefaultFile();
	bool							isRedirect();
	bool							isCgiBlock();
	std::string&					getCgiPath();
	std::string&					getCgiScript();
	std::string						getErrorPagePath(int key);
	bool							hasDelete();
};