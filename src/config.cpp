#include "config.h"
#include "perf_model.h"
#include <thread>
#include <sstream>

using namespace xitao;
using namespace std;



bool perf_model::minimize_parallel_cost = true;
int  perf_model::refresh_frequency      = 10;
int  perf_model::old_tick_weight     = 4;
bool perf_model::mold = true;
bool config::verbose  = true;
bool config::delete_executed_taos  = false;
int  config::thread_base = GOTAO_THREAD_BASE;
int  config::affinity = GOTAO_NO_AFFINITY;
int  config::steal_attempts = STEAL_ATTEMPTS;
int  config::task_pool = TASK_POOL;
int  config::sta = TAO_STA;
int  config::hw_contexts = GOTAO_HW_CONTEXTS;
int  config::nthreads = std::thread::hardware_concurrency();
bool config::enable_workstealing = true; 
bool config::enable_local_workstealing = false; 
bool config::use_performance_modeling = true;
const string config::xitao_args_prefix = "--xitao_args=";

static struct option long_options[] = {
  {"wstealing",    required_argument, 0, 'w'},
  {"lwstealing",   required_argument, 0, 'l'},
  {"perfmodel",    required_argument, 0, 'p'},
  {"nthreads",     required_argument, 0, 't'},
  {"idletries",    required_argument, 0, 'i'},
  {"minparcost",   required_argument, 0, 'c'},
  {"sta",          required_argument, 0, 's'},
  {"oldtickweight",    required_argument, 0, 'o'},
  {"refreshtablefreq",    required_argument, 0, 'f'},
  {"dealloctaos",    required_argument, 0, 'd'},
  {"mold",    required_argument, 0, 'm'},
  {"help",         no_argument,       0, 'h'},
  {0, 0, 0, 0}
};

void config::init_config(int argc, char** argv, bool read_all_args) { 
  // extract args after xitao_args_prefix (e.g. --xitao_args=) if required
  std::vector<char *> args;
  if(!read_all_args) {
    args.push_back((char*)xitao_args_prefix.c_str());
    string xitao_cmd_line_args;
    for(int i = 1; i < argc; ++i) {
      std::string xitao_args(argv[i]);
      std::string prefix(xitao_args_prefix);
      if (!xitao_args.compare(0, prefix.size(), prefix)) {
        xitao_cmd_line_args = xitao_args.substr(prefix.size());
        break;
      }
    }
    // extract xitao command line args
    std::istringstream iss(xitao_cmd_line_args);
    std::string token;
    while(iss >> token) {
      char *arg = new char[token.size() + 1];
      copy(token.begin(), token.end(), arg);
      arg[token.size()] = '\0';
      args.push_back(arg); 
    }
    argv = &args[0]; 
    argc = args.size();
  }
  // fill in the extracted args
  while (argc > 1) {
    int option_index;
    int c = getopt_long(argc, argv, "hp:c:w:l:s:m:t:i:o:f:d:",
                        long_options, &option_index);

    if (c == -1) break;
    switch (c) {
      case 'w':
        enable_workstealing = atoi(optarg);
        break;
      case 's':
        sta = atoi(optarg);
        break;
      case 'l':
        enable_local_workstealing = atoi(optarg);
        break;
      case 'm':
        perf_model::mold = atoi(optarg);
        break;
      case 'p':
        use_performance_modeling = atoi(optarg);
        break;
      case 'c':
        perf_model::minimize_parallel_cost = atoi(optarg);;
        break;
      case 't':
        config::nthreads = atoi(optarg);
        break;
      case 'i':
        config::steal_attempts = atoi(optarg);
        break;
      case 'h':
        config::usage(argv[0]);
        abort();
      case 'o':
        perf_model::old_tick_weight = atoi(optarg);
        break;
      case 'f':
        perf_model::refresh_frequency = atoi(optarg);
        break;
      case 'd':
        delete_executed_taos = atoi(optarg);
        break;
       default:
        config::usage(argv[0]);
        abort();
      }
  }
  if(args.size() > 1) {
    for(int i = 1; i < args.size(); ++i) delete[] args[i];
  }
}

void config::usage(char* name) {
  fprintf(stderr,
          "Usage: %s [options]\n"
          "Long option (short option)               : Description (Default value)\n"
          " --wstealing (-w) [0/1]                  : Enable/Disable work-stealing (%d)\n"
          " --lwstealing (-l)[0/1]                  : Enable/Disable cluster-level work-stealing (%d)\n"
          " --perfmodel (-p) [0/1]                  : Enable/Disable performance modeling  (%d)\n"
          " --sta (-s) [0/1]                        : Enable/Disable capturing sta  (%d)\n"
          " --nthreads (-t)                         : The number of worker threads (%d)\n"
          " --idletries (-i)                        : The number of idle tries before a steal attempt (%d)\n"
          " --minparcost (-c) [0/1]                 : model 1 (parallel cost) - 0 (parallel time) (%d)\n"
          " --oldtickweight (-o)                    : Weight of old tick versus new tick (%d)\n"
          " --refreshtablefreq (-f)                 : How often to attempt a random moldability to heat the table (%d)\n"
          " --dealloctaos (-d)[0/1]                 : The runtime deletes executed taos (%d)\n"
          " --mold (-m)                             : Enable/Disable dynamic moldability (%d)\n"
          " --help (-h)                             : Show this help document\n",
          name,
          config::enable_workstealing,
          config::enable_local_workstealing,
          config::use_performance_modeling,
          config::sta,
          config::nthreads,
          config::steal_attempts,
          perf_model::minimize_parallel_cost,
          perf_model::old_tick_weight,
          perf_model::refresh_frequency,
          config::delete_executed_taos,
          perf_model::mold
          );
}

void config::enable_stealing(int idle_tries_before_steal_count = STEAL_ATTEMPTS) {
  config::enable_workstealing = true;
  config::steal_attempts = idle_tries_before_steal_count;
}
void config::disable_stealing() {
  config::enable_workstealing = false;
}
void config::enable_performance_modeling() {
  config::use_performance_modeling = true;
}
void config::disable_performancem_modeling() {
  config::use_performance_modeling = false;
}
void config::enable_performance_modeling(bool min_par_cost, 
                                         int table_refresh_rate, 
                                         int old_tck_weight) {
  config::use_performance_modeling = true;
  perf_model::minimize_parallel_cost = min_par_cost;
  perf_model::refresh_frequency      = table_refresh_rate;
  perf_model::old_tick_weight     = old_tck_weight;
}

void config::print_configs() { 
  if(verbose) {
    cout << "***************************************************" << endl;
    cout << "XiTAO is running with the following configurations:" << endl;
    cout << "***************************************************" << endl;
    formatted_print("Number of threads", config::nthreads);
    formatted_print("Work stealing", ((config::enable_workstealing)? "enabled" : "disabled"));
    formatted_print("Cluster w-stealing", ((config::enable_local_workstealing)? "enabled" : "disabled"));
    formatted_print("Performance model", ((config::use_performance_modeling)? "enabled" : "disabled"));
    formatted_print("Capturing STA", ((config::sta == 1)? "enabled" : "disabled"));
    if(config::enable_workstealing) {
      formatted_print("Idle tries before steal", config::steal_attempts);
    }
    if(config::use_performance_modeling) {
      formatted_print("Min parallel cost", ((perf_model::minimize_parallel_cost)? "enabled" : "disabled"));
      formatted_print("Refresh ptt entries", perf_model::refresh_frequency);
      formatted_print("Old tick weights", perf_model::old_tick_weight);
      formatted_print("Dynamic moldability", perf_model::mold);
    }
  }
  cout << "***************************************************" << endl;
}
