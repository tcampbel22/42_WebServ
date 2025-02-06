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
#include <fstream>
#include <ctime>
#include <filesystem>

enum e_log
{
	ERROR,
	INFO
};

class Logger
{
private:
	static	std::ofstream 	log_file;
	static	int				count;
	static	std::string		last_line;
public:
	Logger();
	~Logger();
	static std::string 		getCurrentTime();
	static void				setErrorAndLog(int *error, int set, std::string msg);
	static void 			log(std::string msg, e_log log_code, bool print);
	static bool				checkFileSize();
	static void 			closeLogger();
	static void				checkLoop(std::string msg, std::string log_code);
};