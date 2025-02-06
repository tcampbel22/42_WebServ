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

#include "Response.hpp"
#include <iomanip>
#include <ctime>
#include <sys/socket.h>

Response::Response(): 
_responseCode(999), _contentLength(0)
{
}

Response::Response(int errorCode): 
_responseCode(errorCode)
{
}

Response::Response(int responseCode, uint contentLenght, std::string body, bool closeConnection, bool _redirect)
: _responseCode(responseCode), _contentType("text/html"), _contentLength(contentLenght), _body(body), _closeConnection(closeConnection), _redirect(_redirect) {

}

void Response::setResponseCode(uint _code){_responseCode = _code;}

void Response::setContentType(std::string _content){_contentType = _content;}

void Response::setContentLength(uint _length){_contentLength = _length;}

void Response::set_body(std::string _newBody){_body = _newBody;}

void Response::setCloseConnection(bool _close){_closeConnection = _close;}

void Response::setRedirect(bool _redir){_redirect = _redir;}

uint Response::getResponseCode(){return (_responseCode);}

void Response::setLocation(std::string _newLocation){_location = _newLocation;}

std::string Response::getResponseCodeStr()
{
	std::string status = "HTTP/1.1 ";
	switch (_responseCode)
	{
	case 200:
		return(status += "200 OK\r\n");
	case 301:
		return(status += "301 Moved Permanently\r\n");
	case 302:
		return(status += "302 Found\r\n");
	case 400:
		return(status += "400 Bad Request\r\n");
	case 401:
		return(status += "401 Unauthorized\r\n");
	case 403:
		return(status += "403 Forbidden\r\n");		
	case 404:
		return(status += "404 Not Found\r\n");
	case 413:
		return(status += "413 Payload Too Large\r\n");				
	case 500:
		return(status += "500 Internal Server Error\r\n");
	case 502:
		return(status += "502 Bad Gateway\r\n");		
	case 503:
		return(status += "503 Service Unavailable\r\n");
	case 504:
		return (status += "504 Gateway Timeout\r\n");	
	case 507:
		return(status += "507 Insufficient Storage\r\n");
	default:
		return(status += std::to_string(_responseCode) + "\r\n");
	}
}

std::string Response::getContentType()
{
	std::string _content = "Content-Type: ";
	_content += _contentType + "\r\n";
	return (_content);
}

std::string Response::getContentLength()
{
	std::string _length = "Content-Length: ";
	_length += std::to_string(_contentLength) + "\r\n";
	return (_length);
}

std::string Response::makeDay(int _day)
{	
	switch (_day)
	{
	case 1:
		return ("Mon");
	case 2:
		return ("Tue");
	case 3:
		return ("Wed");
	case 4:
		return ("Thu");
	case 5:
		return ("Fri");
	case 6:
		return ("Sat");
	case 0:
		return ("Sun");		
	default:
		return ("inv");
		break;
	}
}

std::string Response::makeMonth(int _month)
{	
	switch (_month)
	{
	case 0:
		return ("Jan");
	case 1:
		return ("Feb");
	case 2:
		return ("Mar");
	case 3:
		return ("Apr");
	case 4:
		return ("May");
	case 5:
		return ("Jun");
	case 6:
		return ("Jul");	
	case 7:
		return ("Aug");
	case 8:
		return ("Sep");
	case 9:
		return ("Oct");
	case 10:
		return ("Nov");
	case 11:
		return ("Dec");			
	default:
		return ("inv");
		break;
	}
}

std::string Response::makeDate()
{
	//get the current time from system
	std::time_t _time;
	std::time(&_time);

	//break the time down to its components, and convert to GMT time
	std::tm *_gmtTime = std::gmtime(&_time);
	
	//Date: <day>, <day-of-month> <month> <year> <hour>:<minute>:<second> GMT
	std::string _date = "Date: " + makeDay(_gmtTime->tm_wday) + ", " + std::to_string(_gmtTime->tm_mday) \
		+ " " + makeMonth(_gmtTime->tm_mon) + " " +  std::to_string(_gmtTime->tm_year + 1900) + " " + std::to_string(_gmtTime->tm_hour) \
		+ ":" +std::to_string(_gmtTime->tm_min) + ":" + std::to_string(_gmtTime->tm_sec) + " GMT\r\n";
	
	return (_date);
}

int Response::sendResponse(int fd)
{
	std::string _buffer = getResponseCodeStr();
	
	if (!_contentType.empty())
		_buffer += getContentType();
	_buffer += getContentLength();
	if (_closeConnection == true)
		_buffer += "Connection: close\r\n";
	if (_redirect == true || getResponseCode() == 201)
		_buffer += _location;
	_buffer += makeDate();
	_buffer += "\r\n";

	if (!_body.empty())
		_buffer += _body;
	
	//uncomment this in order to see the response in the terminal
	// std::cout <<  _buffer << std::endl;
	
	try
	{
		if (getResponseCode() == 200)
			Logger::log("[200] Great Success!", INFO, false);
			//if send receives 0 or -1, return 1 in order to close the connection with the client
		if (send(fd, _buffer.c_str(), _buffer.size(), 0) <= 0)
			return 1;
	}
	catch(const std::exception& e)
	{
		Logger::log(e.what(), ERROR, false);
	}
	return 0;
}
