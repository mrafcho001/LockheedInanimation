#include "facetracker.h"
#include <stdexcept>
#include <sstream>
#include <limits>
#include <cmath>

const QRect FaceTracker::InvalidQRect(1,1,0,0);

FaceTracker::FaceTracker()
{
    Init(0);
}

FaceTracker::FaceTracker(int deviceID)
{
    Init(deviceID);
}

void FaceTracker::ResetTracker()
{
    //Sets invalid, empty, and null flags for the QRect
    m_lastPosition.setCoords(1,1,0,0);
}

QRect FaceTracker::GetFacePosition()
{
    m_vc >> cameraFrame_saved;

    cv::Mat cameraFrame;

    if(cameraFrame_saved.empty())
        return InvalidQRect;

    //To Grayscale
    if(cameraFrame_saved.channels() == 3)
        cv::cvtColor(cameraFrame_saved, cameraFrame, CV_BGR2GRAY);
    else if(cameraFrame_saved.channels() == 4)
        cv::cvtColor(cameraFrame_saved, cameraFrame, CV_BGRA2GRAY);

    //Histogram Equalization
    cv::equalizeHist(cameraFrame, cameraFrame);

    std::vector<cv::Rect> faceRects;
    faceDetector.detectMultiScale(cameraFrame, faceRects,
                                  m_searchScaleFactor, m_minNeighbors,
                                  cv::CASCADE_SCALE_IMAGE | m_additionalFlags);

    //No Faces detected
    if(faceRects.size() == 0)
        return InvalidQRect;

    //Select new face to track
    if(!m_lastPosition.isValid())
    {
        m_lastPosition = SelectFace2Track(faceRects);
        return m_lastPosition;
    }

    //Find face closest to where the tracked face was last time
    m_lastPosition = findClosest(faceRects,m_lastPosition.center());

    return m_lastPosition;
}

QRect FaceTracker::SelectFace2Track(std::vector<cv::Rect> faceRects)
{
    return findClosest(faceRects, QPoint(m_imageWidth/2, m_imageHeight/2));
}

int FaceTracker::GetMinFeatureSize()
{
    return m_minFeatureSize.height;
}

void FaceTracker::SetMinFeatureSize(int minFeatureSize)
{
    m_minFeatureSize.height = minFeatureSize;
    m_minFeatureSize.width = minFeatureSize;
}

float FaceTracker::GetSearchScaleFactor()
{
    return m_searchScaleFactor;
}

void FaceTracker::SetSearchScaleFactor(float searchScaleFactor)
{
    m_searchScaleFactor = searchScaleFactor;
}

int FaceTracker::GetMinNeighbors()
{
    return m_minNeighbors;
}

void FaceTracker::SetMinNeighbors(int minNeighbors)
{
    m_minNeighbors = minNeighbors;
}

void FaceTracker::SetProcessingImageDimensions(int width, int height)
{
    m_imageWidth = width;
    m_imageHeight = height;

    m_vc.set(CV_CAP_PROP_FRAME_WIDTH, m_imageWidth);
    m_vc.set(CV_CAP_PROP_FRAME_HEIGHT, m_imageHeight);
}

unsigned int FaceTracker::GetAdditionalFlags()
{
    return m_additionalFlags;
}

void FaceTracker::SetAdditionalFlag(unsigned int flag)
{
    m_additionalFlags |= flag;
}

void FaceTracker::ClearAdditionalFlag(unsigned int flag)
{
    m_additionalFlags &= ~flag;
}

void FaceTracker::SetAdditionalFlags(unsigned int flags)
{
    m_additionalFlags = flags;
}

QImage *FaceTracker::GetLastImage()
{
    cv::cvtColor(cameraFrame_saved, cameraFrame_saved, CV_BGR2RGB);

    return new QImage(cameraFrame_saved.data, cameraFrame_saved.cols,
                  cameraFrame_saved.rows, QImage::Format_RGB888);
}


void FaceTracker::Init(int deviceID)
{
    m_minFeatureSize = cv::Size(DEFAULT_MIN_FEATURE_SIZE,DEFAULT_MIN_FEATURE_SIZE);
    m_searchScaleFactor = DEFAULT_SEARCH_SCALE_FACTOR;
    m_minNeighbors = DEFAULT_MIN_NEIGHBORS_CUTOFF;
    m_additionalFlags = DEFAULT_ADDITIONAL_FLAGS;
    classifier_xml_filename = DEFAULT_CLASSIFIER_XML_FILENAME;

    m_vc.open(deviceID);
    if(!m_vc.isOpened())
    {
        std::ostringstream error;
        error << "Device at id: " << deviceID << " is not present.";
        throw std::invalid_argument(error.str());
    }

    SetProcessingImageDimensions(DEFAULT_IMAGE_WIDTH, DEFAULT_IMAGE_HEIGHT);

    try {
    faceDetector.load(classifier_xml_filename);
    }catch (...) { }
    if(faceDetector.empty())
        throw std::runtime_error("Unable to load classifier xml file.");
}

QRect FaceTracker::findClosest(const std::vector<cv::Rect> &rects, QPoint point)
{
    cv::Rect bestMatch(0,0,0,0);
    float bestDistanceSquared = std::numeric_limits<float>::max();
    for(std::vector<cv::Rect>::const_iterator itr = rects.begin();
        itr != rects.end(); ++itr)
    {
        float dX = fabs((float)((itr->x + (itr->width/2)) - point.x()));
        float dY = fabs((float)((itr->y + (itr->height/2)) - point.y()));

        float distSq = dX*dX + dY*dY;

        if(distSq < bestDistanceSquared)
        {
            bestMatch = *itr;
            bestDistanceSquared = distSq;
        }
    }

    return QRect(bestMatch.x, bestMatch.y, bestMatch.width, bestMatch.height);
}
