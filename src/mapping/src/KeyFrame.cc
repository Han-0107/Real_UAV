#include <list>
#include "KeyFrame.h"

KeyFrame::KeyFrame(Eigen::Matrix3d _K,Eigen::Vector4d _D): mK(_K),mD(_D)
{
    
}


void KeyFrame::setPose(Eigen::Matrix3d R, Eigen::Vector3d t)
{
    mRwc = R;
    mtwc = t;
}

void KeyFrame::cptPointCloudc(cv::Mat Img, cv::Mat Depth)
{
    //相机内参
    double fx = mK(0,0);
    double fy = mK(1,1);
    double cx = mK(0,2);
    double cy = mK(1,2);
    
    //创建该关键帧的归一化坐标系下的点云
    list<PointT> linitPts;
    for(int u=0; u<Img.cols; u+=10)
        for(int v=0; v<Img.rows; v+=10)
        {
            PointT pt;
            
            pt.x = (u-cx)/fx;
            pt.y = (v-cy)/fy;
            pt.z = 1;
            
            pt.b = Img.ptr<uchar>(v)[u*3];
            pt.g = Img.ptr<uchar>(v)[u*3+1];
            pt.r = Img.ptr<uchar>(v)[u*3+2];
           
            linitPts.push_back(pt);
        }
    
    //点云去畸变(因为我们用小觅的输入图像是带畸变的图像，如果你的图像本身就已经不带畸变，就可以注释掉这个函数)
    //undistortPoints(linitPts);
    
    //计算相机坐标系下的点云
    PointCloud::Ptr curPCc(new PointCloud);
    for(auto it=linitPts.begin();it!=linitPts.end();it++)
    {
        double x =(*it).x;
        //double y =(*it).y;
        double y =(*it).y;
        
        int u = fx*x + cx;
        int v = fy*y + cy;
        
        if(u<=0 || u>=Img.cols) continue;
        if(v<=0 || v>=Img.rows) continue;
        
        double d = Depth.ptr<uint16_t>(v)[u];
        
        if(d<=300 || d>=3000) continue;

        d /= 1000;
        
        (*it).x = d*x;
        (*it).y = d*y;
        (*it).z = d;
        
        curPCc->push_back(*it);
    }
    curPCc->is_dense = false;

    //点云去外点
    pcl::StatisticalOutlierRemoval<PointT> sta_filter;
    PointCloud::Ptr sta_tmp(new PointCloud);
    sta_filter.setInputCloud(curPCc);
    sta_filter.setMeanK(20); //20
    sta_filter.setStddevMulThresh(0.2); //0.2
    sta_filter.filter(*sta_tmp);
    
    mPointCloudc = *sta_tmp;
}

void KeyFrame::undistortPoints(list<PointT> &lPts)
{    
    //相机畸变参数
    double k1 = mD(0);
    double k2 = mD(1);
    double p1 = mD(2);
    double p2 = mD(3);
    
    //点云去畸变
    for(auto it=lPts.begin();it!=lPts.end();it++)
    {
        double x = (*it).x;
        double y = (*it).y;
        double r2 = x*x + y*y;
        double r4 = r2*r2;
        
        double x_ = x*(1 + k1*r2 + k2*r4) + 2*p1*x*y + p2*(r2 + 2*x*x);
        double y_ = y*(1 + k1*r2 + k2*r4) + 2*p2*x*y + p1*(r2 + 2*y*y);
        
        (*it).x = x_;
        (*it).y = y_;
    }
}

PointCloud::Ptr KeyFrame::getPointCloudw()
{
    //获取世界坐标系下的点云
    PointCloud::Ptr pPointCloudw(new PointCloud);
    
    for(auto it=mPointCloudc.begin();it!=mPointCloudc.end();it++)
    {
        Eigen::Vector3d ptc;
        PointT ptw;
        ptc.x() = (*it).z;
        ptc.y() = -(*it).x;
        ptc.z() = -(*it).y;
        
        ptc = mRwc*ptc + mtwc;
        
        ptw.x = ptc.x();
        //ptw.y = ptc.y();
        ptw.y = ptc.y();
        ptw.z = ptc.z();
        
        ptw.b = (*it).b;
        ptw.g = (*it).g;
        ptw.r = (*it).r;
        
        pPointCloudw->push_back(ptw);
    }
    
    return pPointCloudw;   
}

Eigen::Matrix3d KeyFrame::getRotation()
{
    return mRwc;
}

Eigen::Vector3d KeyFrame::getTranslation()
{
    return mtwc;
}

