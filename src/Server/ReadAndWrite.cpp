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

#include "HttpServer.hpp"

bool	HttpServer::handle_read(fdNode* nodePtr)
{
	readRequest(nodePtr);
	if (requestComplete)
	{
		if (nodePtr->_clientDataBuffer.empty())
			cleanUpFds(nodePtr);
		else
		{
			nodePtr->_readyToSend = true;
			_events.events = EPOLLOUT;
			if (epoll_ctl(epollFd, EPOLL_CTL_MOD, nodePtr->fd, &_events) == -1)		
			{
				Logger::log("Failed to mod epoll", ERROR, false);
				cleanUpFds(nodePtr);
				return false; 
			}
		}
		return true;
	}
	else
	{
		Logger::log("Waiting for more data...", INFO, false);
		return true;
	}
}

//Read data from client stream
void	HttpServer::readRequest(fdNode *nodePtr)
{
	int _fd_out = nodePtr->fd;

	ssize_t bytesReceived = 0;
	ssize_t bytes = 1024;
	requestComplete = false;

		nodePtr->_clientDataBuffer.resize(nodePtr->_clientDataBuffer.size() + bytes);
		bytesReceived = recv(_fd_out, &nodePtr->_clientDataBuffer[nodePtr->_clientDataBuffer.size() - bytes], bytes, 0);
		Logger::log("Bytes recieved: " + std::to_string(bytesReceived), INFO, false);
		if (nodePtr->_clientDataBuffer.size() >= MAX_HEADER_SIZE) 
		{
			if (!nodePtr->headerCorrect && nodePtr->_clientDataBuffer.size() >= MAX_HEADER_SIZE) 
			{
				validateHeaders(nodePtr->_clientDataBuffer, &nodePtr->_error);
				if (nodePtr->_error != 0) 
				{
					requestComplete = true;
					nodePtr->headerCorrect = true;
					_clientClosedConn = true;
					return ;
				}
				nodePtr->headerCorrect = true;
			}
		}
		if (bytesReceived < bytes) 
		{
			int temp = bytesReceived;
			if (bytesReceived < 0)
				temp = 0;
			nodePtr->_clientDataBuffer.resize(nodePtr->_clientDataBuffer.size() - (bytes - temp));
			requestComplete = isRequestComplete(nodePtr->_clientDataBuffer, nodePtr->_clientDataBuffer.size(), nodePtr);
		}
		if (bytesReceived < 0)
		{
			if (isNonBlockingSocket(nodePtr->fd)) //check if there is an error with recv
			{
				Logger::log("recv: failed to read, better check ERRNO :/", ERROR, false);
				requestComplete = isRequestComplete(nodePtr->_clientDataBuffer, nodePtr->_clientDataBuffer.size(), nodePtr);
			}
		}
		else if (bytesReceived == 0) //read is successful and client closes connection
		{
			Logger::log("Client closed the connection.", INFO, false);
			requestComplete = true;
			_clientClosedConn = true;
		}
		else
			requestComplete = isRequestComplete(nodePtr->_clientDataBuffer, nodePtr->_clientDataBuffer.size(), nodePtr);
}

bool HttpServer::handle_write(fdNode* nodePtr)
{
	if (nodePtr && nodePtr->cgiStarted == true)
	{	
		if (HttpServer::checkCGI(nodePtr) == 1)
		{
			if (HttpParser::bigSend(nodePtr, epollFd, _events, *_instance) || _clientClosedConn == true)
			{ 
				if (nodePtr)
					cleanUpFds(nodePtr);
			}
			else
			{
				if (!resetCGI(nodePtr))
					return false;
			}
		}
	}
	else if (HttpParser::bigSend(nodePtr, epollFd, _events, *_instance) || _clientClosedConn == true) // Once we have the full data, process the request
	{
		if (nodePtr)
			cleanUpFds(nodePtr);
	}
	else if (nodePtr->cgiStarted == false)
	{
		_events.events = EPOLLIN;
		if (epoll_ctl(epollFd, EPOLL_CTL_MOD, nodePtr->fd, &_events) == -1)		
		{
			Logger::log("Failed to mod epoll", ERROR, false);
			cleanUpFds(nodePtr);
			return false; 
		}
		resetNode(nodePtr);
	}
	return true;
}

// Function to check if the request is fully received (for chunked encoding or complete body)
bool HttpServer::isRequestComplete(const std::vector<char>& data, ssize_t bytesReceived, fdNode* node)
{
    std::string requestStr(data.begin(), data.end());
	bool isChunked = isChunkedTransferEncoding(requestStr);
	if (isChunked) {
		if (requestStr.find("0\r\n\r\n") != std::string::npos) {  // End of chunked data
        	return true;
   		}
		else
			return false;
	}
	bool hasBody = isRequestWithBody(requestStr);
	if (hasBody) {
		int complete = getContentLength(requestStr);
		if (complete < 0 || !validateContentLength(node, complete))
			return true;
		size_t test = requestStr.find("\r\n\r\n") + 4;
		if ((requestStr.find("\r\n\r\n", test) != std::string::npos) || (bytesReceived - test == (size_t)complete))
			return true;
		else
			return false;
	}
	if (!isChunked && !hasBody) 
	{
		if (requestStr.find("\r\n\r\n") != std::string::npos)  // End of nonBody data
			return true;
		else
			return false;
	}
    return false;
}

int HttpServer::checkCGI(fdNode *requestNode)
{
	char 	buffer[1024]; //CGI buffer
	ssize_t bytesRead = 0; //for CGI reading

	if (requestNode == nullptr)
		return (0);
	int status;
	pid_t result = waitpid(requestNode->pid, &status, WNOHANG);
	if (!requestNode)
	 return 0;
	if (result == requestNode->pid)
	{
		if (WIFEXITED(status))
		{
			if(WEXITSTATUS(status))
			{
				Logger::setErrorAndLog(&requestNode->CGIError, 502, "child process failed");
				requestNode->CGIReady = true;
				if (epoll_ctl(epollFd, EPOLL_CTL_DEL, requestNode->pipe_fds[READ_END], nullptr) == -1)
				{
					Logger::log("epoll_ctl: failed to delete fd from epoll", ERROR, false);
					close(requestNode->pipe_fds[READ_END]);
				}
				close(requestNode->pipe_fds[READ_END]);
				return (1);
			}
		}
	}
	else
		return (0);
	requestNode->CGIReady = true;
	// Read the output from the child process
	requestNode->CGIBody.clear();
	while ((bytesRead = read(requestNode->pipe_fds[READ_END], buffer, sizeof(buffer) - 1)) > 0) {
		buffer[bytesRead] = '\0'; // Null-terminate the output
		// printf("CGI Output: %s", buffer); // Or store it in a variable if needed
		requestNode->CGIBody += buffer;
		if (requestNode->CGIBody.size() > MAX_BODY_SIZE)
		{		
			Logger::setErrorAndLog(&requestNode->CGIError, 413, "too large body from child");
			break ;
		}
	}
	if (requestNode != nullptr || requestNode->fd != -1)
	{
		if (epoll_ctl(epollFd, EPOLL_CTL_DEL, requestNode->pipe_fds[READ_END], nullptr) == -1)
		{
			Logger::setErrorAndLog(&requestNode->CGIError, 500, "epoll_ctl: failed to delete fd from epoll");
			close(requestNode->pipe_fds[READ_END]);
		}
		close(requestNode->pipe_fds[READ_END]);

		if (requestNode->CGIBody.find("502") != std::string::npos)
			Logger::setErrorAndLog(&requestNode->CGIError, 502, "cgi-script: processing error inside cgi");
	}
	return (1);
}