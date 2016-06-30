#!/bin/bash

# evaluation of i.3.1 UTS benchmark
# assembly size of 3, minimum computation granularity (~800ns)

#for cores in 3 6 12 18 24 30 36 42 48; do
#for cores in 24 30 36 42 48; do
#  for i in `seq 0 99`; do 
#    echo "TAO: $cores, $i"
#    TAO_NTHREADS=${cores} TAO_THREAD_BASE=0 ../uts11 -f uts.in | grep elapsed >> tao11.${cores}.out
#  done
#done

for cores in 48 42 36 30 24 18 12 6 2; do
  for i in `seq 0 32`; do 
    echo "TAO: $cores, $i"
    TAO_REPETITIONS=3 TAO_NTHREADS=${cores} TAO_THREAD_BASE=0 ../uts22 -f uts.in | grep elapsed | tail -1 >> tao22.${cores}.out
  done
done

#for cores in 12 18 24 30 36 42 48; do
#  for i in `seq 0 32`; do 
#    echo "TAO: $cores, $i"
#    TAO_REPETITIONS=3 TAO_NTHREADS=${cores} TAO_THREAD_BASE=0 ../uts11 -f uts.in | grep elapsed | tail -1 >> tao11.${cores}.out
#  done
#done
#
#for cores in  12 18 24 30 36 42 48; do
#  for i in `seq 0 32`; do 
#    echo "TAO: $cores, $i"
#    TAO_REPETITIONS=3 TAO_NTHREADS=${cores} TAO_THREAD_BASE=0 ../uts33 -f uts.in | grep elapsed | tail -1 >> tao33.${cores}.out
#  done
#done
#
#for cores in 3 6 ; do
#  for i in `seq 0 32`; do 
#    echo "TAO: $cores, $i"
#    TAO_REPETITIONS=3 TAO_NTHREADS=${cores} TAO_THREAD_BASE=0 ../uts11 -f uts.in | grep elapsed | tail -1 >> tao11.${cores}.out
#  done
#done
#
#for cores in 3 6 ; do
#  for i in `seq 0 32`; do 
#    echo "TAO: $cores, $i"
#    TAO_REPETITIONS=3 TAO_NTHREADS=${cores} TAO_THREAD_BASE=0 ../uts33 -f uts.in | grep elapsed | tail -1 >> tao33.${cores}.out
#  done
#done
#
#for cores in 30 36 42 48; do
#  for i in `seq 0 32`; do 
#    echo "MYTH: $cores, $i"
#    MYTH_WORKER_NUM=${cores} ~/parallel2/apps/bots/bots-1.1.2/bin/uts.parallel.generic-tasks_g_mth -f uts.in -r 3 | grep Time | tail -1 >> mth.${cores}.out
#  done
#done
#
#for cores in 30 36 42 48; do
#  for i in `seq 0 32`; do 
#    echo "TBB: $cores, $i"
#    TBB_NTHREADS=${cores} ~/parallel2/apps/bots/bots-1.1.2/bin/uts.parallel.generic-tasks_g_tbb -f uts.in -r 3 | grep Time | tail -1 >> tbb.${cores}.out
#  done
#done
#
#for cores in 12 18 24 30 36 42 48; do
#  for i in `seq 0 32`; do 
#    echo "QTH: $cores, $i"
#    QTHREAD_HWPAR=${cores} ~/parallel2/apps/bots/bots-1.1.2/bin/uts.parallel.generic-tasks_g_qth -f uts.in -r 3 | grep Time | tail -1 >> qth.${cores}.out
#  done
#done


#for i in `seq 0 4`; do 
#    echo "Serial: $cores, $i"
#    ~/parallel2/apps/bots/bots-1.1.2/bin/uts.parallel.generic-tasks_g_serial -f uts.in -r 1 | grep Time >> serial.out
#done
