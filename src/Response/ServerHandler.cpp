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

#include "ServerHandler.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <string>

ServerHandler::ServerHandler(int fd, HttpRequest& _newInput):
_response(), _input(_newInput)
{
	try
	{
		_response.setCloseConnection(_input.closeConnection);
		//populate the MIME map
		makeMIME();

		//get the correct serverblock (/ if no other found)
		getLocationSettings();
		//sanitize the path
		if (_input.errorFlag < 1)
			checkPath();
		//parse the full path together using information from the server block and check if the method is allowed
		if (_input.errorFlag < 1)
			parsePath();
		//handle the request 
		executeInput();

		//send a response
		if (_response.sendResponse(fd) == 1)
			_input.closeConnection = true;
	}
	catch(const std::exception& e)
	{
		Logger::log(e.what(), ERROR, false);
	}
	
}

void ServerHandler::checkPath()
{
	if (_input.path.empty())
		return Logger::setErrorAndLog(&_input.errorFlag, 400, "check-path: path is empty");
	std::regex validPathRegex("^[a-zA-Z0-9/_.-]+$");
	if (!std::regex_match(_input.path, validPathRegex) || _input.path.find("..") != std::string::npos)
		return Logger::setErrorAndLog(&_input.errorFlag, 400, "check-path: path syntax error");
}

void ServerHandler::responseCode(int code)
{
	_input.errorFlag = code;
}

int ServerHandler::checkMethod()
{
	if (locSettings->isCgiBlock())
		return 0;
	std::vector<int> allowedMethods = locSettings->getMethods();
	for (auto& method : allowedMethods)
	{
		if (_input.method == method)
			return 0;
	}
	return (1);
}

//tries to get the location settings by using location block matching rules, defaults to / if unsuccessful
void ServerHandler::getLocationSettings()
{
	std::string key = _input.path;
	int len = key.length();

	while (42)
	{
		locSettings = _input.settings->getLocationBlock(key);
		if (locSettings != nullptr || len < 2)
			break ;
		len--;
		if (len < 1)
			len = 1;
		key = key.substr(0, len);
	}
	if (!locSettings)
		locSettings = _input.settings->getLocationBlock("/");
	if (!locSettings)
		return Logger::setErrorAndLog(&_input.errorFlag, 500, "get-location-settings: failed to fetch location block :(");

}

void ServerHandler::parsePath()
{
	_baseInput = _input.path;

	if(locSettings->isRedirect() == true && _input.method == GET)
	{
		_response.setRedirect(true);
		_response.setLocation("Location: " + locSettings->getRedirectPath() + "\r\n");
		_input.errorFlag = locSettings->getRedirectStatus(); //UPDATED currently acccepting 300-308
		return ;
	}
	else
	{
		if (locSettings->getPath().length() > 1)
			_input.path = _input.path.substr(locSettings->getPath().length(), _input.path.length() - locSettings->getPath().length()); //remove the alias from the front of the URI
		if (locSettings->getPath().back() != '/' && _input.path.front() != '/')
			_input.path = '/' + _input.path;
		if (locSettings->getRoot().back() != '/' && _input.path.front() != '/')
			_input.path = locSettings->getRoot() + '/' + _input.path;
		else
			_input.path = locSettings->getRoot() + _input.path;
	}
	if (_input.path.length() > 1 && _input.path.at(0) == '/')
		_input.path = _input.path.substr(1, _input.path.length() -1);
	if (_input.path.back() == '/')
	{
		//use bool from config to see if there is a deafault file & if directory listing is on
		_input.path = _input.path + locSettings->getDefaultFile();
	}
	if (checkMethod())
		return Logger::setErrorAndLog(&_input.errorFlag, 405, "parse-path: method not allowed");
}


void ServerHandler::executeInput()
{
	if (_input.errorFlag > 0)
	{
		doError();
		return ;
	}
	else if (_input.method == POST)
		doPost();
	else if (_input.method == GET)
		doGet();
	else if (_input.method == DELETE)
		doDelete();
	else
		throw std::invalid_argument("execute-input: Invalid argument");
	if (_input.errorFlag > 0)
		doError();

}
void	ServerHandler::makeMIME()
{
		MIMEs = {
		//Text files
		{".html", "text/html"},
		{".txt", "text/plain"},
		{".css", "text/css"},
		{".js", "text/javascript"}, //could also be application/javascript

		//Image files
		{".webp", "image/webp"},
		{".jpg", "image/jpeg"},
		{".jpeg", "image/jpeg"},
		{".png", "image/png"},
		{".gif", "image/gif"},
		{".svg", "image/svg+xml"},
		{".ico", "image/x-icon"},

		//Video files
		{".mp4", "video/mp4"},
		{".webm", "video/webm"},

		//Audio files
		{".mp3", "audio/mpeg"},
		{".waw", "audio/waw"},

		//Application files
		{".json", "application/json"},
		{".xml", "application/xml"},
		{".pdf", "application/pdf"},
		{".zip", "application/zip"},
		{".gzip", "application/gzip"},
		//Font files
		{".woff", "font/woff"},
		{".woff2", "font/woff2"},
		{".ttf", "font/ttf"},
		{".otf", "font/otf"}
	};
}

void	ServerHandler::setContentType(std::string path)
{
	if (path.find(".") != std::string::npos)
	{
		//generating the key from the extension of the file
		std::string key = path.substr(path.find_last_of("."), path.length() - path.find_last_of("."));
		//makes the key lowercase before searching
		std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });

		for (const auto& pair : MIMEs)
		{
			if (key == pair.first)
			{
				_response.setContentType(pair.second);
				return ; 
			}
		}
	}

	//default if no file extension
	_response.setContentType("text/plain");
}

bool	ServerHandler::isReadable(std::string path)
{
	if (!std::filesystem::exists(path))
	{
		Logger::setErrorAndLog(&_input.errorFlag, 404, "is-readable: file does not exist");
		return false;
	}

	if (access(path.c_str(), R_OK) < 0)
	{
		Logger::setErrorAndLog(&_input.errorFlag, 403, "is-readable: no read access");
		return false;
	}
	return true;
}

int ServerHandler::getFile(std::string path)
{
	std::ostringstream	stream;
	std::fstream 		infile;

	if (!isReadable(path)) //checks if path is valid and that the file has read permissions
		return 1;
	infile.open(path);
	if (!infile.is_open())
		return 1;
	if (std::filesystem::file_size(path) > MAX_FILE_SIZE)
	{
		Logger::setErrorAndLog(&_input.errorFlag, 413, "Too large file requested");
		_response.setResponseCode(413);	
		return 1;
	}
	stream << infile.rdbuf();
	_response.set_body(stream.str());
	infile.close();
	_response.setContentLength(stream.str().length());
	setContentType(path);
	return (0);
}

//Generates a generic error response 
void ServerHandler::defaultError()
{
	std::string code = std::to_string(_input.errorFlag);
	std::string body;

	body = "	<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n    <title>" + code + "</title>\n</head>\n<body>\n    <h1>";
	body += code + " Error</h1>\n</body>\n</html>";

	_response.set_body(body);
	_response.setContentType("text/html");
	_response.setContentLength(body.length());
}

void ServerHandler::doError()
{
	_response.setResponseCode(_input.errorFlag);
	if (locSettings->isRedirect() == true)
		return ;
	std::string errorPath;
	//check if there are location level error pages for the requested code

	try
	{
		//please initialize locSettings before using it...
		errorPath = locSettings->getErrorPagePath(_input.errorFlag);
		if (!errorPath.empty())
		{
			if (errorPath.at(0) == '/')
				errorPath = errorPath.substr(1, errorPath.size() -1);
			//check that the path is valid
			if (getFile(errorPath) == 0)
				return;
		}
	}
	catch(const std::exception& e){}

	//if no location level pages, check if there is server level pages
	try
	{
		errorPath = _input.settings->getErrorPages(_input.errorFlag);
		if (!errorPath.empty())
		{
			if (errorPath.at(0) == '/')
				errorPath = errorPath.substr(1, errorPath.size() -1);
			//check that the path is valid
			if (getFile(errorPath) == 0)
				return;
		}
	}
	catch(const std::exception& e){}

	//and finally, if no pages found, generate default pages;
	defaultError();
}

int	ServerHandler::checkDirectorySize(std::filesystem::path path)
{
	size_t totalSize = 0;
	try
	{
		for (auto it : std::filesystem::recursive_directory_iterator(path))
		{
			if (std::filesystem::is_regular_file(it.status()))
				totalSize += std::filesystem::file_size(it);
			if (totalSize > MAX_DIRECTORY_SIZE)
				return (0);
		}
	}
	catch(const std::exception& e)
	{
		Logger::log("check directory size " + (std::string)e.what(), ERROR, false);
	}
	return (1);
}

void ServerHandler::doPost()
{
	std::filesystem::path path(_input.path);
	std::filesystem::path dirPath(_input.path.substr(0, _input.path.rfind('/')));
	//check if the path ends with / meaning its a directory
	if (_input.path.back() == '/')
		return Logger::setErrorAndLog(&_input.errorFlag, 403, "do-post: directory creation is forbidden");

	//first check if the directory where the file is / is to be created exists
	if(std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath))
	{
		//check that the maximum size for the directory is not yet reached
		if (!checkDirectorySize(dirPath))
			return Logger::setErrorAndLog(&_input.errorFlag, 507, "do-post: directory capacity exceeded");

		//then check if the file exists or not
		if(std::filesystem::exists(path))
		{
			 //to append to a existing file // will also create the file if it dose not exist also ensures there is write permission
			std::ofstream file(_input.path, std::ios::app);
			if (!file.is_open())
				return Logger::setErrorAndLog(&_input.errorFlag, 401, "do-post: no write permissions/file failed to open");
			file << _input.body;
			file.close();
			_response.setResponseCode(200);
		}
		else
		{
			std::ofstream new_file(_input.path);
			if (!new_file.is_open())
				return Logger::setErrorAndLog(&_input.errorFlag, 500, "do-post: failed to create file");
			new_file << _input.body;
			new_file.close();
			_response.setResponseCode(201);
			_response.setLocation("Location: " + _baseInput + "\r\n");
		}
	}
	else
		return Logger::setErrorAndLog(&_input.errorFlag, 405, "do-post: directory does not exist"); //creation of directory not allowed
}

void ServerHandler::generateIndex()
{
	std::filesystem::path dirPath(_input.path);
	std::string body;

	body = "	<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n    <title>" + _baseInput + " directory listing</title>\n</head>\n<body>\n    <h1>";
	body += _baseInput + " directory listing:</h1>\n<ul>";

	try
	{
		if(std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath))
		{
			for (const auto& file : std::filesystem::directory_iterator(dirPath))
			{
				if (file.is_directory())
					body += "    <li>[DIR] <a href=\"" + file.path().filename().string() + "/\">" + file.path().filename().string();
				else
					body += "    <li>[FILE] <a href=\"" + file.path().filename().string() + "\">" + file.path().filename().string();
				body += "</a></li>\n";
			}
		}
	}
	catch(const std::exception& e)
	{
		return Logger::setErrorAndLog(&_input.errorFlag, 403, "generate-index: directory index creation failed"); //probably other code
	}
	body += "</ul>\n</body>\n</html>";

	_response.set_body(body);
	_response.setContentType("text/html");
	_response.setContentLength(body.length());
	_response.setResponseCode(200);
}

void ServerHandler::doGet()
{
	//check if it is asking for a directory and if autoindex is on...
	//if so, generate the directory index
	if ((_input.path.back() == '/' || std::filesystem::is_directory(_input.path) )&& locSettings->isAutoIndex() == true)
	{
		if (std::filesystem::is_directory(_input.path))
			generateIndex();
		else
			Logger::setErrorAndLog(&_input.errorFlag, 404, "do-get: Directory dose not exist");	
		return;
	}
	//check if the request is a directory, and return a 301 permanently moved with a / if so
	if (std::filesystem::is_directory(_input.path))
	{
		// _response.setRedirect(true);
		// _response.setLocation("Location: " + _baseInput + "/\n");
		// _input.errorFlag = 301;
		_input.errorFlag = 403;
		return ;
	}

	if (getFile(_input.path) == 1)
	{
		if (_input.errorFlag < 1)
			Logger::setErrorAndLog(&_input.errorFlag, 404, "do-get: file does not exist");
		return ;
	}
	_response.setResponseCode(200);
}

void ServerHandler::doDelete()
{
	std::filesystem::path path(_input.path);
	std::filesystem::path dirPath(_input.path.substr(0, _input.path.rfind('/')));
	//check if the path ends with / meaning its a directory
	if (_input.path.back() == '/' || std::filesystem::is_directory(_input.path))
		return Logger::setErrorAndLog(&_input.errorFlag, 403, "do-delete: directory deletion is forbidden");

	//first check if the directory where the file is exists	
	if(std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath))
	{	
		//then check if the file exists or not
		if(std::filesystem::exists(path))
		{
			try
			{
				if (!std::filesystem::remove(path))
					return Logger::setErrorAndLog(&_input.errorFlag, 403, "do-delete: file failed to delete");
			}
			catch(const std::exception& e)
			{
				Logger::log(e.what(), ERROR, false);
			}
		}
		else
			return Logger::setErrorAndLog(&_input.errorFlag, 404, "do-delete: file does not exist");
		_response.setResponseCode(200);
	}
	else
		Logger::setErrorAndLog(&_input.errorFlag, 404, "do-delete: directory does not exist");
}
