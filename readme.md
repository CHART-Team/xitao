# XiTAO RUNTIME #
XiTAO is an execution model and runtime for low-overhead and interference-free execution of mixed-mode DAGs on manycores. XiTAO generalizes the concept of task by providing it with (1) internal concurrency, (2) an embedded scheduler and (3) a resizeable resource container. Hence, XiTAO tasks, called TAOs: "Task Assembly Objects" become moldable entities. Via a elastic resource partitioning, XiTAO provides better mapping between tasks and hardware resources such as cores, caches or interconnect bandwidth. Therefore, XiTAO provides fast parallelism at low overhead, with constructive sharing and interference-avoidance. 

The following figure shows the architecture of the XiTAO runtime


![Image of the XiTAO Arch](xitao_arch.png)

## Building the XiTAO Library ##
```bash
make lib
```

## Running Dot Product Example ##
```bash
cd benchmarks/dotproduct
make dotprod
./dotprod 8192 2 16
```

## Explanation for the Dot Product Example ##
read documentation.pdf


## Running Synthetic DAGs Benchmark ##
```bash
cd benchmarks/syntheticDAGS
make synbench
./synbench <Block Side Length> <Resource Width> <TAO Mul Count> <TAO Copy Count> <TAO Stencil Count> <Degree of Parallelism>
```

## Turning on Debug Trace ##
```bash
cd ./include/
vim config.h
#define DEBUG
```

## Switching across Scheduling Strategies ##
```bash
vi makefile.sched
```

## Changing PTT Hardware Topology ##
Create a file with number of lines = number of threads + 1

-- first line contains affinity for each thread

-- the subsequent lines correspond to the threads listed in order, in which the thread is a leader. The line contains the allowed resource partition widths

-- set environment variable XITAO_LAYOUT_PATH to the absolute path of the layout file

Example: suppose you have 6 threads, and you would like to allow widths 1 and 2 where appropriate. A possible file (assuming contiguous core affinity ids)

0 1 2 3 4 5

1 2

1

1 2

1

1 2

1


## ACKNOWLEDGEMENT ##
This work has been supported by EU H2020 ICT project LEGaTO, contract #780681.

