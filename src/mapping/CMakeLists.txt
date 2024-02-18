cmake_minimum_required(VERSION 2.8.3)
project(mapping)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "-std=c++11")
#-DEIGEN_USE_MKL_ALL")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -g")

find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
  cv_bridge
  geometry_msgs
  nav_msgs
  pcl_conversions
  pcl_ros
)

find_package(OpenCV 3.2.0 REQUIRED)
#find_package(octomap REQUIRED)


include_directories(${catkin_INCLUDE_DIRS})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(Eigen3)
include_directories(
  ${catkin_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR}
)

catkin_package()

add_executable(mapping
    src/KeyFrame.cc
    src/Parameter.cc
    src/PclGenerator_node.cc
    )

target_link_libraries(mapping ${catkin_LIBRARIES} ${OpenCV_LIBS} )
