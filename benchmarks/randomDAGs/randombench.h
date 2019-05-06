#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

//matrix mult configs
#define ROW_SIZE 64
#define M_ASSEMBLY_WIDTH 1
//sort config
#define SORT_SIZE 32
#define S_ASSEMBLY_WIDTH 1
//heat configs
#define HEAT_RESOLUTION 512
#define H_ASSEMBLY_WIDTH 1
//DAG configs
#define R_SEED 123
#define MATRIX_COUNT 3000
#define SORT_COUNT 0
#define HEAT_COUNT 0
#define EDGE_RATE 5
#define LEVEL_WIDTH 2

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

