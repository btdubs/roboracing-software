#include <cv_bridge/cv_bridge.h>
#include <ros/publisher.h>
#include <ros/ros.h>
#include <rr_msgs/speed.h>
#include <rr_msgs/steering.h>
#include <sensor_msgs/Image.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <opencv2/opencv.hpp>

#include "PID.h"

using namespace std;

ros::Publisher pub_line_detector;
ros::Publisher speed_pub;
ros::Publisher steer_pub;

rr_msgs::speed speed_message;
rr_msgs::steering steer_message;

// PID IMPLEMENATION SETUP
double kP;
double kI;
double kD;
double setpoint;
double input;
double outputSteering;
PID myPID(&input, &outputSteering, &setpoint, 0.0, 0.0, 0.0, P_ON_E, REVERSE);

double speedGoal;
bool useHistogramFinder;

cv::Mat kernel(int x, int y) {
    return cv::getStructuringElement(cv::MORPH_RECT, cv::Size(x, y));
}

cv::Mat getColHist(cv::Mat img) {
    cv::Mat1i pixels(img.cols, 1);
    for (int x = 0; x < img.cols; x++) {
        cv::Mat col = img.col(x);
        pixels(x, 0) = cv::countNonZero(col);
    }
    return pixels;
}

// re-center around the average x coordinate of the line segment
// returns true if he currCenter changed.
bool centerOnLineSegment(cv::Mat imGray, cv::Point& currCenter, cv::Point offset, int width, int height) {
    cv::Point topLeft = currCenter - offset;
    int sumX = 0;
    int count = 0;
    for (int y = topLeft.y; y < topLeft.y + height; y++) {
        for (int x = topLeft.x; x < topLeft.x + width; x++) {
            if ((int)imGray.at<unsigned char>(y, x) > 0) {
                sumX += x;
                count++;
            }
        }
    }
    if (count != 0) {  //#TODO: set a minimum threshold of pixels
        // we found a line segment
        int avgX = sumX / count;
        currCenter.x = avgX;
        return true;
    }

    return false;
}

// find the "center line" to follow
std::vector<cv::Point> createCenterLine(std::vector<cv::Point> leftLine, std::vector<cv::Point> rightLine) {
    std::vector<cv::Point> centerLine;
    for (int i = 0; i < leftLine.size(); i++) {
        int x = cvRound((leftLine[i].x + rightLine[i].x) / 2);
        int y = cvRound((leftLine[i].y + rightLine[i].y) / 2);
        cv::Point center(x, y);
        centerLine.push_back(center);
    }

    return centerLine;
}

/**
 * Use histogram to find start of lines, then use sliding window to track line
 */
void img_callback(const sensor_msgs::ImageConstPtr& msg) {
    // Convert msg to Mat image
    cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, "mono8");
    cv::Mat frame = cv_ptr->image;
    cv::Mat output;
    cv::cvtColor(frame, output,
                 cv::COLOR_GRAY2BGR);  // Doing this just for debugging

    setpoint = frame.cols / 2;  // want our center line on the center of the camera
    cv::Point rightMaxLoc;
    cv::Point leftMaxLoc;

    if (useHistogramFinder) {
        // locate beginnings of lines by a large number of pixels in the column
        cv::Mat hist = getColHist(frame);
        // left line
        double max;
        double min;
        cv::Point rightMinLoc;
        cv::Point leftMinLoc;
        cv::minMaxLoc(hist(cv::Range(0, hist.rows / 2 - 1), cv::Range::all()), &min, &max, &leftMinLoc, &leftMaxLoc);
        leftMaxLoc.x = leftMaxLoc.y;  // gotta flip x and y
        leftMaxLoc.y = frame.rows - 1;
        cv::minMaxLoc(hist(cv::Range(hist.rows / 2, hist.rows - 1), cv::Range::all()), &min, &max, &rightMinLoc,
                      &rightMaxLoc);
        rightMaxLoc.x = rightMaxLoc.y + hist.rows / 2;
        rightMaxLoc.y = frame.rows - 1;

        if (rightMaxLoc.x == frame.cols / 2) {
            rightMaxLoc.x = frame.cols - 1;  // handle line not found
        }
    } else {
        // locate beginnings of lines by centering from search window
        leftMaxLoc.x = frame.cols / 4;
        rightMaxLoc.x = frame.cols / 2 + frame.cols / 4;
        rightMaxLoc.y = 80;  // 120
        leftMaxLoc.y = 80;
        int w = (frame.cols) / 2;
        bool rightFound = centerOnLineSegment(frame, rightMaxLoc, cv::Point(w / 2, 32), w - 1, 16);
        bool leftFound = centerOnLineSegment(frame, leftMaxLoc, cv::Point(w / 2, 32), w - 1, 16);
        if (!rightFound) {
            rightMaxLoc.x = frame.cols / 2 + 40;
        }
        if (!leftFound) {
            leftMaxLoc.x = frame.cols / 2 - 40;
        }
    }

    // debug visualization
    cv::circle(output, leftMaxLoc, 8, cv::Scalar(0, 0, 255), -1);
    cv::circle(output, rightMaxLoc, 8, cv::Scalar(0, 255, 255), -1);

    int width = 40;  //#TODO: this will be updated to not be hardcoded.
    int height = 16;

    cv::Point offset(width / 2, height / 2);

    cv::Point rightCenter = rightMaxLoc;
    rightCenter.y = frame.rows - height / 2;

    std::vector<cv::Point> rightLinePoints;
    rightLinePoints.push_back(rightCenter);

    cv::Point leftCenter = leftMaxLoc;
    leftCenter.y = frame.rows - height / 2;

    std::vector<cv::Point> leftLinePoints;
    leftLinePoints.push_back(leftCenter);

    while (rightCenter.y >= 0) {
        centerOnLineSegment(frame, rightCenter, offset, width, height);
        cv::circle(output, rightCenter, 2, cv::Scalar(0, 0, 255), -1);
        cv::Point rightBox_topLeft = rightCenter - offset;
        cv::rectangle(output, rightBox_topLeft, rightCenter + offset, cv::Scalar(0, 255, 0), 1);
        rightCenter.y -= height;

        rightLinePoints.push_back(rightCenter);

        centerOnLineSegment(frame, leftCenter, offset, width, height);
        cv::circle(output, leftCenter, 2, cv::Scalar(0, 0, 255), -1);
        cv::Point leftBox_topLeft = leftCenter - offset;
        cv::rectangle(output, leftBox_topLeft, leftCenter + offset, cv::Scalar(0, 255, 0), 1);
        leftCenter.y -= height;

        leftLinePoints.push_back(leftCenter);
    }

    std::vector<cv::Point> centerLane = createCenterLine(leftLinePoints, rightLinePoints);
    cv::polylines(output, centerLane, false, cv::Scalar(255, 0, 0), 2);

    // find error, P term. Maybe add curvature and stuff
    cv::Point goal = centerLane[centerLane.size() / 2];
    int error = (frame.cols / 2) - goal.x;

    // double steering = error * 0.01; //kP
    input = static_cast<double>(goal.x);
    myPID.Compute();
    double steering = outputSteering;

    // debug visualization
    cv::putText(output, std::to_string(steering), cv::Point(20, 100), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(0, 255, 0),
                1);
    cv::line(output, cv::Point(frame.cols / 2, 0), cv::Point(frame.cols / 2, frame.rows - 1), cv::Scalar(0, 255, 255),
             1);                                                                          // center
    cv::line(output, goal, cv::Point(frame.cols / 2, goal.y), cv::Scalar(0, 0, 255), 1);  // error amount

    speed_message.speed = speedGoal;
    steer_message.angle = steering;
    speed_pub.publish(speed_message);
    steer_pub.publish(steer_message);

    // Publish Message
    if (pub_line_detector.getNumSubscribers() > 0) {
        sensor_msgs::Image outmsg;
        cv_ptr->image = output;
        cv_ptr->encoding = "bgr8";
        cv_ptr->toImageMsg(outmsg);
        pub_line_detector.publish(outmsg);
    }
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "drag_center_lane_planner");

    ros::NodeHandle nh;
    ros::NodeHandle nhp("~");
    std::string subscription_node;
    nhp.param("subscription_node", subscription_node,
              std::string("/camera_center/image_color_rect/lines/detection_img_transformed"));

    nhp.param("PID_kP", kP, 0.0001);
    nhp.param("PID_kI", kI, 0.0);
    nhp.param("PID_kD", kD, 0.0);
    nhp.param("speed", speedGoal, 1.0);

    nhp.param("useHistogramFinder", useHistogramFinder, false);

    double maxTurnLimit;
    nhp.param("maxTurnLimitRadians", maxTurnLimit, 0.44);

    // setup PID controllers
    myPID.SetTunings(kP, kI, kD);
    myPID.SetMode(AUTOMATIC);
    myPID.SetOutputLimits(-maxTurnLimit, maxTurnLimit);

    pub_line_detector = nh.advertise<sensor_msgs::Image>("/drag_centerline_track", 1);  // test publish of image
    auto img_real = nh.subscribe(subscription_node, 1, img_callback);

    speed_pub = nh.advertise<rr_msgs::speed>("/plan/speed", 1);
    steer_pub = nh.advertise<rr_msgs::steering>("/plan/steering", 1);

    ros::spin();
    return 0;
}
