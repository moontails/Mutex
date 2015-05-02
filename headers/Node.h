/*
* Node.h
*
*  Created on: April 4, 2015
*      Author: moontails
*/


#ifndef NODE_H_
#define NODE_H_

#include <iostream>
#include <string.h>
#include <array>
#include <vector>
#include <queue>
#include <map>
#include <mutex>
#include <thread>
#include <ctime>
#include <ratio>
#include <chrono>

enum State { INIT, REQUEST, HELD, RELEASE };
typedef std::pair <std::string, std::chrono::system_clock::time_point> MessageTimePair;

class Node
{

private:
  int threadId;
  std::vector<int> requestSet;
  std::string lastReply;
  bool voted;
  std::string requestSetString;
  MessageTimePair last_voted;

public:
  Node(const int);
  State myState;
  bool expired;
  bool logged;
  static int cs_int;
  static int next_req;
  static int total_exec_time;
  static int option;
  static std::vector<int> member_list;
  static std::vector<std::mutex> mtx_locks;
  static std::vector<std::deque<MessageTimePair>> messageQ;
  static std::vector<std::vector<std::string>> grantQ;

  void init();
  void check_messages();
  void check_grants();
  bool check_last_voted(const MessageTimePair, const MessageTimePair);
  void multicast(const State);
  void yield();
  void display();
  void logging();

  void pop_messageQ();
  void push_messageQ(const int, const MessageTimePair, const State);
  void clear_grantQ();
  void push_grantQ(const int, const std::string);
};

#endif  /* NODE_H_ */
