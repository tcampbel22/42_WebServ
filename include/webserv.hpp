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
#include <poll.h> //poll
#include <sys/socket.h> //socket
#include <string>
#include <dirent.h>
#include "../src/Logger/Logger.hpp"

#define MAX_DIRECTORY_SIZE 500000000
#define MAX_FILE_SIZE 500000000
#define MAX_BODY_SIZE 9000000
// #define CGI_TIMEOUT 5000 //CGI timeout in milliseconds (1000 = 1sec)
#define MAX_HEADER_SIZE 8912
#define GET 1
#define POST 2
#define DELETE 3
#define RESET  "\033[0m"
#define RED    "\033[31m"
#define GREEN  "\033[32m" 
#define BLUE   "\033[34m" 
