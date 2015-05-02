# Mutex
Implementation of mutual exclusion based on Maekawa’salgorithm.
---

Voting set:
The voting set for every thread is selected by laying the thread ids in a 3x3 grid as follows. Each node/thread’s voting set was selected using all the ids present in its row and column. For example , the voting set for thread with id = 1, consists of {1,2,3,4,7} as shown below.

| 1 | 2 | 3 |
| 4 | 5 | 6 |
| 7 | 8 | 9 |

---
Mechanism to avoid deadlocks:

Although Maekawa’s algorithm is designed to provide mutual exclusion, the order in which the requests are processed by the voting sets can sometimes results in a deadlock. Thus, to remove possible deadlocks in Maekawa algorithm, it is necessary to
break one of the four conditions that lead to a deadlock. I have implemented a priority queue based method to ensure there is no circular wait and thus avoid deadlocks. This imposes ordering on the resources, which in our case are the votes and thus ensures that there is no circular wait, thereby no deadlocks.
