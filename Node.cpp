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
#include <cmath>
#include <typeinfo>

std::vector<std::thread> thread_pool(9);
std::vector<std::mutex> Node::mtx_locks(10);
std::map<int,std::vector<int>> votingSet;
std::chrono::system_clock::time_point beginning_of_time;

//defining the static variables
int Node::cs_int = 0;
int Node::next_req = 0;
int Node::total_exec_time = 0;
int Node::option = 0;
std::vector<std::deque<MessageTimePair>> Node::messageQ(10);
std::vector<std::vector<std::string>> Node::grantQ(10);

void node_runner(const int id);

//custom comparator to sort a vector of pairs based on the second element of the pair
// and break ties based on thread ids.
struct sort_pred
{
  bool operator()(const MessageTimePair &left, const MessageTimePair &right)
  {
    if(left.second == right.second)
    {
      std::vector<std::string> inputMessageVector1,inputMessageVector2;
      inputMessageVector1 = MessageHandler::deserialize(left.first);
      inputMessageVector2 = MessageHandler::deserialize(right.first);
      return std::stoi(inputMessageVector1[1]) < std::stoi(inputMessageVector2[1]);
    }
    else
      return left.second < right.second;
  }
};

// Function to pop safely from a node's message queue
void Node::pop_messageQ()
{
  Node::mtx_locks[this->threadId].lock();
  Node::messageQ[this->threadId].pop_front();
  Node::mtx_locks[this->threadId].unlock();
}

// Function to push safely to a node's message queue
void Node::push_messageQ(const int node_id, const MessageTimePair inputMessage, const State mode)
{
  Node::mtx_locks[node_id].lock();
  Node::messageQ[node_id].push_back(inputMessage);
  std::sort(Node::messageQ[node_id].begin(), Node::messageQ[node_id].end(), sort_pred());
  Node::mtx_locks[node_id].unlock();
}

// Function to pop safely from a node's message queue
void Node::clear_grantQ()
{
  Node::mtx_locks[this->threadId].lock();
  Node::grantQ[this->threadId].clear();
  Node::mtx_locks[this->threadId].unlock();
}

// Function to push safely to a node's message queue
void Node::push_grantQ(const int node_id, const std::string inputMessage)
{
  Node::mtx_locks[node_id].lock();
  Node::grantQ[node_id].push_back(inputMessage);
  Node::mtx_locks[node_id].unlock();
}

// Node constructor
Node::Node(const int id)
{
  ostringstream ss;

  this->threadId = id;
  this->myState = INIT;
  this->requestSet = votingSet[id];
  this->voted = false;
  this->expired = false;
  this->logged = false;
  for (int i=0; i<this->requestSet.size();i++)
  {
    ss << this->requestSet[i] << " ";
  }
  this->requestSetString = ss.str();
}

//Function to display the voting set of each node
void displaySet(const std::vector<int>& v)
{
  std::cout << "Membership List: ";
  for (int i=0; i<v.size();i++)
  {
    cout << v[i] << " ";
  }
  std::cout << std::endl;
}

// Function to display a message when threads are initialized
void Node::display()
{
  std::cout << "Hi I am node: " << this->threadId << " with quorum: " << this->requestSetString << std::endl;
}

//Function to send a mutlicast message to all nodes in the voting set
void Node::multicast(const State msgType)
{
  std::string message;
  ostringstream ss;
  MessageTimePair temp;
  std::time_t logTime;
  switch(msgType)
  {
    case REQUEST:
      logTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      //std::cout << "Sending Request from " << this->threadId << " at " << logTime << std::endl;
      ss << "request " << this->threadId;

      temp.first = MessageHandler::serialize(ss.str());
      temp.second = std::chrono::system_clock::now();

      for(int i = 0; i < this->requestSet.size(); i++)
      {
        if(Node::option == 1)
        {
          logTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
          std::cout << "Time: " << ctime(&logTime) << ", ThreadId: " << this->requestSet[i] << ", From: " << this->threadId << ", MessageType: " << temp.first << std::endl;
        }
        this->push_messageQ(this->requestSet[i], temp, msgType);
      }
      break;

    case RELEASE:
      ss << "release " << this->threadId;

      temp.first = MessageHandler::serialize(ss.str());
      temp.second = beginning_of_time;

      for(int i = 0; i < this->requestSet.size(); i++)
      {
        if(Node::option == 1)
        {
          logTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
          std::cout << "Time: " << ctime(&logTime) << ", ThreadId: " << this->requestSet[i] << ", From: " << this->threadId << ", MessageType: " << temp.first << std::endl;
        }
        this->push_messageQ(this->requestSet[i], temp, msgType);
      }
      break;
  }
}

//Function to check last voted
bool Node::check_last_voted(const MessageTimePair left, const MessageTimePair right)
{
  if(left.second == right.second)
  {
    std::vector<std::string> inputMessageVector1,inputMessageVector2;
    inputMessageVector1 = MessageHandler::deserialize(left.first);
    inputMessageVector2 = MessageHandler::deserialize(right.first);
    return std::stoi(inputMessageVector1[1]) < std::stoi(inputMessageVector2[1]);
  }
  else
    return left.second < right.second;
}

//Function to check a nodes message queue and take the necessary actions
void Node::check_messages()
{
  std::string inputMessage, outputMessage;
  std::vector<std::string> inputMessageVector;
  MessageTimePair temp;
  //ostringstream ss;
  std::time_t logTime;

  if(!Node::messageQ[this->threadId].empty())
  {
    temp = Node::messageQ[this->threadId].front();
    inputMessageVector = MessageHandler::deserialize(temp.first);
    //ss << "Granted";
    if(inputMessageVector[0] == "request" && this->voted == false)
    {
      if(this->voted == false)
      {
        //std::cout << "Sending Grant to " << inputMessageVector[1] << std::endl;
        this->voted = true;
        if(Node::option == 1)
        {
          logTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
          std::cout << "Time: " << ctime(&logTime) << ", ThreadId: " << inputMessageVector[1] << ", From: " << this->threadId << ", MessageType: grant" <<  std::endl;
        }
        this->push_grantQ(std::stoi(inputMessageVector[1]), "grant");
        this->pop_messageQ();
        this->last_voted = temp;
      }
      else
      {
        bool flag = this->check_last_voted(this->last_voted, temp);

      }
    }
    else if(inputMessageVector[0] == "release")
    {
      this->pop_messageQ();
      this->voted = false;
    }
  }
}

//Function to check for grants
void Node::check_grants()
{
  if(Node::grantQ[this->threadId].size() == 5)
  {
    this->myState = HELD;
    this->logged = true;
  }
}

//Function to log a message to stdout before enter the critical section
void Node::logging()
{
  ostringstream ss;
  std::time_t logTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  std::cout << "Time: " << ctime(&logTime) << ", ThreadId: " << this->threadId << ", Node-List: " << this->requestSetString << std::endl;
}

//Function to simulate each node/thread
void node_runner(const int i)
{
  std::chrono::system_clock::time_point start, cs_start, release_start;
  std::chrono::seconds timer;
  std::chrono::milliseconds cs_timer, release_timer;
  ostringstream ss;

  start = std::chrono::system_clock::now();

  //std::cout << "Hi, I am node: " << node_id << std::endl;

  //NODE INITIALIZATION
  Node newnode = Node(i+1);
  newnode.display();
  while( !newnode.expired )
  {
    //check for program time elapse
    timer = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start);

    if(static_cast<int>(timer.count()) >= Node::total_exec_time )
      newnode.expired = true;

    // cs_start = std::chrono::system_clock::now();
    // srand(time(0));
    // int breaker = std::rand() % 1000*i;
    // while(1)
    // {
    //   cs_timer = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - cs_start);
    //
    //   if(static_cast<int>(cs_timer.count()) >= breaker )
    //     break;
    //   newnode.check_messages();
    // }

    //NODE REQUEST STAGE
    newnode.myState = REQUEST;
    newnode.multicast(newnode.myState);

    while(newnode.myState == REQUEST && !newnode.expired)
    {
      newnode.check_messages();
      newnode.check_grants();

      timer = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start);

      if(static_cast<int>(timer.count()) >= Node::total_exec_time )
        newnode.expired = true;
    }

    //NODE CRITICAL SECTION STAGE

    // Each thread printing a log to the screen before it enters its crtical section
    if(newnode.logged)
    {
      newnode.logging();
    }

    newnode.clear_grantQ();
    cs_start = std::chrono::system_clock::now();

    while(newnode.myState == HELD && !newnode.expired)
    {
      // wait for cs_int seconds
      cs_timer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - cs_start);

      if(static_cast<int>(cs_timer.count()) >= Node::cs_int )
        break;

      timer = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start);

      if(static_cast<int>(timer.count()) >= Node::total_exec_time )
        newnode.expired = true;
    }

    //NODE RELEASE STAGE
    newnode.logged = false;
    newnode.myState = RELEASE;
    newnode.multicast(newnode.myState);
    release_start = std::chrono::system_clock::now();

    while(newnode.myState == RELEASE && !newnode.expired)
    {
      //wait for next_req seconds
      release_timer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - release_start);

      if(static_cast<int>(release_timer.count()) >= Node::next_req )
        break;

      timer = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start);

      if(static_cast<int>(timer.count()) >= Node::total_exec_time )
        newnode.expired = true;

      newnode.check_messages();


    }
  }
}

//voting set map for each node based on a 3x3 grid
void init_voting_set()
{
  votingSet[1] = {1,2,3,4,7};
  votingSet[2] = {1,2,3,5,8};
  votingSet[3] = {1,2,3,6,9};
  votingSet[4] = {1,4,5,6,7};
  votingSet[5] = {2,4,5,6,8};
  votingSet[6] = {3,4,5,6,9};
  votingSet[7] = {1,4,7,8,9};
  votingSet[8] = {2,5,7,8,9};
  votingSet[9] = {3,6,7,8,9};
}

/*
 * Function - Main function to map commands, spawn node 0 and spawn listener thread.
 */
int main(int argc, char *argv[])
{
  beginning_of_time =  std::chrono::system_clock::now();
  init_voting_set();
  // <cs_int>	 <next_req>	  <tot_exec_time>	  <option>
  int N = 9;
  if(argc > 5 || argc < 4 )
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

    if(argc == 5)
    {
      istringstream ss4(argv[4]);
      ss4 >> Node::option;
      std::cout << "option is set: " << typeid(Node::option).name() << std::endl;
    }

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
  //std::cout << "Closing all nodes as total execution time has elapsed" << std::endl;
  for(int i=0; i<9; i++)
  {
    thread_pool[i].join();
  }

  return 0;
}
