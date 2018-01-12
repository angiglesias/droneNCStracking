#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/imgproc.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mvnc.h>
#include <cmath>

typedef unsigned short half;

class Box
{
  public:
    int x1;
    int x2;
    int y1;
    int y2;
    float score;
    std::string label;
    std::vector<int> center();
};

class Tracker
{
  public:
    Tracker(std::string graph, std::vector<std::string> labels, int dim);
    ~Tracker();
    void track(cv::Mat frame);
    std::vector<Box> getBoxes(float score, float minRel, float maxRel);
    std::vector<Box> getBoxes();

  private:
    int networkDim;
    std::vector<std::string> labels;
    void *graphHandle;
    void *deviceHandle;
    std::vector<Box> detections;
    void *LoadGraphFile(const char *path, unsigned int *length);
};
