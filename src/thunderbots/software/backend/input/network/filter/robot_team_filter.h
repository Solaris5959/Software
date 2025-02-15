#pragma once

#include "ai/world/team.h"
#include "backend/input/network/filter/robot_filter.h"
#include "geom/angle.h"
#include "geom/point.h"

class RobotTeamFilter
{
   public:
    /**
     * Creates a new Robot Team Filter
     */
    explicit RobotTeamFilter();

    /**
     * Filters the new robot detection data, and returns the updated state of the team
     * given the new data
     *
     * @param current_team_state The current state of the Team
     * @param new_robot_detections A list of new SSL Robot detections
     *
     * @return The updated state of the team given the new data
     */
    Team getFilteredData(const Team& current_team_state,
                         const std::vector<SSLRobotDetection>& new_robot_detections);


    // A map used to store a separate robot filter for each robot on this team, so
    // each robot can be filtered and handled separately
    std::map<int, RobotFilter> robot_filters;
};
