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

#include "chunkedBodyParser.hpp"

void chunkedBodyParser::parseChunkedBody(std::istringstream& stream, HttpRequest& request) {
	chunkedBodyParser parser;
	parser.parse(stream, request);
}

int chunkedBodyParser::hexToInt(std::string line) {
	int val;
	try
	{
		val = std::stoi(line, nullptr, 16);
	}
	catch(const std::exception& e)
	{
		Logger::log(e.what(), ERROR, false);
	}
	return val;
}

void chunkedBodyParser::parse(std::istringstream& stream, HttpRequest& request)  {
	try {
		std::string line;
		while (getline(stream, line))
		{
			if (!line.empty() && line.back() == '\r')
           	 line.pop_back();
			int chunkSize = hexToInt(line);
			if (chunkSize == 0) {
				procesLeftOvers(stream); //handles the rest of the characters we have read with getline
				break;
			}
			request.body += readChunk(stream, chunkSize);
		}
	}
	catch(const std::exception& e)
	{
		Logger::setErrorAndLog(&request.errorFlag, 400, e.what());
	}
}

std::string chunkedBodyParser::readChunk(std::istringstream& stream, int size)
{
	std::string chunk;
	chunk.reserve(size);
	char c;
		for (int i = 0; i < size && stream.get(c); i++)
			chunk += c;

		std::string line;
		getline(stream, line);
	return chunk;
}
void chunkedBodyParser::procesLeftOvers(std::istringstream& stream) {
	std::string line;
	while (getline(stream, line))
	{
		if (line.empty() || line == "r")
			break;
	}
	//might have to add some trailer field parsing?? not sure if we need to handle it, since they are uncommonly used
}