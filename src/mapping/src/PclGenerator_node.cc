#include <iostream>
#include <ros/ros.h>
#include <stdio.h>
#include <Eigen/Geometry>
#include <cv_bridge/cv_bridge.h>
#include <pcl/conversions.h>
#include <pcl_conversions/pcl_conversions.h>
#include <nav_msgs/Path.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Pose.h>
#include <sensor_msgs/PointCloud2.h>
#include <message_filters/subscriber.h>
#include <message_filters/time_synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#include <map>
#include <queue>

#include "KeyFrame.h"
#include "Parameter.h"

map<ros::Time, KeyFrame> *KFs(new map<ros::Time, KeyFrame>);

PointCloud::Ptr PointCloudw(new PointCloud);
ros::Publisher pub_pcl;


bool checkMotionThrehold(Eigen::Matrix3d Rwc, Eigen::Vector3d twc, KeyFrame last_kf)
{
    bool insertNewKF = false;

    Eigen::Matrix3d Rwl = last_kf.getRotation();
    Eigen::Vector3d twl = last_kf.getTranslation();

    Eigen::Matrix3d Rcl = Rwc.transpose() * Rwl;
    Eigen::Vector3d tcl = Rwc.transpose() * (twl - twc);

    double tcl_norm = tcl.norm();
    double theta = acos((Rcl.trace() - 1) / 2) * 180 / CV_PI;

    bool c1 = (tcl_norm > 0.5 && tcl_norm < 10);
    bool c2 = (theta > 10 && theta < 90);

    if (c1 || c2)
        insertNewKF = true;

    return insertNewKF;
}

//�������ƻ���
void pubPCL(PointCloud::Ptr PointCloud, ros::Time time)
{
    sensor_msgs::PointCloud2 output;
    pcl::toROSMsg(*PointCloud, output);
    output.header.stamp = time;
    output.header.frame_id = "map";
    pub_pcl.publish(output);
}

//����&�������Ƶ����ص�����
void kfs_callback(const sensor_msgs::ImageConstPtr &img_msg, const sensor_msgs::ImageConstPtr &depth_msg, const nav_msgs::OdometryConstPtr &pose_msg)
{
    //rosͼ����Ϣ��ʽ��cvͼ���ʽת��
    cv_bridge::CvImageConstPtr cvpImg, cvpDepth;
    try
    {
        cvpImg = cv_bridge::toCvShare(img_msg);
        cvpDepth = cv_bridge::toCvShare(depth_msg);
    }
    catch (cv_bridge::Exception &e)
    {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return;
    }

    cv::Mat cvImg = cvpImg->image;
    cv::Mat cvDepth = cvpDepth->image;

    cout << "cvdepth type: " << cvDepth.type() << endl; 
    //cout << cvDepth << endl;
    
    //�Ҷ�ͼת��Ϊ��ͼͨ��
    if (cvImg.channels() == 1) {
        cvtColor(cvImg, cvImg, CV_GRAY2BGR);
    }

    // time alignment
    /*map<ros::Time, geometry_msgs::Pose> Poses;
    for (auto it = pose_msg->poses.begin(); it != pose_msg->poses.end(); it++)
        Poses.insert({(*it).header.stamp, (*it).pose});
    auto Pit = Poses.lower_bound(img_msg->header.stamp);

    if (Pit == Poses.end())
        return;
    */
    // ���� Rwc & twc
    double tx = pose_msg->pose.pose.position.x;
    double ty = pose_msg->pose.pose.position.y;
    double tz = pose_msg->pose.pose.position.z;

    double qw = pose_msg->pose.pose.orientation.w;
    double qx = pose_msg->pose.pose.orientation.x;
    double qy = pose_msg->pose.pose.orientation.y;
    //double qz = pose_msg->pose.pose.orientation.z;
    double qz = pose_msg->pose.pose.orientation.z;
/*
    double tx = Pit->second.position.x;
    double ty = Pit->second.position.y;
    double tz = Pit->second.position.z;

    double qw = Pit->second.orientation.w;
    double qx = Pit->second.orientation.x;
    double qy = Pit->second.orientation.y;
    double qz = Pit->second.orientation.z;
*/

    Eigen::Quaterniond Qwi(qw, qx, qy, qz); // imu frame to world frame

    Eigen::Matrix3d Rwi, Rwc;
    Rwi = Qwi.toRotationMatrix();// imu2world

    Eigen::Vector3d twi, twc;
    twi << tx, ty, tz;

    //Rwc = Rwi * RIC;//cameral2world
    //twc = Rwi * TIC + twi;
    Rwc = Rwi;
    twc=twi;

   // �ж���ת��ƽ����ֵ
    bool insertNewKF = false;

    if (!KFs->empty())
    {
        KeyFrame last_kf = KFs->rbegin()->second;
        insertNewKF = checkMotionThrehold(Rwc, twc, last_kf);
    }

    if (insertNewKF || KFs->empty())
    {
        if (KFs->size() == 10)
            auto it = KFs->erase(KFs->begin());//erase old pointcloud
        
        KeyFrame cur_kf(K, D);
        cur_kf.setPose(Rwc, twc);
        cur_kf.cptPointCloudc(cvImg, cvDepth);

        KFs->insert({img_msg->header.stamp, cur_kf});

        PointCloud::Ptr cur_PCw(new PointCloud());
        for (auto it = KFs->begin(); it != KFs->end(); it++)
            (*cur_PCw) += *(it->second.getPointCloudw());// all pointclouds in world_frameId

        double dPvr = PCL_VOXEL_RESOLUTION; //���ƽ������ֱ��ʣ�����ʹ�õ���0.1
        PointCloud::Ptr voxel_tmp(new PointCloud);
        pcl::VoxelGrid<PointT> voxel_filter;// downSample pointcloud
        voxel_filter.setLeafSize(dPvr, dPvr, dPvr);
        voxel_filter.setInputCloud(cur_PCw);
        voxel_filter.filter(*voxel_tmp);// voxel_temp is filter's output
        cur_PCw->swap(*voxel_tmp);// refresh cur_pcw

        pubPCL(cur_PCw, img_msg->header.stamp);
    }    
 
    // ���㲢�������ƻ���
    /*if (insertNewKF || KFs->empty())
    {
        KeyFrame cur_kf(K, D);
        cur_kf.setPose(Rwc, twc);
        cur_kf.cptPointCloudc(cvImg, cvDepth);

        KFs->insert({img_msg->header.stamp, cur_kf});

        //��ȡ��ǰ�ؼ�֡����������ϵ�µĵ���
        PointCloud::Ptr cur_PCw(new PointCloud());
        (*cur_PCw) += *(cur_kf.getPointCloudw());

        //���ƽ�����
        double dPvr = PCL_VOXEL_RESOLUTION; //���ƽ������ֱ��ʣ�����ʹ�õ���0.1
        PointCloud::Ptr voxel_tmp(new PointCloud);
        pcl::VoxelGrid<PointT> voxel_filter;
        voxel_filter.setLeafSize(dPvr, dPvr, dPvr);
        voxel_filter.setInputCloud(cur_PCw);
        voxel_filter.filter(*voxel_tmp);
        cur_PCw->swap(*voxel_tmp);

        (*PointCloudw) += (*cur_PCw);

        //��������
        pubPCL(PointCloudw, img_msg->header.stamp);
    }*/

    //pcl::io::savePCDFileBinary("map.pcd", *PointCloudw);
}


int main(int argc, char **argv)
{
    ros::init(argc, argv, "mapping");
    ros::NodeHandle n("~");
    ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Info);

    readParameters(n);
  
    pub_pcl = n.advertise<sensor_msgs::PointCloud2>("/mapping/pcl_map", 1000);

    //ros 消息时间戳对齐
    message_filters::Subscriber<sensor_msgs::Image> sub_img(n, RGB_TOPIC, 2000);
    message_filters::Subscriber<sensor_msgs::Image> sub_depth(n, DEPTH_TOPIC, 2000);
    message_filters::Subscriber<nav_msgs::Odometry> sub_pose(n, POSE_TOPIC, 2000);

    typedef message_filters::sync_policies::ApproximateTime<sensor_msgs::Image, sensor_msgs::Image, nav_msgs::Odometry> sync_pol;
    message_filters::Synchronizer<sync_pol> sync(sync_pol(1000), sub_img, sub_depth, sub_pose);
    sync.registerCallback(boost::bind(kfs_callback, _1, _2, _3));
    //boost::bind used to pass more pram to callback function

    //pcl::io::savePCDFileBinary("map.pcd", *PointCloudw);

    ros::spin();

    ros::shutdown();

    return 0;
}
