/*
* Node.cpp
*
*  Created on: Mar 8, 2015
*      Author: moontails, emch2
*/

#include "headers/Node.h"
#include "headers/MessageHandler.h"
#include <algorithm>
#include <sstream>
#include <thread>
#include <ctime>
#include <chrono>
#include <mutex>
#include <map>
#include <cmath>

std::vector<std::thread> thread_pool(9);
std::vector<std::mutex> mtx_locks(9);
std::map<int,std::vector<int>> votingSet;
//defining the static variables
int Node::cs_int = 0;
int Node::next_req = 0;
int Node::total_exec_time = 0;
std::vector<int> Node::member_list;
std::vector<std::deque<std::string>> Node::messageQ(10);
std::vector<std::vector<std::string>> Node::grantQ(10);

void node_runner(const int id);

// Function to pop safely from a node's message queue
void pop_messageQ(const int node_id)
{
  mtx_locks[node_id].lock();
  Node::messageQ[node_id].pop_front();
  mtx_locks[node_id].unlock();
}

// Function to push safely to a node's message queue
void push_messageQ(const int node_id, const std::string inputMessage, const State mode)
{
  mtx_locks[node_id].lock();

  switch(mode)
  {
    case REQUEST:
      Node::messageQ[node_id].push_back(inputMessage);
      break;

    case RELEASE:
      Node::messageQ[node_id].push_front(inputMessage);
      break;
  }

  mtx_locks[node_id].unlock();
}

// Function to pop safely from a node's message queue
void clear_grantQ(const int node_id)
{
  mtx_locks[node_id].lock();
  Node::grantQ[node_id].clear();
  mtx_locks[node_id].unlock();
}

// Function to push safely to a node's message queue
void push_grantQ(const int node_id, const std::string inputMessage)
{
  mtx_locks[node_id].lock();
  Node::grantQ[node_id].push_back(inputMessage);
  mtx_locks[node_id].unlock();
}

// Function to display a vector
void display(const std::vector<int>& v)
{
  std::cout << "Membership List: ";
  for (int i=0; i<v.size();i++)
  {
    cout << v[i] << " ";
  }
  std::cout << std::endl;
}

Node::Node(const int id)
{
  this->threadId = id;
  this->myState = INIT;
  this->requestSet = votingSet[id+1];
  this->voted = false;
}

void displaySet(const std::vector<int>& v)
{
  std::cout << "Membership List: ";
  for (int i=0; i<v.size();i++)
  {
    cout << v[i] << " ";
  }
  std::cout << std::endl;
}

void Node::display()
{
  std::cout << "Hi I am node: " << this->threadId << " with quorum as " << std::endl;
  displaySet(this->requestSet);
}

void Node::multicast(const State msgType)
{
  std::string message;
  ostringstream ss;

  switch(msgType)
  {
    case REQUEST:
      ss << "request " << this->threadId;

      for(int i = 0; i < this->requestSet.size(); i++)
      {
        push_messageQ(this->requestSet[i], MessageHandler::serialize(ss.str()), msgType);
      }
      break;

    case RELEASE:
      ss << "release " << this->threadId;

      for(int i = 0; i < this->requestSet.size(); i++)
      {
        push_messageQ(this->requestSet[i], MessageHandler::serialize(ss.str()), msgType);
      }
      break;
  }
}

void Node::check_messages()
{
  std::string inputMessage, outputMessage;
  std::vector<std::string> inputMessageVector;
  //ostringstream ss;

  if(!Node::messageQ[this->threadId].empty())
  {
    inputMessageVector = MessageHandler::deserialize(Node::messageQ[this->threadId].front());
    //ss << "Granted";
    if(inputMessageVector[0] == "request" && this->voted == false)
    {
      this->voted = true;
      push_grantQ(std::stoi(inputMessageVector[1]), "grant");
      pop_messageQ(this->threadId);
    }
    else if(inputMessageVector[0] == "release")
    {
      pop_messageQ(this->threadId);
      this->voted = false;
    }
  }
}

void Node::check_grants()
{
  if(Node::grantQ[this->threadId].size() == 4)
    this->myState == HELD;
}

void Node::logging()
{
  ostringstream ss;
  time_t logTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  ss << "Time: " << ctime(&logTime) << ", ThreadId: " << this->threadId << ", Node-List: " ;

  for (int i=0; i<this->requestSet.size(); i++)
  {
    ss << this->requestSet[i] << " ";
  }

  std::cout << ss.str() << std::endl;
}

void node_runner(const int i)
{
  std::chrono::system_clock::time_point start, cs_start, release_start;
  std::chrono::seconds timer;
  std::chrono::milliseconds cs_timer, release_timer;
  ostringstream ss;

  start = std::chrono::system_clock::now();

  //std::cout << "Hi, I am node: " << node_id << std::endl;
  Node newnode = Node(i+1);
  newnode.display();
  while( 1 )
  {
    timer = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start);

    if(static_cast<int>(timer.count()) >= Node::total_exec_time )
      break;

    newnode.myState = REQUEST;
    newnode.multicast(newnode.myState);

    while(newnode.myState == REQUEST)
    {
      newnode.check_messages();
      newnode.check_grants();
    }

    // Each thread printing a log to the screen before it enters its crtical section
    newnode.logging();

    cs_start = std::chrono::system_clock::now();

    while(newnode.myState == HELD)
    {
      // wait for cs_int seconds
      cs_timer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - cs_start);

      if(static_cast<int>(cs_timer.count()) >= Node::cs_int )
        break;
    }

    newnode.myState = RELEASE;
    newnode.multicast(newnode.myState);
    release_start = std::chrono::system_clock::now();

    while(newnode.myState == RELEASE)
    {
      //wait for next_req seconds
      release_timer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - release_start);

      if(static_cast<int>(release_timer.count()) >= Node::next_req )
        break;

      newnode.check_messages();
    }
  }
}

void init_voting_set()
{
  votingSet[1] = {2,3,4,7};
  votingSet[2] = {1,3,5,8};
  votingSet[3] = {1,2,6,9};
  votingSet[4] = {1,5,6,7};
  votingSet[5] = {2,4,6,8};
  votingSet[6] = {3,4,5,9};
  votingSet[7] = {1,4,8,9};
  votingSet[8] = {2,5,7,9};
  votingSet[9] = {3,6,7,8};

}
/*
 * Function - Main function to map commands, spawn node 0 and spawn listener thread.
 */
int main(int argc, char *argv[])
{
  init_voting_set();
  // <cs_int>	 <next_req>	  <tot_exec_time>	  <option>
  int N = 9;
  if(argc >=5 )
  {
    std::cout << "\nPlease enter the right number of arguements\n" << std::endl;
    std::cout << "\n./mutex	<cs_int> <next_req> <tot_exec_time> <option> " << std::endl;
    exit(0);
  }
  else
  {
    istringstream ss1(argv[1]);
    istringstream ss2(argv[2]);
    istringstream ss3(argv[3]);

    ss1 >> Node::cs_int;
    ss2 >> Node::next_req;
    ss3 >> Node::total_exec_time;

  }
  // display startup message
  std::cout << "\n=======================" << std::endl;
  std::cout << "\n Intiliazing the system" << std::endl;
  std::cout << "\n=======================\n" << std::endl;

  // spawn the N threads
  for(int i=0; i<9; i++)
  {
    thread_pool[i] = std::thread(node_runner, i);
  }
  // wait for all the node threads to complete their execution
  //std::cout << "Joining" << std::endl;
  for(int i=0; i<9; i++)
  {
    thread_pool[i].join();
  }

  return 0;
}
