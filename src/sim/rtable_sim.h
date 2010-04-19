#ifndef __RTABLE_SIM_H__
#define __RTABLE_SIM_H__

#include <struct/rtable.h>
#include "rtable_planner.h"

enum rtable_sim_overlay_type {
  RTABLE_SIM_OVERLAY_TYPE_RANDOM,
  RTABLE_SIM_OVERLAY_TYPE_LOCALITY,
};

typedef struct rtable_simulator{
  rtable_planner_t planner;
} rtable_simulator, *rtable_simulator_t;

rtable_simulator_t rtable_simulator_create(const char* filename, int seed);
void rtable_simulator_destroy(rtable_simulator_t simulator);
overlay_rtable_vector_t rtable_simulator_run(rtable_simulator_t simulator, int overlaytype, float density, int rttype, int seed);
overlay_rtable_vector_t rtable_simulator_run2(rtable_simulator_t sim, float density, int rttype, int seed);


#endif // __RTABLE_SIM_H__
