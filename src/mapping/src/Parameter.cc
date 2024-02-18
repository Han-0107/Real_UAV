#include "Parameter.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/eigen.hpp>

Eigen::Matrix3d K;
Eigen::Vector4d D;
Eigen::Matrix3d RIC;
Eigen::Vector3d TIC;

std::string RGB_TOPIC;
std::string DEPTH_TOPIC;
std::string POSE_TOPIC;

double PCL_VOXEL_RESOLUTION;

template <typename T>
T readParam(ros::NodeHandle &n, std::string name)
{
    T ans;
    if (n.getParam(name, ans))
    {
        ROS_INFO_STREAM("Loaded " << name << ": " << ans);
    }
    else
    {
        ROS_ERROR_STREAM("Failed to load " << name);
        n.shutdown();
    }
    return ans;
}

void readParameters(ros::NodeHandle &n)
{
    std::string config_file;
    config_file = readParam<std::string>(n, "config_file");
    cv::FileStorage fsSettings(config_file, cv::FileStorage::READ);
    if(!fsSettings.isOpened())
    {
        std::cerr << "Wrong Wrong Wrong config path" << std::endl;
    }

    fsSettings["rgb_topic"] >> RGB_TOPIC;
    fsSettings["pose_topic"] >> POSE_TOPIC;
    fsSettings["depth_topic"] >> DEPTH_TOPIC;

    cv::Mat cv_R, cv_T, cv_K, cv_D;
    //获取畸变参数
    fsSettings["internalDistortion"] >> cv_D;
    //获取相机内参
    fsSettings["internalProjection"] >> cv_K;
    //获取相机外参
    fsSettings["extrinsicRotation"] >> cv_R;
    fsSettings["extrinsicTranslation"] >> cv_T;
    
    cv::cv2eigen(cv_D,D);
    cv::cv2eigen(cv_K,K);
    cv::cv2eigen(cv_R, RIC);
    cv::cv2eigen(cv_T, TIC);
    
    fsSettings["pcl_voxel_resolution"] >> PCL_VOXEL_RESOLUTION;
    
    fsSettings.release();
}
