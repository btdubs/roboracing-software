#include <rr_common/planning/inflation_map.h>

namespace rr {

InflationMap::InflationMap(ros::NodeHandle nh)
      : map(), hit_box(ros::NodeHandle(nh, "hitbox")), listener(new tf::TransformListener) {
    std::string map_topic;
    assertions::getParam(nh, "map_topic", map_topic);
    map_sub = nh.subscribe(map_topic, 1, &InflationMap::SetMapMessage, this);

    assertions::getParam(nh, "lethal_threshold", lethal_threshold, { assertions::greater(0), assertions::less(256) });
}

double InflationMap::DistanceCost(const rr::Pose& rr_pose) {
    const tf::Pose pose(tf::createQuaternionFromYaw(rr_pose.theta), tf::Vector3(rr_pose.x, rr_pose.y, 0));
    tf::Pose world_Pose = transform * pose;

    unsigned int mx = std::floor((world_Pose.getOrigin().x() - map->info.origin.position.x) / map->info.resolution);
    unsigned int my = std::floor((world_Pose.getOrigin().y() - map->info.origin.position.y) / map->info.resolution);
    if (my < 0 || my >= map->info.height || mx < 0 || mx >= map->info.width) {
        return 0.0;
    }

    char cost = map->data[my * map->info.width + mx];

    if (!hit_box.PointInside(rr_pose.x, rr_pose.y) && cost > lethal_threshold) {
        return -1.0;
    }

    return cost;
}

void InflationMap::SetMapMessage(const boost::shared_ptr<nav_msgs::OccupancyGrid const>& map_msg) {
    if (!accepting_updates_) {
        return;
    }

    map = map_msg;

    try {
        listener->waitForTransform(map_msg->header.frame_id, "/base_footprint", ros::Time(0), ros::Duration(.05));
        listener->lookupTransform(map_msg->header.frame_id, "/base_footprint", ros::Time(0), transform);
    } catch (tf::TransformException& ex) {
        ROS_ERROR_STREAM(ex.what());
    }

    updated_ = true;
}

}  // namespace rr
