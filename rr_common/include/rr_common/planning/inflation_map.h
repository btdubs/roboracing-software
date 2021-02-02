#pragma once

#include <nav_msgs/OccupancyGrid.h>
#include <parameter_assertions/assertions.h>
#include <tf/tf.h>
#include <tf/transform_datatypes.h>
#include <tf/transform_listener.h>

#include "map_cost_interface.h"
#include "planner_types.hpp"
#include "rectangle.hpp"

namespace rr {

class InflationMap : public MapCostInterface {
  public:
    explicit InflationMap(ros::NodeHandle nh);

    double DistanceCost(const Pose& pose) override;

  private:
    void SetMapMessage(const nav_msgs::OccupancyGridConstPtr& map_msg);

    ros::Subscriber map_sub;
    nav_msgs::OccupancyGridConstPtr map;
    Rectangle hit_box;
    std::unique_ptr<tf::TransformListener> listener;
    tf::StampedTransform transform;
    int lethal_threshold;
};

}  // namespace rr
