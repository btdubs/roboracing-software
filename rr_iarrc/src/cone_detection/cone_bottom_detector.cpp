#include <cv_bridge/cv_bridge.h>
#include <ros/publisher.h>
#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;

cv_bridge::CvImagePtr cv_ptr;
ros::Publisher pub_cone_detector, pub_debug_img;

int low_H, high_H, low_S, low_V;
int blockSky_height, blockWheels_height, blockBumper_height;
int originalWidth, originalHeight;
int decreasedSize = 400;

cv::Mat kernel(int x, int y) {
    return cv::getStructuringElement(cv::MORPH_RECT, cv::Size(x, y));
}

void blockEnvironment(const cv::Mat& img) {
    cv::rectangle(img, cv::Point(0, 0), cv::Point(img.cols, blockSky_height * decreasedSize / originalHeight),
                  cv::Scalar(0, 0, 0), cv::FILLED);

    cv::rectangle(img, cv::Point(0, img.rows), cv::Point(img.cols, blockWheels_height * decreasedSize / originalHeight),
                  cv::Scalar(0, 0, 0), cv::FILLED);

    cv::rectangle(img, cv::Point(img.cols / 3, img.rows),
                  cv::Point(2 * img.cols / 3, blockBumper_height * decreasedSize / originalHeight), cv::Scalar(0, 0, 0),
                  cv::FILLED);
}

void publishMessage(ros::Publisher pub, const cv::Mat& img, std::string img_type) {
    if (pub.getNumSubscribers() > 0) {
        sensor_msgs::Image outmsg;
        cv_ptr->image = img;
        cv_ptr->encoding = img_type;
        cv_ptr->toImageMsg(outmsg);
        pub.publish(outmsg);
    }
}

cv::Mat getDebugImage(cv::Mat& frame, const cv::Mat& hsv_detected, const cv::Mat& bottom_edges) {
    frame.setTo(cv::Scalar(0, 255, 255), hsv_detected);
    frame.setTo(cv::Scalar(0, 255, 0), bottom_edges);
    return frame;
}

/**
 * Gets the bottom edge of the cones
 * @param msg image from camera
 */
void img_callback(const sensor_msgs::ImageConstPtr& msg) {
    // Convert msg to Mat image
    cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");
    cv::Mat frame = cv_ptr->image;

    originalHeight = frame.rows;
    originalWidth = frame.cols;
    cv::resize(frame, frame, cv::Size(decreasedSize, decreasedSize));

    // Get Orange-HSV Cones
    cv::Mat hsv_frame, orange_found, debug_img;
    cv::cvtColor(frame, hsv_frame, cv::COLOR_BGR2HSV);
    cv::inRange(hsv_frame, cv::Scalar(low_H, low_S, low_V), cv::Scalar(high_H, 255, 255), orange_found);
    blockEnvironment(orange_found);

    // Gets the Bottom of the Orange Color Thresholding
    cv::Mat bottom_edges(orange_found.size(), CV_8UC1, cv::Scalar::all(0));
    for (int col = 0; col < orange_found.cols; col++) {
        for (int row = orange_found.rows - 1; row >= 0; row--) {
            if (orange_found.at<uchar>(row, col) == 255) {
                bottom_edges.at<uchar>(row, col) = 255;
                break;
            }
        }
    }

    // Dilate, make green overlay, and resize image to initial dimensions
    cv::dilate(bottom_edges, bottom_edges, kernel(2, 2));
    debug_img = getDebugImage(frame, orange_found, bottom_edges);
    cv::resize(bottom_edges, bottom_edges, cv::Size(originalWidth, originalHeight));

    // publish Images
    publishMessage(pub_cone_detector, bottom_edges, "mono8");
    publishMessage(pub_debug_img, debug_img, "bgr8");
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "coneBottomDetector");

    ros::NodeHandle nh;
    ros::NodeHandle nhp("~");
    std::string subscription_node;

    nhp.param("orange_low_H", low_H, 5);
    nhp.param("orange_high_H", high_H, 15);
    nhp.param("orange_low_S", low_S, 140);
    nhp.param("orange_low_V", low_V, 140);

    nhp.param("blockSky_height", blockSky_height, 220);
    nhp.param("blockWheels_height", blockWheels_height, 200);
    nhp.param("blockBumper_height", blockBumper_height, 200);

    nhp.param("subscription_node", subscription_node, std::string("image_color_rect"));

    pub_cone_detector = nh.advertise<sensor_msgs::Image>("cones/bottom/detection_img", 1);  // test publish of image
    pub_debug_img = nh.advertise<sensor_msgs::Image>("cones/bottom/debug_img", 1);
    auto img_real = nh.subscribe(subscription_node, 1, img_callback);

    ros::spin();
    return 0;
}
