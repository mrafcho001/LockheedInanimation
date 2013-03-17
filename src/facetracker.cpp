#include "facetracker.h"
#include <stdexcept>
#include <sstream>
#include <limits>
#include <cmath>
#include <QFile>
#include <QTemporaryFile>
#include <QDebug>
#include <QResource>
#include <QElapsedTimer>

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

QRect FaceTracker::GetFacePosition(bool normalized)
{
    cv::Mat cameraFrame;
    GetProcessReadyWebcamImage(cameraFrame);
    if(cameraFrame.empty())
        return InvalidQRect;

    std::vector<cv::Rect> faceRects;
    m_faceDetector.detectMultiScale(cameraFrame, faceRects,
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

    if(normalized)
    {
        return QRect(100*m_lastPosition.x()/m_imageWidth,
                     100*m_lastPosition.y()/m_imageHeight,
                     100*m_lastPosition.width()/m_imageWidth,
                     100*m_lastPosition.height()/m_imageHeight);
    }

    return m_lastPosition;
}

QList<QRect> FaceTracker::GetAllFacesPositions(bool normalized)
{
    cv::Mat cameraFrame;
    GetProcessReadyWebcamImage(cameraFrame);
    if(cameraFrame.empty())
        return QList<QRect>();

    std::vector<cv::Rect> faceRects;
    m_faceDetector.detectMultiScale(cameraFrame, faceRects,
                                  m_searchScaleFactor, m_minNeighbors,
                                  cv::CASCADE_SCALE_IMAGE | m_additionalFlags);
    QList<QRect> qFaceRects;

    if(normalized)
    {
        for(std::vector<cv::Rect>::const_iterator itr = faceRects.begin();
            itr != faceRects.end(); ++itr)
            qFaceRects.append(QRect(itr->x, itr->y, itr->width, itr->height));
    }
    else
    {
        for(std::vector<cv::Rect>::const_iterator itr = faceRects.begin();
            itr != faceRects.end(); ++itr)
            qFaceRects.append(QRect(100*(itr->x)/m_imageWidth,
                                    100*(itr->y)/m_imageHeight,
                                    100*(itr->width)/m_imageWidth,
                                    100*(itr->height)/m_imageHeight));
    }

    return qFaceRects;
}

QRect FaceTracker::GetBestFacePosition(bool normalized)
{
    cv::Mat cameraFrame;
    GetProcessReadyWebcamImage(cameraFrame);
    if(cameraFrame.empty())
        return InvalidQRect;

    std::vector<cv::Rect> faceRects;
    m_faceDetector.detectMultiScale(cameraFrame, faceRects,
                                  m_searchScaleFactor, m_minNeighbors,
                                  cv::CASCADE_FIND_BIGGEST_OBJECT | cv::CASCADE_DO_ROUGH_SEARCH | m_additionalFlags);

    //No Faces detected
    if(faceRects.size() == 0)
        return InvalidQRect;

    if(normalized)
        m_lastPosition.setRect(faceRects[0].x, faceRects[0].y,
                               faceRects[0].width, faceRects[0].height);
    else
        m_lastPosition.setRect(100*faceRects[0].x/m_imageWidth,
                               100*faceRects[0].y/m_imageHeight,
                               100*faceRects[0].width/m_imageWidth,
                               100*faceRects[0].height/m_imageHeight);

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
    //! \todo This will corrupt image if function called multiple times, or FaceTracker::GetFaceImage gets called
    cv::cvtColor(m_cameraFrame, m_cameraFrame, CV_BGR2RGB);

    return new QImage(m_cameraFrame.data, m_cameraFrame.cols,
                      m_cameraFrame.rows, QImage::Format_RGB888);
}

QImage *FaceTracker::GetFaceImage()
{
    //! \todo This is very expensive and inefficient way of performing this action
    QImage *image = GetLastImage();
    QImage *result = new QImage(image->copy(m_lastPosition));
    delete image;
    return result;
}


void FaceTracker::Init(int deviceID)
{
    m_minFeatureSize = cv::Size(DEFAULT_MIN_FEATURE_SIZE,DEFAULT_MIN_FEATURE_SIZE);
    m_searchScaleFactor = DEFAULT_SEARCH_SCALE_FACTOR;
    m_minNeighbors = DEFAULT_MIN_NEIGHBORS_CUTOFF;
    m_additionalFlags = DEFAULT_ADDITIONAL_FLAGS;
    m_classifierXmlFilename = DEFAULT_CLASSIFIER_XML_FILENAME;

    m_vc.open(deviceID);
    if(!m_vc.isOpened())
    {
        std::ostringstream error;
        error << "Device at id: " << deviceID << " is not present.";
        throw std::invalid_argument(error.str());
    }

    //m_vc.set(CV_CAP_PROP_AUTO_EXPOSURE, 0.0);

    qDebug() << "CV_VERSION:" << CV_VERSION;

    SetProcessingImageDimensions(DEFAULT_IMAGE_WIDTH, DEFAULT_IMAGE_HEIGHT);

    QResource resource(QString(m_classifierXmlFilename.c_str()));
    if(resource.isValid())
    {
        QFile resFile(QString(m_classifierXmlFilename.c_str()));
        QTemporaryFile *tempFile = QTemporaryFile::createLocalFile(resFile);

        LoadCascadeClassifier(tempFile->fileName().toStdString());

        tempFile->close();
        delete tempFile;
    }
    else
        LoadCascadeClassifier(m_classifierXmlFilename);
}

void FaceTracker::LoadCascadeClassifier(const std::string filename)
{
    try
    {
        m_faceDetector.load(filename);
    }
    catch (...) { }

    if(m_faceDetector.empty())
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

void FaceTracker::GetProcessReadyWebcamImage(cv::Mat &cameraFrame)
{
    QElapsedTimer timer;
    timer.start();
    m_vc >> m_cameraFrame;

    qint64 s1 = timer.elapsed();

    if(m_cameraFrame.empty())
    {
        cameraFrame = m_cameraFrame;
        return;
    }

    //cv::resize(m_cameraFrame, cameraFrame,cv::Size(m_imageWidth,m_imageHeight));

    //To Grayscale
    if(m_cameraFrame.channels() == 3)
        cv::cvtColor(m_cameraFrame, cameraFrame, CV_BGR2GRAY);
    else if(m_cameraFrame.channels() == 4)
        cv::cvtColor(m_cameraFrame, cameraFrame, CV_BGRA2GRAY);

    //Histogram Equalization
    cv::equalizeHist(cameraFrame, cameraFrame);
    qint64 s2 = timer.elapsed();

    qDebug() << "s1: " << s1 << "\ts2: " << s2;
}
