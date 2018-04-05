


//extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>


enum Taotype { heat, sort_, matrix };

typedef struct
{
  Taotype ttype;
  int nodenr;
  int taonr;
  std::vector<int> edges;
  int mem_space;
}node;


node new_node(Taotype type, int nodenr, int taonr);

void add_edge(node &n, node const &e);
void add_space(node &n, std::vector<int> &v, int a);
int find_mem(Taotype type, int nodenr, node const &n, std::vector<int> &v);
bool edge_check(int edge, node const &n);



//}
