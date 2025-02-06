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

#include "HttpParser.hpp"
#include "HttpHeaderParser.hpp"
#include "requestLineValidator.hpp"
#include "chunkedBodyParser.hpp"
#include "../CGI/CGIparsing.hpp"
#include "../Response/Response.hpp"

HttpParser::HttpParser() : _contentLength(0), cgiflag(false), query(""), pathInfo("") {}

HttpParser::~HttpParser() {}

HttpRequest::HttpRequest(ServerSettings *serverPtr, int fd, epoll_event& _event) : closeConnection(false), errorFlag(0), settings(serverPtr), isCGI(false), epollFd(fd), events(_event)  {
	this->path = "";
}

//Empty the vector to the requestMap, needs to be parsed in the response.
void HttpParser::parseClientRequest(const std::vector<char>& clientData, HttpRequest& request, ServerSettings *serverPtr)
{
	try {
		std::string data(clientData.begin(), clientData.end());
		std::istringstream requestStream(data);
    	std::string line;
		//Parse the requestline and store the relevant stuff (path and method)
		if (!std::getline(requestStream, line) || !requestLineValidator::isValidRequestLine(line, request)) {
			if (requestStream.bad() || requestStream.fail())
				Logger::log("parseClientRequest: could not read request line", ERROR, false);
			else
				Logger::log("parseClientRequest: request line is invalid", ERROR, false);
			return;
		}
		HttpHeaderParser::parseHeaders(requestStream, request);
		HttpHeaderParser::procesHeaderFields(request, this->_contentLength);
		if (!HttpHeaderParser::HostParse(serverPtr, request) && !request.errorFlag) {
		 	Logger::setErrorAndLog(&request.errorFlag, 400, "header: host name error");
		 	request.closeConnection = true;
		}
		if (request.method == GET && !request.errorFlag)
		{
			std::getline(requestStream, line);
			if (!requestStream.eof()) {
				Logger::setErrorAndLog(&request.errorFlag, 400, "header: body in GET request");
				return;
			}
			if (this->_contentLength != 0) {
				Logger::setErrorAndLog(&request.errorFlag, 400, "header: content-length not zero in GET request");
				return;
			}
		}
		if (!request.errorFlag)
			parseCGI(request);
		if ((_contentLength || request.headers.find("Transfer-Encoding") != request.headers.end()) && !request.errorFlag) {
			parseBody(request, requestStream);
		}
		} catch (std::exception& e) {
			Logger::log(e.what(), ERROR, false);
		}
}

void HttpParser::parseBody(HttpRequest& request, std::istringstream& stream) {
        if (request.headers.count("Transfer-Encoding") == std::string::npos && _contentLength == 0)
			return ;
		auto it = request.headers.find("Transfer-Encoding");
        if (it != request.headers.end() && it->second.find("chunked") != std::string::npos) {
            chunkedBodyParser::parseChunkedBody(stream, request);
        } else {
            parseRegularBody(stream, request);
        }
}

void HttpParser::parseRegularBody(std::istringstream& stream, HttpRequest& request) {
	char c;
	for (int i = 0; i < _contentLength + 1 && stream.get(c); i++) {
		request.body += c;
	}
	if (!stream.eof()) {
		if (!request.errorFlag)
			Logger::setErrorAndLog(&request.errorFlag, 400, "body: no eof in body");
		return ;
	}
	if (_contentLength != (int)request.body.size())
	{
		if (!request.errorFlag)
			Logger::setErrorAndLog(&request.errorFlag, 400, "body: content-length mismatched with body size");
	}
}

void checkHeaderError(const std::vector<char> clientData, HttpRequest& request) {
	std::string tmp(clientData.begin(), clientData.end());
	
	if (tmp.find(' ') == std::string::npos) {
			Logger::setErrorAndLog(&request.errorFlag, 405, "request-line: invalid method");
	}
	else if (tmp.find(' ') != std::string::npos)
	{
		if (tmp.find("HTTP/1.1") == std::string::npos)
			Logger::setErrorAndLog(&request.errorFlag, 414, "request-line: too long URI");
		else
			request.errorFlag = 431;
	}
}

int	HttpParser::bigSend(fdNode *requestNode, int epollFd, epoll_event &_events, HttpServer& server) 
{
	HttpParser parser;
	HttpRequest request(requestNode->serverPtr, epollFd, _events);
	request.errorFlag = requestNode->_error;
	if (requestNode->CGIReady == true)
	{
		request.errorFlag = requestNode->CGIError;
		if (request.errorFlag == 0)
		{
			request.body = requestNode->CGIBody;
			request.method = requestNode->method;
			request.path = requestNode->path;
			Response response(200, request.body.size(), request.body, request.closeConnection, false);
			response.sendResponse(requestNode->fd);
			return (0);
		}
		else
		{
			ServerHandler Response(requestNode->fd, request);
			return (1);
		}
	}
	else
	{
		if (!request.errorFlag)
			parser.parseClientRequest(requestNode->_clientDataBuffer, request, requestNode->serverPtr);
		if (parser.cgiflag && !request.errorFlag)
		{
			LocationSettings* cgiBlock = request.settings->getLocationBlock(parser.currentCGI);
			if (cgiBlock && request.method != 3 && requestNode->cgiStarted == false)
			{
				CGIparsing myCgi(parser.cgiPath, cgiBlock->getCgiScript());
				requestNode->path = request.path;
				requestNode->method = request.method;
				myCgi.setCGIenvironment(request, parser);
				myCgi.execute(request, epollFd, _events, server, requestNode, parser);
				return (0);
			}
			else {
				Logger::setErrorAndLog(&request.errorFlag, 400, "big send: cgi path not found");
				return (1);
			}
		}
	}
	if (requestNode->cgiStarted == false)
		ServerHandler response(requestNode->fd, request);
	else
		return (0);
	if (request.closeConnection == true)
		return (1);
	else
		return (0);
}



void	HttpParser::validateCGIPath(LocationSettings& block, int* error)
{
	std::string abs_path;
	if (block.getCgiPath().back() != '/')
		abs_path = block.getCgiPath() + "/" + block.getCgiScript();
	else
		abs_path = block.getCgiPath() + block.getCgiScript();
	if (block.getCgiScript().length() != cgiPath.length()) // Check if there is more in the URI
	{
		pathInfo = cgiPath; //Preserves original path
		if (cgiPath[cgiPath.find(".py") + 3] == '/') 
			cgiPath.erase(cgiPath.find(".py") + 3); //Isolates the path up until the script
	}
	if (cgiPath.back() != '/' && (cgiPath.compare(abs_path) || cgiPath.find(".py") == std::string::npos))
	{
		Logger::setErrorAndLog(error, 404, "validate-cgi-path: invalid cgi file " + cgiPath);
		return ;
	}
	if (cgiPath.back() == '/')
		cgiflag = false;
	else
	{
		currentCGI = block.getPath();
		cgiflag = true;
	}
}

void	HttpParser::parseQueryString(std::string& path, int *error)
{
	size_t pos = path.find('?');
	if (pos != std::string::npos)
	{
		if (std::count(path.begin(), path.end(), '?') != 1)
			Logger::setErrorAndLog(error, 400, "cgi: Bad query string: " + path);
		query = path.substr(path.find('?') + 1);
		path.erase(pos);
	}
}

void	HttpParser::formatCGIPath(std::string& request_path, LocationSettings& block, HttpRequest& request)
{
	if (request_path.compare(block.getCgiScript()))
	{
		cgiPath = request_path;
		cgiPath.erase(0, block.getPath().length()); //extract the file
		if (cgiPath.front() != '/' && block.getCgiPath().back() != '/') //Add slash too beginning of path
			cgiPath.insert(0, "/");
		cgiPath.insert(0, block.getCgiPath()); // add path from config to get full path
	}
	else
		cgiPath = block.getCgiScript();
	parseQueryString(cgiPath, &request.errorFlag);
	if (!request.errorFlag)
		validateCGIPath(block, &request.errorFlag);
}

void	HttpParser::parseCGI(HttpRequest& request)
{
	std::string key = request.path; //Request path eg cgi-bin/pytester.py
	int len = key.length();
	LocationSettings *locSettings = nullptr;
	while (1) // Check URI against location block to see if it matches a cgi block
	{
		locSettings = request.settings->getLocationBlock(key);
		if (locSettings != nullptr || len < 2)
			break ;
		len--;
		if (len < 1)
			len = 1;
		key = key.substr(0, len);
	}
	if (!locSettings)
		locSettings = request.settings->getLocationBlock("/");
	if (!locSettings)
		return ;
	if (locSettings->isCgiBlock()) {
		if (request.method != 2 && request.method != 1 )
		{
			Logger::setErrorAndLog(&request.errorFlag, 405, "cgi: Method not allowed");
			return ;
		}	
		formatCGIPath(request.path, *locSettings, request);
		if (request.errorFlag)
			return ;
		if (request.method == 2 && _contentLength == 0) {
			Logger::setErrorAndLog(&request.errorFlag, 411, "cgi: no contetent for POST");
			return ;
		}
		if (request.headers.find("Content-Type") != request.headers.end()) {
			if (request.headers.at("Content-Type") != "application/x-www-form-urlencoded") {
				Logger::setErrorAndLog(&request.errorFlag, 415, "cgi: invalid type");
				return ;
			}
		}
	}
}

std::string HttpParser::getQuery() { return query; }
std::string HttpParser::getPathInfo() { return pathInfo; }
uint HttpParser::getContentLength() { return _contentLength; }
std::string HttpParser::getCgiPath() { return cgiPath; }

HttpRequest::~HttpRequest() {}