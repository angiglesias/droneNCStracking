#include "tracking.hpp"

// #include <opencv2/core.hpp>
// #include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <cstdio>
// #include "opencv2/imgproc.hpp"

int main(int argc, char **argv)
{
    /*init tracking*/
    std::vector<std::string> lbls;
    lbls.insert(lbls.end(), "background");
    lbls.insert(lbls.end(), "marcador");
    // Tracker *ncs = new Tracker(std::string("resources/graph"), lbls, 300);
    Tracker *ncs = new Tracker(std::string("resources/graph"), lbls, 300);

    /*init video capture*/
    cv::VideoCapture video("resources/video0.avi");
    cv::VideoWriter out("result.avi", CV_FOURCC('M', 'J', 'P', 'G'), 20, cv::Size(video.get(CV_CAP_PROP_FRAME_WIDTH), video.get(CV_CAP_PROP_FRAME_HEIGHT)));
    cv::Mat frame;

    for (;;)
    {
        video >> frame;
        if (frame.empty())
            break;
        ncs->track(frame);
        std::vector<Box> detections = ncs->getBoxes();
        for (int i = 0; i < (20 <= detections.size() ? 20 : detections.size()); i++)
        {
            cv::Point p1(detections[i].x1, detections[i].y1), p2(detections[i].x2, detections[i].y2);
            cv::rectangle(frame, p1, p2, cv::Scalar(255, 0, 0), 2);
            cv::Scalar col(125, 175, 75);
            cv::Size labelsize = cv::getTextSize(std::to_string(detections[i].score), cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, NULL);
            // rectangle(img, Point(pt1.x - 1, pt1.y - labelsize.height - 1), Point(pt1.x + labelsize.width + 1, pt1.y + labelsize.height + 1), col, -1);
            cv::putText(frame, std::to_string(detections[i].score), cv::Point(p1.x, p1.y + labelsize.height), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
            printf("\tBox%d[x1:%d, y1:%d, x2:%d, y2:%d, center: (%d,%d)] score: %f label: %s\n", i, detections[i].x1, detections[i].y1, detections[i].x2, detections[i].y2, detections[i].center()[0], detections[i].center()[1], detections[i].score, detections[i].label.c_str());
        }
        out.write(frame);
    }
    // delete ncs;
}