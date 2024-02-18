#pragma once

#include <ros/ros.h>
#include <string>
#include <eigen3/Eigen/Dense>

extern Eigen::Matrix3d K;
extern Eigen::Vector4d D;
extern Eigen::Matrix3d RIC;
extern Eigen::Vector3d TIC;

extern std::string RGB_TOPIC;
extern std::string DEPTH_TOPIC;
extern std::string POSE_TOPIC;

extern double PCL_VOXEL_RESOLUTION;

void readParameters(ros::NodeHandle &n);
