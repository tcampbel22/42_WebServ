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

#include "Logger.hpp"
std::ofstream	Logger::log_file;
int				Logger::count = 1;
std::string		Logger::last_line = "";

bool	Logger::checkFileSize()
{
	log_file.seekp(0, std::ios::end);
	int bytes = log_file.tellp();
	if (bytes > 10000000)
		return true;
	return false;
}

Logger::Logger()
{
	std::string path = "log";
	std::filesystem::create_directories(path);

	try {
		log_file.open(path + "/log.log", std::ios::trunc);
		if (!log_file)
			throw std::runtime_error("log file failed to open");
		log_file << "[INFO]  " << getCurrentTime() << ": WEBSERV STARTED\n";
		if (checkFileSize())
			throw std::runtime_error("Log file limit exceeded");
	}
	catch (std::exception& e)
	{
		log_file.close();
		std::cerr << e.what() << std::endl;
	} 

}

std::string Logger::getCurrentTime() 
{
	std::time_t now = std::time(nullptr);
	char buf[100];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
	return std::string(buf);
}

void	Logger::setErrorAndLog(int *error, int set, std::string msg)
{
	*error = set;
	log("[" + std::to_string(set) + "] " + msg, INFO, false);
}

void	Logger::checkLoop(std::string msg, std::string log_code)
{
	if (!last_line.empty() && !msg.compare(last_line))
		count++;
	else
	{
		if (count > 1)
			log_file << "[INFO] " << getCurrentTime() << ": " << last_line << 
			": Loop ran for " + std::to_string(count) + " iterations" << std::endl;
		log_file << "[" + log_code + "] " << getCurrentTime() << ": " << msg << std::endl;
		count = 1;
		last_line = msg;
	}
}

void	Logger::log(std::string msg, e_log log_code, bool print)
{
	if (log_file.is_open())
	{
		if (log_code == ERROR)
		{
			if (print)
				std::cerr << "[ERROR]: webserv: " << msg << std::endl;
			checkLoop(msg, "ERROR");
			if (checkFileSize())
			{
				std::cerr << "Log file limit exceeded" << std::endl; 
				log_file.close();
			}
		}
		if (log_code == INFO)
		{
			if (print)
				std::cerr << msg << std::endl;
			checkLoop(msg, "INFO");
			if (checkFileSize())
			{
				std::cerr << "Log file limit exceeded" <<  std::endl; 
				log_file.close();
			}
		}
	}
}

void Logger::closeLogger() 
{
    if (log_file.is_open())
	{
		log_file << "[INFO]  " << getCurrentTime() << ": WEBSERV FINISHED\n";
		log_file.close();
	}
}

Logger::~Logger() {}