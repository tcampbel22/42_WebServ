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

#include "HttpParser.hpp"

class chunkedBodyParser
{
private:
	static int hexToInt(std::string line);
	static std::string readChunk(std::istringstream& stream, int chunkSize);
	void parse(std::istringstream& stream, HttpRequest& request);
	void procesLeftOvers(std::istringstream& stream);
public:
	static void parseChunkedBody(std::istringstream& stream, HttpRequest& request);
};

