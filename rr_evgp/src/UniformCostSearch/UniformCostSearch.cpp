#include <rr_evgp/UniformCostSearch.h>

#include <algorithm>
#include <functional>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <queue>
#include <vector>

UniformCostSearch::UniformCostSearch(cv::Mat obstacleGrid, cv::Mat distanceGrid, cv::Point startPt, cv::Point goalPt) {
    obstacleGrid_ = obstacleGrid;
    distanceGrid_ = distanceGrid;
    startPoint_ = startPt;
    goalPoint_ = goalPt;
}

UniformCostSearch::State UniformCostSearch::getStartState() {
    UniformCostSearch::State s;
    s.pt = startPoint_;
    s.cost = 0.0;
    return s;
}

bool UniformCostSearch::isGoalState(UniformCostSearch::State s) {
    if (s.pt == goalPoint_) {
        return true;
    }
    return false;
}

float UniformCostSearch::pointToCost(cv::Point pt) {
    return distanceGrid_.at<float>(pt);  // DistanceTransform outputs CV_32F
}

bool UniformCostSearch::isValidPoint(cv::Point pt) {
    cv::Rect rect(cv::Point(), distanceGrid_.size());
    return rect.contains(pt) && obstacleGrid_.at<uchar>(pt) == UniformCostSearch::FREE_SPACE;
}

std::vector<UniformCostSearch::State> UniformCostSearch::getSuccessors(UniformCostSearch::State s) {
    // returns the 8 point grid around a center point
    std::vector<UniformCostSearch::State> successors;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            if (i != 0 || j != 0) {  // not center position
                cv::Point pt(s.pt.x + i, s.pt.y + j);
                if (this->isValidPoint(pt)) {
                    UniformCostSearch::State newState;
                    newState.pt = pt;
                    newState.cost = this->pointToCost(pt);
                    successors.push_back(newState);
                }
            }
        }
    }
    return successors;
}

std::vector<cv::Point> UniformCostSearch::search() {
    std::priority_queue<UniformCostSearch::State, std::vector<UniformCostSearch::State>,
                        std::greater<UniformCostSearch::State>>
          pq;  // minimum priority queue
    cv::Mat visited(obstacleGrid_.size(), CV_8UC1, cv::Scalar(0));

    pq.push(this->getStartState());
    // uniform cost search
    while (!pq.empty()) {
        UniformCostSearch::State e = pq.top();
        if (visited.at<uchar>(e.pt) == 0) {
            if (this->isGoalState(e)) {
                return e.path;
            }
            std::vector<UniformCostSearch::State> successors = this->getSuccessors(e);
            for (UniformCostSearch::State s : successors) {
                UniformCostSearch::State newState;
                newState.pt = s.pt;
                newState.path = e.path;
                newState.path.push_back(s.pt);
                newState.cost = e.cost + s.cost;
                pq.push(newState);
            }
            visited.at<uchar>(e.pt) = 255;
        }
        pq.pop();  // remove it now that we are done
    }
    return std::vector<cv::Point>();  // no path found
}

void UniformCostSearch::setStartPoint(cv::Point pt) {
    startPoint_ = pt;
}

void UniformCostSearch::setGoalPoint(cv::Point pt) {
    goalPoint_ = pt;
}

// BFS from initPoint to find a free space nearby
cv::Point UniformCostSearch::getNearestFreePointBFS(cv::Point initPoint) {
    std::queue<cv::Point> queue;
    cv::Mat visited(obstacleGrid_.size(), CV_8UC1, cv::Scalar(0));
    queue.push(initPoint);

    while (!queue.empty()) {
        cv::Point currPoint = queue.front();
        if (visited.at<uchar>(currPoint) == 0) {
            if (obstacleGrid_.at<uchar>(currPoint) == UniformCostSearch::FREE_SPACE) {
                return currPoint;
            }
            // add successors: 8 point grid around a center point
            for (int i = -1; i < 2; i++) {
                for (int j = -1; j < 2; j++) {
                    if (i != 0 || j != 0) {  // not center position
                        cv::Point newPt(currPoint.x + i, currPoint.y + j);
                        cv::Rect rect(cv::Point(), obstacleGrid_.size());
                        if (rect.contains(newPt)) {
                            queue.push(newPt);
                        }
                    }
                }
            }

            visited.at<uchar>(currPoint) = 255;
        }
        queue.pop();
    }
    //#TODO: failure condition?
}
