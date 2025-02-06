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
#include <iostream>
#include "../../include/webserv.hpp"

class Response
{
private:
	uint _responseCode = 999;
	std::string _contentType;
	uint _contentLength = 0;
	std::string _body;
	bool _closeConnection = false;
	bool _redirect = false;
	std::string _location;

public:	
	Response();
	Response(int responseCode, uint contentLenght, std::string body, bool closeConnection, bool _redirect);
	Response(int errorCode);
	~Response(){};

	//Setters
	void setResponseCode(uint _code);
	void setContentType(std::string _content);
	void setContentLength(uint _length);
	void set_body(std::string _body);
	void setCloseConnection(bool _close);
	void setRedirect(bool _redir);
	void setLocation(std::string _newLocation);

	//Methods
	std::string getContentLength();
	std::string getContentType();
	uint getResponseCode();
	std::string getResponseCodeStr();	
	std::string makeDate();
	int sendResponse(int fd);
	std::string makeDay(int day);
	std::string makeMonth(int _month);
};
