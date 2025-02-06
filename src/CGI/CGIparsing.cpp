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

#include "CGIparsing.hpp"
#include "../HttpParsing/HttpParser.hpp"

CGIparsing::CGIparsing(std::string& root, std::string& script) 
{
	_scriptName = script;
	_execInfo = new std::string("." + root);
}

void CGIparsing::setCGIenvironment(HttpRequest& request, HttpParser& parser) {
	setenv("REQUEST_METHOD", getMethod(request.method).c_str(), 1);
	setenv("QUERY_STRING", parser.getQuery().c_str(), 1);
	if (request.headers.find("Content-Type") != request.headers.end())
		setenv("CONTENT_TYPE", request.headers.at("Content-Type").c_str(), 1); //default text, needs parsing for images etc.
	if (request.headers.find("Content-Length") != request.headers.end())
		setenv("CONTENT_LENGTH", request.headers.at("Content-Length").c_str(), 1);
	setenv("PATH_INFO", parser.getPathInfo().c_str(), 1);
	setenv("SERVER_NAME", request.headers.at("Host").c_str(), 1);
	setenv("SERVER_PORT", getPort(request.host).c_str(), 1);
	setenv("REMOTE_ADDR", getIp(request.host).c_str(), 1);
}

std::string CGIparsing::getMethod(int val) {
	switch (val)
	{
	case 1:
		return "GET";
	case 2:
		return "POST";
	default:
		break;
	}
	return (NULL);
}

std::string CGIparsing::getIp(std::string& host) {
	size_t colPos = host.find(':');
	if (colPos != std::string::npos)
		return host.substr(0, colPos);
	return nullptr;	
}

std::string CGIparsing::getPort(std::string& host) {
	size_t colPos = host.find(':');
	if (colPos != std::string::npos)
		return host.substr(colPos + 1);
	return nullptr;
}

bool	setToNonBlocking(int socket)
{
	int	flag = fcntl(socket, F_GETFL, 0); //retrieves flags/settings from socket
	if (flag < 0)
	{
		Logger::log("fcntl: GETFL failed", ERROR, true);
		return false;
	}
	if (fcntl(socket, F_SETFL, flag | O_NONBLOCK) < 0) //Sets socket to be nonblocking
	{
		Logger::log("fcntl: SETFL failed", ERROR, true); 
		return false; 
	}
	return true;
}

void CGIparsing::execute(HttpRequest& request, int epollFd, epoll_event& _events, HttpServer& server, fdNode *requestNode, HttpParser& parser) 
{
    // Create a pipe
	if (pipe(requestNode->pipe_fds) == -1) 
	{
        Logger::setErrorAndLog(&requestNode->CGIError, 500, "pipe: failed to create pipe");
		return ;
    }
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, requestNode->pipe_fds[WRITE_END], &_events) == -1)
	{
		Logger::setErrorAndLog(&requestNode->CGIError, 504, "epoll_ctl: failed to add to epoll");
		requestNode->CGIReady = true;
		return ;
		
	}
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, requestNode->pipe_fds[READ_END], &_events) == -1)
	{
		Logger::setErrorAndLog(&requestNode->CGIError, 504, "epoll_ctl: failed to add to epoll");
		requestNode->CGIReady = true;
		return ;
		
	}
	if (!setToNonBlocking(requestNode->pipe_fds[WRITE_END]) || !setToNonBlocking(requestNode->pipe_fds[READ_END]))
	{
		Logger::setErrorAndLog(&requestNode->CGIError, 504, "setToNonBlocking: ");
		requestNode->CGIReady = true;
		return ;
	}
    // Fork the child process
    requestNode->pid = fork();
    if (requestNode->pid == -1) 
	{
        Logger::setErrorAndLog(&requestNode->CGIError, 500, "fork: failed to fork");
        return ;
    }
    // Child process
    if (requestNode->pid == 0) 
	{

		// Redirect stdout to the write end of the pipe
        if (dup2(requestNode->pipe_fds[WRITE_END], STDOUT_FILENO) == -1) 
		{
            Logger::log("dup2: failed", ERROR, false);
			server.cleanUpChild(requestNode);
            exit(1);
        }
		if (dup2(requestNode->pipe_fds[READ_END], STDIN_FILENO) == -1) 
		{
            Logger::log("dup2: failed", ERROR, false);
			server.cleanUpChild(requestNode);
            exit(1);
        }
        close(requestNode->pipe_fds[READ_END]);

        // Close the write end of the pipe now that it's duplicated
		if (request.method == 2) 
		{  
			ssize_t bytesRecieved = write(requestNode->pipe_fds[WRITE_END], request.body.c_str(), request.body.size());
			close(requestNode->pipe_fds[WRITE_END]);
            if (bytesRecieved == -1 || bytesRecieved == 0)
			{
				server.cleanUpChild(requestNode);
				exit(1);
			}
        }
		const char *const argv[] = {_scriptName.c_str(), nullptr};
		if (execve(_execInfo->c_str(), (char *const *)argv, environ) == -1) 
		{
            Logger::log("execve: failed to execute command", ERROR, false);
			delete _execInfo;
			request.~HttpRequest();
			parser.~HttpParser();
			server.cleanUpChild(requestNode);
			exit(1);
        }

    } 
	else 
	{
		requestNode->cgiStarted = true;
        // Parent process
		if (epoll_ctl(epollFd, EPOLL_CTL_DEL, requestNode->pipe_fds[WRITE_END], nullptr) == -1)
		{
			Logger::log("epoll_ctl: failed to delete fd", ERROR, false);
			close(requestNode->pipe_fds[READ_END]);
		}
        // Close the write end of the pipe since the parent will only read from the pipe
		close(requestNode->pipe_fds[WRITE_END]);
    }
}

std::string CGIparsing::getPath() {
	return _scriptName;
}

CGIparsing::~CGIparsing() { delete _execInfo; }