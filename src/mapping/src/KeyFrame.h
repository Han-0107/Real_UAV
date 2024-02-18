#pragma once
#include <ros/ros.h>
#include <opencv2/opencv.hpp>
#include <Eigen/Geometry> 
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/filters/voxel_grid.h>


using namespace std;
typedef pcl::PointXYZRGBA PointT;
typedef pcl::PointCloud<PointT> PointCloud;

class KeyFrame{

public:
    KeyFrame(Eigen::Matrix3d _K, Eigen::Vector4d _D);
    
public:
    void setPose(Eigen::Matrix3d R, Eigen::Vector3d t);
    
    void cptPointCloudc(cv::Mat Img, cv::Mat Depth);
    
    void undistortPoints(list<PointT> &lPts);

    PointCloud::Ptr getPointCloudw();
    
    Eigen::Matrix3d getRotation();
    
    Eigen::Vector3d getTranslation();
    
private:
    Eigen::Matrix3d mRwc;
    Eigen::Vector3d mtwc;
    Eigen::Matrix3d mK;
    Eigen::Vector4d mD;
    
    PointCloud mPointCloudc;
};
