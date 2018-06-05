--------------
The following directory contains the synthetic benchmarks used for the hetergenous scheduling evaluations
in Agnes Rohlin & Henrik Fahlgrens thesis spring 2018.

The chains directory contains the single/parallel chains benchmarks.
The fork-join directory contains the fork-join patterened benchmark.
The random-dags directory contains the randomized dags benchmarks.
For all of the benchmarks the kernels are located in this directory(benchmarks): taocopy.h, taosort.h & taomatrix.h

To run the respective benchmarks cp the sample makefiles and sample configuration files and remake the runtime.Please make sure that the configuration parameters in config.h is also correct.The following CXXFLAGS are used to enable/disable the hetergenous scheduling extensions.

--------------

CXXFLAGS += -DTIME_TRACE  -- This flag enables the tracing of past executions, must be enabled for all scheduling extensions but CRIT_HETERO_SCHED.

##The task molding flags can be used together with any other flags##
CXXFLAGS += -DHISTORY_MOLD -- This flag enables the history based molding.
CXXFLAGS += -DLOAD_MOLD --This flag enables the load based molding.

#####Please note that only one of the following flags should be enabled at any one point#####
CXXFLAGS += -DCRIT_HETERO_SCHED -- This flag enables the heterogeneity-aware criticality scheduling extension. Requires defines for hetero environment in config.h

CXXFLAGS += -DCRIT_PERF_SCHED -- This flag enables the heterogeneity-UNaware criticality scheduling extension. Requires the TIME TRACE flag.

CXXFLAGS += -DWEIGHT_SCHED  -- This flag enables the weight-based scheduling extension. Requires the TIME TRACE flag. Requires defines for hetero environment in config.h