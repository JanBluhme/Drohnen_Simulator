#include "dp_lib/util/priority.hpp"

#include "dp_lib/util/raise.hpp"

#include <algorithm>
#include <sched.h>
#include <sys/resource.h>

namespace dp {
namespace {
    void set_scheduler(int algorithm, int priority, priority::Inheritance pi) {
        int const maxPrio = ::sched_get_priority_max(algorithm);
        if(-1 == maxPrio) { DPRAISE_SYSTEM_ERROR("sched_get_priority_max failed"); }
        int const minPrio = ::sched_get_priority_min(algorithm);
        if(-1 == minPrio) { DPRAISE_SYSTEM_ERROR("sched_get_priority_min failed"); }
        priority = std::clamp(priority, std::min(maxPrio, minPrio), std::max(maxPrio, minPrio));
        sched_param schedp{};
        schedp.sched_priority     = priority;
        int const inheritanceFlag = pi == priority::Inheritance::Reset ? SCHED_RESET_ON_FORK : 0;
        //NOLINTNEXTLINE(hicpp-signed-bitwise)
        if(-1 == ::sched_setscheduler(0, algorithm | inheritanceFlag, &schedp)) {
            DPRAISE_SYSTEM_ERROR("sched_setscheduler failed");
        }
    }
}   // namespace

void make_process_very_important() {
    set_real_time_scheduler(priority::RealTimePriority::Highest, priority::Inheritance::Inherit);
    set_nicenes(priority::Nicenes::Highest);
}

void set_default_scheduler(priority::Inheritance pi) {
    set_scheduler(SCHED_BATCH, 0, pi);
}

void set_real_time_scheduler(priority::RealTimePriority priority, priority::Inheritance pi) {
    set_scheduler(SCHED_FIFO, static_cast<int>(priority), pi);
}

void set_nicenes(priority::Nicenes nicenes) {
    nicenes = std::clamp(nicenes,
                        std::min(priority::Nicenes::Highest, priority::Nicenes::Lowest),
                        std::max(priority::Nicenes::Highest, priority::Nicenes::Lowest));
    if(-1 == ::setpriority(PRIO_PROCESS, 0, static_cast<int>(nicenes))) { DPRAISE_SYSTEM_ERROR("setpriority failed"); }
}

priority::Nicenes get_current_highest_nicenes() {
    rlimit limit{};
    if(-1 == ::getrlimit(RLIMIT_NICE, &limit)) { DPRAISE_SYSTEM_ERROR("getrlimit failed"); }
    return priority::Nicenes(20 - limit.rlim_cur);
}
}   // namespace dp
