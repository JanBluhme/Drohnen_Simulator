#pragma once

namespace dp {
namespace priority {
    enum class Nicenes : int {
        Highest = -20,
        Default = 0,
        Lowest  = 19,
    };

    enum class RealTimePriority : int {
        Lowest  = 1,
        Default = 1,
        Highest = 99,
    };
    enum class Inheritance { Inherit, Reset };
}   // namespace priority

void              make_process_very_important();
void              set_real_time_scheduler(priority::RealTimePriority priority, priority::Inheritance pi);
void              set_default_scheduler(priority::Inheritance pi);
void              set_nicenes(priority::Nicenes nicenes);
priority::Nicenes get_current_highest_nicenes();
}   // namespace dp
