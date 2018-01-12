#include <iostream>

#include "tracking.hpp"
#include "fp16.h"

Tracker::Tracker(std::string graphPath, std::vector<std::string> label, int dim)
{
    mvncStatus retCode;
    char devName[100];
    unsigned int graphFileLen;
    void *graphFileBuf;

    labels = label;
    networkDim = dim;

    //buscando stick
    retCode = mvncGetDeviceName(0, devName, 100);
    if (retCode != MVNC_OK)
    {
        std::cout << "Error. No se ha encontrado ningun NCS" << std::endl;
        std::cout << "\tmvncStatus: " << retCode << std::endl;
        std::exit(-1);
    }

    //intentando abrir dispositivo usando nombre encontrado
    retCode = mvncOpenDevice(devName, &deviceHandle);
    if (retCode != MVNC_OK)
    {
        std::cout << "Error. No se ha podido acceder al dispositivo NCS." << std::endl;
        std::cout << "\tmvncStatus: " << retCode << std::endl;
        std::exit(-1);
    }

    //dispositivo listo para usarse
    // printf("Dispositoivo NCS preparado\n");

    graphFileBuf = LoadGraphFile(graphPath.c_str(), &graphFileLen);

    // allocate the graph
    retCode = mvncAllocateGraph(deviceHandle, &graphHandle, graphFileBuf, graphFileLen);
    free(graphFileBuf);
    if (retCode != MVNC_OK)
    { // error allocating graph
        std::cout << "No se ha podido cargar el grafo del fichero: " << graphPath << std::endl;
        std::cout << "\tError: " << retCode << std::endl;
        std::exit(-1);
    }
    std::cout << "NCS up and ready!!!" << std::endl;
}

using namespace cv;
void Tracker::track(cv::Mat frame)
{
    mvncStatus retCode;
    Mat cvImg, aux;
    Size size(300, 300);
    resize(frame, aux, size);
    aux.convertTo(cvImg, CV_32FC3, 1 / 255.0);

    std::vector<float> array;
    if (cvImg.isContinuous())
    {
        array.assign((float *)cvImg.datastart, (float *)cvImg.dataend);
    }
    else
    {
        for (int i = 0; i < cvImg.rows; ++i)
        {
            array.insert(array.end(), cvImg.ptr<float>(i), cvImg.ptr<float>(i) + cvImg.cols);
        }
    }

    //NORMALIZE
    float *cvImg32 = (float *)malloc(((int)array.size()) * sizeof(float));
    for (int i = 0; i < (int)array.size() / 3; i++)
    {
        cvImg32[3 * i] = (array[3 * i] * 2.0f - 1.0f);
        cvImg32[3 * i + 1] = (array[3 * i + 1] * 2.0f - 1.0f);
        cvImg32[3 * i + 2] = (array[3 * i + 2] * 2.0f - 1.0f);
    }

    half *image = (half *)malloc(((int)array.size()) * sizeof(half));
    unsigned int lenImage = ((int)array.size());
    floattofp16((unsigned char *)(void *)image, cvImg32, lenImage);
    // free(cvImg32);
    // half *image = LoadImage(TEST_IMAGE, networkDim);

    retCode = mvncLoadTensor(graphHandle, image, lenImage, NULL);
    free(image);
    if (retCode != MVNC_OK)
    { // error loading tensor
        std::cout << "No se ha podido cargar la imagen: " << std::endl;
        std::cout << "\tError: " << retCode << std::endl;
        std::exit(-1);
    }

    void *resultData16;
    void *userParam;
    unsigned int lenResultData;
    retCode = mvncGetResult(graphHandle, &resultData16, &lenResultData, &userParam);
    if (retCode != MVNC_OK)
    {
        std::cout << "Error - No se ha podido obtener el resultado de la imagen" << std::endl;
        std::cout << "\tError: " << retCode << std::endl;
        std::exit(-1);
    }

    // convert half precision floats to full floats
    int numResults = lenResultData / sizeof(half);
    float *resultData32;
    resultData32 = (float *)malloc(numResults * sizeof(float));

    fp16tofloat(resultData32, (unsigned char *)resultData16, numResults);
    float len = resultData32[0];

    detections.clear();
    for (int i = 0; i < len; i++)
    {
        int index = 7 + i * 7;

        if (resultData32[index] != INFINITY && resultData32[index + 1] != INFINITY && resultData32[index + 1] != INFINITY && resultData32[index + 2] != INFINITY && resultData32[index + 3] != INFINITY && resultData32[index + 4] != INFINITY && resultData32[index + 5] != INFINITY && resultData32[index + 6] != INFINITY && !isnanf(resultData32[index]) && !isnanf(resultData32[index + 1]) && !isnanf(resultData32[index + 1]) && !isnanf(resultData32[index + 2]) && !isnanf(resultData32[index + 3]) && !isnanf(resultData32[index + 4]) && !isnanf(resultData32[index + 5]) && !isnanf(resultData32[index + 6]))
        {
            float x1, x2, y1, y2, score;
            x1 = resultData32[index + 3] * frame.size().width;
            x2 = resultData32[index + 5] * frame.size().width;
            y1 = resultData32[index + 4] * frame.size().height;
            y2 = resultData32[index + 6] * frame.size().height;

            /* Discard if is out of bounds*/
            if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0)
            {
                continue;
            }

            if (x1 > frame.size().width || y1 > frame.size().height || x2 > frame.size().width || y2 > frame.size().height)
            {
                continue;
            }
            /*******************************/

            /* Discard if dimensions make no sense */
            float rel = (y2 - y1) / (x2 - x1);
            if (rel > 3.0f || rel < 0.33f)
            {
                continue;
            }
            /***************************************/

            // center[0] = (int)round((x1 + x2) / 2);
            // center[1] = (int)round((y1 + y2) / 2);
            score = resultData32[index + 2] * 100.0f;
            // = labels[(int)round(resultData32[index + 1])];

            if (score > 0.6f)
            {
                Box box;
                box.x1 = x1;
                box.x2 = x2;
                box.y1 = y1;
                box.y2 = y2;
                box.score = score;
                box.label = labels[(int)round(resultData32[index + 1])];
                detections.insert(detections.end(), box);
            }
        }
    }
}

std::vector<Box> Tracker::getBoxes()
{
    return detections;
}

Tracker::~Tracker()
{
    mvncStatus retCode;
    retCode = mvncDeallocateGraph(graphHandle);
    graphHandle = NULL;
    if (retCode != MVNC_OK)
    {
        std::cout << "Error. No se ha podido eliminar el grafo en memoria" << std::endl;
        std::cout << "\tCodigo: " << retCode << std::endl;
        std::exit(-1);
    }

    retCode = mvncCloseDevice(deviceHandle);
    deviceHandle = NULL;
    if (retCode != MVNC_OK)
    {
        std::cout << "Error. No se ha podido cerrar el dispositivo NCS" << std::endl;
        std::cout << "\tmvncStatus: " << retCode << std::endl;
        std::exit(-1);
    }

    std::cout << "All shiny and cleaned" << std::endl;
}

void *Tracker::LoadGraphFile(const char *path, unsigned int *length)
{
    FILE *fp;
    char *buf;

    fp = fopen(path, "rb");
    if (fp == NULL)
        return 0;
    fseek(fp, 0, SEEK_END);
    *length = ftell(fp);
    rewind(fp);
    if (!(buf = (char *)malloc(*length)))
    {
        fclose(fp);
        return 0;
    }
    if (fread(buf, 1, *length, fp) != *length)
    {
        fclose(fp);
        free(buf);
        return 0;
    }
    fclose(fp);
    return buf;
}

std::vector<int> Box::center()
{
    std::vector<int> toRet;
    toRet.insert(toRet.end(), (int)round((x1 + x2) / 2));
    toRet.insert(toRet.end(), (int)round((y1 + y2) / 2));
    return toRet;
}