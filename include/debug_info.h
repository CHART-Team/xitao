#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H
#include "config.h"
#include "poly_task.h"
#include <string>
#include <iostream>

#ifdef DEBUG
extern GENERIC_LOCK(output_lck);
#define DEBUG_MSG(x) do {  \
					LOCK_ACQUIRE(output_lck);\
					std::cerr << x << std::endl; \
					LOCK_RELEASE(output_lck);\
					} while (0)
#else
#define DEBUG_MSG(x)
#endif
#endif
