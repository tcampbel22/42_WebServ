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
#include "../Server/HttpServer.hpp"
#include "../HttpParsing/HttpParser.hpp"
#include "../Config/LocationSettings.hpp"
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <thread>


#define READ_END 0
#define WRITE_END 1

struct HttpRequest;
class LocationSettings;

class CGIparsing
{
private:
	std::string _scriptName; //path where to upload the cgi file
	std::string* _execInfo;
public:
	CGIparsing(std::string&, std::string&);
	~CGIparsing();
	void setCGIenvironment(HttpRequest&, HttpParser&);
	void execute(HttpRequest&, int, epoll_event&, HttpServer&, fdNode *requestNode, HttpParser&);
	std::string getMethod(int);
	std::string getIp(std::string&);
	std::string getPort(std::string&);
	std::string getPath();
	// void CGIsignalHandler(int signal);
};

