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

#include "HttpParser.hpp"

class HttpHeaderParser
{
public:
	static void parseHeaders(std::istringstream& requestStream, HttpRequest& request);
	static void procesHeaderFields(HttpRequest& request, int& contentLength);
	static bool HostParse(ServerSettings*, HttpRequest&);
	~HttpHeaderParser();
};

std::string trim(const std::string& str);
