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

#include "requestLineValidator.hpp"

const std::unordered_set<std::string> requestLineValidator::_validMethods = {"GET", "POST", "DELETE"};

std::vector<std::string> split(const std::string& tmp, char delimiter) {
    std::vector<std::string> array;
    std::string token;
    std::stringstream stream(tmp);

    while (std::getline(stream, token, delimiter)) {
        array.push_back(token);
    }
    return array;
}

bool validatePath(std::string tmp) {
	std::vector<std::string> result = split(tmp, '/');

	 for (const auto& array : result) {
        if (array.length() > 254) {
			return false;
		}
    }
	return true;
}

bool requestLineValidator::isValidRequestLine(std::string rLine, HttpRequest& request)
{
	std::string tmp;
	size_t startPos;
	
		size_t spPos = rLine.find(' ');    //find the first space in the RL and check that it's either GET, POST or DELETE request. Anything else it's false
		if (spPos == std::string::npos) {
			Logger::setErrorAndLog(&request.errorFlag, 405, "request-line: syntax error");
			return false;
		}
		
		tmp = rLine.substr(0, rLine.find(' '));
		if (_validMethods.find(tmp) == _validMethods.end()) {
			if (!request.errorFlag)
				Logger::setErrorAndLog(&request.errorFlag, 405, "request-line: invalid method"); //error response here (error 404 bad request or 500 internal server error)
			return false;
		}
		if (!tmp.compare("GET"))
			request.method = GET;
		if (!tmp.compare("POST"))
			request.method = POST;
		if (!tmp.compare("DELETE"))
			request.method = DELETE;
		startPos = spPos + 1;
		spPos = rLine.find(' ', startPos);
		if (spPos == std::string::npos)
			return false;

		tmp = rLine.substr(startPos, spPos - startPos);
		if (tmp.empty() ||  tmp[0] != '/' || tmp.size() >= MAX_HEADER_SIZE || !validatePath(tmp)) {
			if (tmp.size() >= MAX_HEADER_SIZE || !validatePath(tmp))
				Logger::setErrorAndLog(&request.errorFlag, 414, "request-line: URI too long");
			if(!validatePath(tmp)) {
				Logger::setErrorAndLog(&request.errorFlag, 414, "request-line: file name too long");
			}
			if (!request.errorFlag)
				Logger::setErrorAndLog(&request.errorFlag, 400, "request-line: incorrect path"); //if Path is incorrect: error handling here(HTTP Status 400 or HTTP Status 404).
			return false;
		}
		//checks if there are conscutive / in the path and normalizes them to 1 / if so
		normalizePath(tmp);	
		request.path = tmp;
		
		startPos = spPos + 1;
		tmp = rLine.substr(startPos, spPos - startPos);     //Version detection, has to be *HTTP/1.1\r*
		if (tmp != "HTTP/1.1\r") {
			if (!request.errorFlag)
				Logger::setErrorAndLog(&request.errorFlag, 505, "request-line: invalid HTTP version"); //error shit here if version wrong(HTTP 505 - HTTP Version Not Supported)
			return false;
		}
	return true;
}

void	requestLineValidator::normalizePath(std::string &path)
{
	size_t pos;

	while ((pos = path.find("//")) != std::string::npos)
		path.erase(pos, 1);
}	
