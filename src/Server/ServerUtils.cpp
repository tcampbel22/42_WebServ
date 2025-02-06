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
#include <sys/wait.h>

void HttpServer::signalHandler(int signal)
{
	(void)signal;
	Logger::log("\nExit signal received, server shutting down.. ", INFO, true);
	_instance->~HttpServer();
	Logger::closeLogger();
	exit(0);
}

void	HttpServer::fillHostPortPairs()
{
	for (auto& it : settings_vec)
	{
		_ip_port_list.push_back( {it.getHost(), it.getPort()} );
	}
}

void	HttpServer::setNonBlocking(int socket)
{
	int	flag = fcntl(socket, F_GETFL, 0); //retrieves flags/settings from socket
	if (flag < 0)
		Logger::log("fcntl: GETFL failed", ERROR, true);
	if (fcntl(socket, F_SETFL, flag | O_NONBLOCK) < 0) //Sets socket to be nonblocking
		Logger::log("fcntl: SETFL failed", ERROR, true); 
}

bool HttpServer::isNonBlockingSocket(int fd) 
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) 
	{
		Logger::log("fcntl: failed to retrieve flags", ERROR, false);
		return false;
    }
    return (flags & O_NONBLOCK) != 0;
}

int HttpServer::getContentLength(const std::string& requestStr)
{
    size_t pos = requestStr.find("Content-Length: ");
    if (pos != std::string::npos)
    {
        size_t start = pos + 15;
        size_t end = requestStr.find("\r\n", start);
		if (end != std::string::npos) {
        	std::string lengthStr = requestStr.substr(start, end - start);
        	try {
				return std::stoi(lengthStr);
			} catch(std::exception& e) 
			{
				Logger::log("stoi: " + (std::string)e.what(), ERROR, false);
				return -1;
			}
		}
    }
    return 0;
}
bool HttpServer::isMultiPart(std::string requestStr) { return requestStr.find("multipart/form-data") != std::string::npos; }

bool HttpServer::isRequestWithBody(std::string requestStr) { return requestStr.find("Content-Length:") != std::string::npos; }

bool HttpServer::isChunkedTransferEncoding(const std::string& requestStr) { return requestStr.find("Transfer-Encoding: chunked") != std::string::npos; }

// If the client has been inactive for too long, close the socket
void HttpServer::fdActivityLoop(const time_t current_time)
{
	if (_connections > 100)
		_timeoutScale = 1.0 - ((float)_connections * TIME_OUT_MOD);
	else
		_timeoutScale = 1.0;
	_timeoutScale *= TIME_OUT_PERIOD;
	for (auto it = _fd_activity_map.begin(); it != _fd_activity_map.end();) 
	{
		if (current_time - it->second > _timeoutScale) 
		{
			Logger::log("timeout: closing client socket " + std::to_string(it->first), INFO, true);
			auto node = client_nodes.find(it->first);
			it = _fd_activity_map.erase(it);
			if (node->second->cgiStarted == true)
			{
				kill(node->second->pid, SIGKILL);
				close(node->second->pipe_fds[READ_END]);
				close(node->second->pipe_fds[WRITE_END]);
				HttpRequest request(node->second->serverPtr, epollFd, _events);
				Logger::setErrorAndLog(&request.errorFlag, 504, "Child is deeeeeeead!");
				ServerHandler response(node->second->fd, request);
			}
			cleanUpFds(node->second);
        } 
		else 
			++it;
    }
}

void	HttpServer::cleanUpFds(fdNode *nodePtr)
{
	if (nodePtr == nullptr || nodePtr->isDead)
		return;
	_connections--;
	if (_connections < 4)
		_connections = 4;
	if (!nodePtr->_clientDataBuffer.empty())
		nodePtr->_clientDataBuffer.clear(); //empty data buffer read from client
	if (!nodePtr->CGIBody.empty())
		nodePtr->CGIBody.clear();	
	int temp = nodePtr->fd;
	if (epoll_ctl(epollFd, EPOLL_CTL_DEL, temp, nullptr))
	{
		Logger::log("clean-up-fds: fd cannot be deleted", ERROR, false);
		return ;
	}
	Logger::log("fd [" + std::to_string(nodePtr->fd) + "] deleted from epoll event list", INFO, false);
	_fd_activity_map.erase(temp);
	client_nodes.erase(temp);
	_clientClosedConn = false;
	close(temp);
	nodePtr->isDead = true;
	delete nodePtr;
}

void	HttpServer::createClientNode(fdNode* nodePtr)
{
	_connections++;
	if (_connections > MAX_CONNECTIONS)
		_connections = MAX_CONNECTIONS;
	fdNode* client_node = new fdNode(*nodePtr);
	client_node->fd = _clientSocket;
	client_nodes[_clientSocket] = client_node;
	client_node->serverPtr = nodePtr->serverPtr;
	_events.data.ptr = client_node;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, client_node->fd, &_events) == -1)
	{
		Logger::log("Failed to add to epoll", ERROR, false);
		client_nodes.erase(_clientSocket);
		close(_clientSocket);
	}
}

bool	HttpServer::checkSystemMemory(fdNode* node)
{
	struct sysinfo sys_data;
	if (sysinfo(&sys_data) != 0) {
        Logger::log("error: failed to get system memory info", ERROR, false);
        return false;
    }
	uint total_mem = (sys_data.freeram + sys_data.bufferram) * sys_data.mem_unit / (1024 * 1024);
	if (total_mem < 100 && !node->_error) 
	{
		Logger::log("Total memory available: " + std::to_string(total_mem) + "MB", INFO, false);
		Logger::setErrorAndLog(&node->_error, 507, "error: system memory critically low, retry later");
		node->_readyToSend = true;
		_events.events = EPOLLOUT;
		if (epoll_ctl(epollFd, EPOLL_CTL_MOD, node->fd, &_events) == -1)		
		{
			Logger::log("Failed to mod epoll", ERROR, false);
			cleanUpFds(node);
		}
		return true;
	}
	return false;
}

bool	HttpServer::resetCGI(fdNode* nodePtr)
{
	nodePtr->cgiStarted = false;
	nodePtr->CGIReady = false;
	nodePtr->pid = 0;
	nodePtr->CGIBody.erase();
	_events.events = EPOLLIN;
	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, nodePtr->fd, &_events) == -1)		
	{
		Logger::log("Failed to mod epoll", ERROR, false);
		cleanUpFds(nodePtr);
		return false; 
	}
	resetNode(nodePtr);
	return true;
}

void	HttpServer::resetNode(fdNode* nodePtr)
{
	nodePtr->headerCorrect = false;
	nodePtr->_error = 0;
	nodePtr->_readyToSend = false;
	nodePtr->_clientDataBuffer.clear();
	_fd_activity_map[nodePtr->fd] = std::time(nullptr);
}

void	HttpServer::validateHeaders(const std::vector<char>& data, int *errorFlag) 
{
	std::string requestStr(data.begin(), data.end());

	if (requestStr.find("\r\n\r\n") != std::string::npos)
		*errorFlag = 0;
	else
		*errorFlag = 431;
}

bool	HttpServer::validateContentLength(fdNode* node, int length)
{
	if (node->serverPtr->getMaxClientBody() < length)
	{
		Logger::setErrorAndLog(&node->_error, 413, "Content length exceeds max body limit");
		return false;
	}
	return true;
}

void	HttpServer::cleanUpChild(fdNode *nodePtr)
{
	if (nodePtr == nullptr)
		return ;
	if (!nodePtr->_clientDataBuffer.empty())
		nodePtr->_clientDataBuffer.clear(); //empty data buffer read from client
	for (auto it = settings_vec.begin(); it != settings_vec.end(); it++)
		close(it->_fd);
	for (auto it : client_nodes)
		close(it.second->fd);
	for (auto it : server_nodes)
		close(it->fd);
	close(nodePtr->pipe_fds[WRITE_END]);
	close(nodePtr->pipe_fds[READ_END]);
	close(epollFd);
	_ip_port_list.clear();
	settings_vec.clear();
	settings_vec.shrink_to_fit();
	close(nodePtr->fd);
	delete nodePtr;
	nodePtr = nullptr;
	_instance->~HttpServer();
}