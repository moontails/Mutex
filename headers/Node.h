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

enum State { INIT, REQUEST, HELD, RELEASE };

class Node
{

private:
  int threadId;
  std::vector<int> requestSet;
  std::string lastReply;
  bool voted;

public:
  Node(const int);
  State myState;
  static int cs_int;
  static int next_req;
  static int total_exec_time;

  static std::vector<int> member_list;
  static std::vector<std::deque<std::string>> messageQ;
  static std::vector<std::vector<std::string>> grantQ;

  void init();
  void check_messages();
  void check_grants();
  void multicast(const State);
  void yield();
  void display();
  void logging();
};

#endif  /* NODE_H_ */
