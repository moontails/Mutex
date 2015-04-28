/*
 * MessageHandler.h
 *
 *  Created on: Mar 8, 2015
 *      Author: moontails, emch2
 */

#ifndef MESSAGEHANDLER_H_
#define MESSAGEHANDLER_H_

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <chrono>

#define DELIM ':'

using namespace std;

class MessageHandler
{
public:
	static std::string serialize(std::string inputMessage);
	static std::vector<std::string> deserialize(std::string inputMessage);
	static std::string deserializeB(std::string inputMessage);
	static std::string serialize_vector(std::vector<int>);
	static std::vector<int> deserialize_vector(std::string);
};

#endif /* MESSAGEHANDLER_H_ */
