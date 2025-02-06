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

HttpServer* HttpServer::_instance = nullptr;

HttpServer::HttpServer(std::vector<ServerSettings> vec)
{
	this->_instance = this;
	settings_vec = vec;
	fillHostPortPairs();
	_clientSocket = -1;
	startServer();
}

void HttpServer::startServer()
{
	for (u_long i = 0 ; i < _ip_port_list.size() ; i++)  //Iterate through host and port pairs (host is first and port is second)
	{
		auto it = _ip_port_list[i];
		int serverFd = socket(AF_INET, SOCK_STREAM, 0);
		if (serverFd == -1)
		{
			Logger::log("failed to create socket: " + it.first, ERROR, true);
			continue;
		}
		//store socket addr info
		int optionValue = 1;
		setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue));
		memset(&_socketInfo, 0, sizeof(_socketInfo));
		_socketInfo.sin_family = AF_INET; //macro for IPV4
		_socketInfo.sin_port = htons(it.second); //converts port number to network byte order
		_socketInfo.sin_addr.s_addr = inet_addr(it.first.c_str()); //converts ip address from string to uint

		//bind socket to port
		if (bind(serverFd, (sockaddr *)&_socketInfo, sizeof(_socketInfo)) < 0)
		{
			Logger::log("failed to bind socket: " + it.first, INFO, true);
			close(serverFd);
			continue;
		}
		if (listen(serverFd, SOMAXCONN) < 0)
		{
			Logger::log("failed to listen", INFO, true);
			close(serverFd);
			continue;
		}
		settings_vec[i]._fd = serverFd;
		_server_fds.push_back(serverFd); //add fd to vector to use later in listening function
		Logger::log("Listening on host: " + (std::string)it.first + " Port: " + std::to_string(it.second), INFO, true);
		_connections++;
	}
}

void HttpServer::startListening()
{
	std::signal(SIGINT, signalHandler);	
	std::signal(SIGPIPE, SIG_IGN);

	addServerToEpoll();
	while (true)
	{
		numEvents = epoll_wait(epollFd, _eventsArr, MAX_EVENTS, 0);
		if (numEvents < 0)
		{
			Logger::log("epoll_wait failed", ERROR, false);
			break ;
		}
		time_t current_time = std::time(nullptr);
		for (int i = 0; i < numEvents; i++)
		{
			fdNode *nodePtr = static_cast <fdNode*>(_eventsArr[i].data.ptr);  //retrieving current serversettings and client fd
			if (nodePtr->isDead)
				continue;
			_events.data.ptr = nodePtr; //store event data in a temp variable for use in epoll functions
			if (std::find(_server_fds.begin(), _server_fds.end(), nodePtr->fd) != _server_fds.end()) //eventFd is a server_socket meaning a new request is incoming
				acceptNewClient(nodePtr, nodePtr->fd, current_time);
			else if (!checkSystemMemory(nodePtr) && _eventsArr[i].events & EPOLLIN) //client socket has data to read from
			{
				handle_read(nodePtr);
            }
			else if (!checkSystemMemory(nodePtr) && _eventsArr[i].events & EPOLLOUT && nodePtr && nodePtr->_readyToSend)
			{
				handle_write(nodePtr);
			}
		}
		fdActivityLoop(current_time);
	}
	close(epollFd);
}

void	HttpServer::addServerToEpoll()
{
	epollFd = epoll_create1(0); //create epoll instance
	if (epollFd < 0)
	{
		Logger::log("epoll_create: create failed", ERROR, true);
		this->~HttpServer();
		return ;
	}
	for (u_long i = 0 ; i < settings_vec.size() ; i++)  //iterate through fd vector and add to epoll
	 {
		auto it = settings_vec[i];
		_events.events = EPOLLIN | EPOLLOUT;
		std::shared_ptr<fdNode> server_node = std::make_shared<fdNode>();
		server_node->fd = settings_vec[i]._fd;
		server_node->serverPtr = &settings_vec[i];
		_events.data.ptr = server_node.get();
		server_nodes.push_back(server_node);
		setNonBlocking(server_node->fd);
		if (epoll_ctl(epollFd, EPOLL_CTL_ADD, settings_vec[i]._fd, &_events) == -1)		
		{
			Logger::log("epoll_ctl: failed to add to epoll", ERROR, false);
			continue; 
		}
	 }
}

void	HttpServer::acceptNewClient(fdNode* nodePtr, int eventFd, time_t current_time)
{
	socklen_t _sockLen = sizeof(_socketInfo);
	memset(&_socketInfo, 0, sizeof(_socketInfo));

	if (_connections < MAX_CONNECTIONS)
	{
		_clientSocket = accept(eventFd, (sockaddr *)&_socketInfo, &_sockLen);
		if (_clientSocket < 0) 
			Logger::log("accept: accept failed", ERROR, false);
		else
		{
			Logger::log("New client connected: " + std::to_string(_clientSocket), INFO, false);
			setNonBlocking(_clientSocket);
			_events.events = EPOLLIN;
			createClientNode(nodePtr);
			_fd_activity_map[_clientSocket] = current_time;
		}
	}
	else
		Logger::log("max connections reached", ERROR, false);
}

std::string getBoundary(std::string requestString) {
	size_t pos = requestString.find("WebKitFormBoundary");
	std::string boundaryCode = requestString.substr(pos, requestString.find_first_of("\r\n", pos) - pos);
	boundaryCode.append("--");
	return boundaryCode;
}

HttpServer::~HttpServer()
{
	close(epollFd);
	_ip_port_list.clear();
	for (auto it = settings_vec.begin(); it != settings_vec.end(); it++)
		close(it->_fd);
	for (auto it : client_nodes)
	{
		if (it.second != nullptr)
			delete it.second;
		close(it.first);
	}
	for (auto it : server_nodes)
		close(it->fd);
	client_nodes.clear();
	settings_vec.clear();
	settings_vec.shrink_to_fit();
	_server_fds.clear();
}

