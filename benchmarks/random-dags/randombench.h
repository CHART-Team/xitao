


//extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>


enum Taotype { heat, sort_, matrix };


//node struct to keep infromation about each node
typedef struct
{
  Taotype ttype; // TAO class of the node
  int nodenr; //unique number of the node
  int taonr; //TAO number of the node, unique within nodes of the same class (example 14:th sort TAO)
  std::vector<int> edges; //vector with nodenr of input edges
  int mem_space; //allocated memory slot of the nodes input and output data
  int criticality; //criticality used to calculate parallelism of the generated DAG
}node;


node new_node(Taotype type, int nodenr, int taonr);
void add_edge(node &n, node const &e);
void add_space(node &n, std::vector<int> &v, int a);
int find_mem(Taotype type, int nodenr, node const &n, std::vector<int> &v);
bool edge_check(int edge, node const &n);
int find_criticality(node &n);


//}
