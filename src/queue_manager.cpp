#include "queue_manager.h"

std::vector<PolyTask *> queue_manager::worker_ready_q[XITAO_MAXTHREADS];
LFQueue<PolyTask *> queue_manager::worker_assembly_q[XITAO_MAXTHREADS];  