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
# include "../Server/HttpServer.hpp"
# include "../Response/ServerHandler.hpp"
# include "../Config/LocationSettings.hpp"
# include <iostream>
# include <sstream>
# include <fstream>
# include <map>
# include <unordered_set>
# include <string>
# include <memory>
# include <filesystem>

struct fdNode;
class ServerSettings;
class LocationSettings;
class HttpServer;


struct HttpRequest {
	int	method;
	std::string path;
	std::map<std::string, std::string> headers;
	std::string body;
	std::string host;
	bool closeConnection = false;
	int errorFlag = 0;
	ServerSettings* settings;
	HttpRequest(ServerSettings *, int, epoll_event&);
	bool isCGI;
	int epollFd;
	epoll_event& events;
	~HttpRequest();
};


class HttpParser
{
private:
	std::vector<char> _clientDataBuffer;
	int _contentLength;
	bool cgiflag;
	std::string query;
	std::string cgiPath;
	std::string pathInfo;
	std::string currentCGI;
public:
	HttpParser();
	~HttpParser();
	static int	bigSend(fdNode*, int, epoll_event&, HttpServer&);
	void parseClientRequest(const std::vector<char>& clientData, HttpRequest& request, ServerSettings *);
	void parseBody(HttpRequest&, std::istringstream&);
	void parseRegularBody(std::istringstream&, HttpRequest&);
	void checkForCgi(HttpRequest&, LocationSettings&);
	uint getContentLength();
	int isBlockCGI(HttpRequest&);
	std::string getQuery();
	std::string getPathInfo();
	std::string getCgiPath();
	void	validateCGIPath(LocationSettings& block, int* error);
	void	parseQueryString(std::string& path, int *error);
	void	formatCGIPath(std::string& request_path, LocationSettings& block, HttpRequest& request);
	void	parseCGI(HttpRequest& request);
};