/*
* Storage.h
*
*  Created on: Mar 8, 2015
*      Author: moontails, emch2
*/


#include "headers/MessageHandler.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <queue>
#include <sstream>
#include <chrono>

using namespace std;
using std::chrono::system_clock;

/*
 * Function to serialize the input message into a string for transport across the communication channel
 */
std::string MessageHandler::serialize(std::string inputMessage)
{
	std::string message = inputMessage;

	std::replace(message.begin(), message.end(), ' ', DELIM);

	return message;
}

/*
 * Function to deserialize upon receiving a message from the communication channel
 */
std::vector<std::string> MessageHandler::deserialize(std::string inputMessage)
{
	std::string token;
	std::vector<std::string> command;

	size_t pos = 0;
	while((pos = inputMessage.find(DELIM)) != std::string::npos){
		token = inputMessage.substr(0,pos);
		command.push_back(token);
		inputMessage.erase(0,pos+1);
	}

	command.push_back(inputMessage);
	return command;
}

/*
 * Function to serialize a map into a string for transport across the communication channel
 */
std::string MessageHandler::serialize_vector(std::vector<int> temp)
{

	std::ostringstream oss;
	for(std::vector<int>::const_iterator i = temp.begin(); i != temp.end(); ++i)
	{
		oss << *i << DELIM;

	}

	return oss.str();
}

/*
 * Function to deserialize a string received from the communication channel into a map
 */
std::vector<int> MessageHandler::deserialize_vector(std::string inputMessage)
{
	int token;
	std::vector<int> command;

	size_t pos = 0;

	while((pos = inputMessage.find(DELIM)) != std::string::npos){
		token = std::stoi(inputMessage.substr(0,pos));
		//std::cout << token <<std::endl;
		command.push_back(token);
		inputMessage.erase(0,pos+1);
	}

	return command;
}
