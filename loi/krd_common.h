/* krd_common.h
 * Copyright 2014 Miquel Pericas and Tokyo Institute of Technology
 * This program is licensed under the terms of the New BSD License.
 * See LICENSE for full details
 *
 * krd_common.h: access functionality to allocate data trace area
 */

#include "loi.h"

extern int krd_trace_magic;
extern struct krd_thread_trace *  threadtraces;

int krd_init();
int krd_init_block(int, long);
int krd_init_index(int, long);
int krd_read_coherent_trace(int);

