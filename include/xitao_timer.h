#ifndef xitao_timer_h
#define xitao_timer_h
#include <map>
#include <sys/time.h>

namespace xitao {
  static timeval timer_obj;                                     //!< Time value
  static std::map<std::string,timeval> timer;                   //!< Map of timer event name to time value

  //! Start timer for given event
  void start(std::string event) {
    gettimeofday(&timer_obj, NULL);                             // Get time of day in seconds and microseconds
    timer[event] = timer_obj;                                   // Store in timer
  }

  //! Stop timer for given event
  void stop(std::string event) {
    gettimeofday(&timer_obj, NULL);                              // Get time of day in seconds and microseconds
    printf("%-20s : %f s\n", event.c_str(), timer_obj.tv_sec-timer[event].tv_sec+
           (timer_obj.tv_usec-timer[event].tv_usec)*1e-6);         // Print time difference
  }
}
#endif
