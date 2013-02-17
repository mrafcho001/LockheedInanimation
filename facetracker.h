/*! \file facetracker.h
    \brief Defines the class for tracking a face as it moves using OpenCV

    FaceTracker uses OpenCV to track a moving face using a webcam.  The
    device ID of the webcam to use can be specified in the constructor of the
    class otherwise the first webcam (device ID: 0) is used.

    In the presence of multiple faces, FaceTracker will attempt to track the
    same face as it moves around the scree. If multiple faces are present a
    face is selected to be tracked as per the FaceTracker::SelectFace2Track.
    \sa FaceTracker::SelectFace2Track

    The class also exposes some parameters for tuning the face detection:
    \sa FaceTracker::m_minFeatureSize
    \sa FaceTracker::m_searchScaleFactor
    \sa FaceTracker::m_minNeighbors
    \sa FaceTracker::m_additionalFlags

*/

#ifndef FACETRACKER_H
#define FACETRACKER_H


#include <opencv2/opencv.hpp>
#include <QRect>
#include <QList>

//Default values for cv::CascadeClassifier::detectMultiScale()
//! \brief Default value for minimum feature size
#define DEFAULT_MIN_FEATURE_SIZE        40

//! \brief Default value for search scale factor
#define DEFAULT_SEARCH_SCALE_FACTOR     1.1f

//! \brief Default value for minimum neighbor cutoff value
#define DEFAULT_MIN_NEIGHBORS_CUTOFF    4

//! \brief Default additional flags
#define DEFAULT_ADDITIONAL_FLAGS        0

//! \brief Default classifier xml filename
#define DEFAULT_CLASSIFIER_XML_FILENAME "lbpcascade_frontalface.xml"

//! \brief Default Image Width
#define DEFAULT_IMAGE_WIDTH             640

//! \brief Default Image Height
#define DEFAULT_IMAGE_HEIGHT            480

/*! \brief Tracks a face as it moves around.
  This class is a wrapper around OpenCV and provides an eassy to use interface
  for tracking a face between frames. The class exposes several face detection tuning
  parameters.
*/
class FaceTracker
{
public:
    /*! \brief Default constructor uses device 0
    */
    FaceTracker();

    /*! \brief Constructor uses specified device ID for VideoCapture
      \param deviceID device to be used for VideoCapture and image acquisition
    */
    FaceTracker(int deviceID);

    /*! \brief Stops tracking of the current face.

      This Tracker will attempt to follow the same face around as it
      moves around the field of view of the camera. This function stops the
      tracking of the current face, the next time position information is
      requested, a new face will be selected to be tracked.
    */
    void ResetTracker();

    /*! \brief Returns the rectangle of the currently tracked face

      If there is no face currently being tracked, or this function is called
      after a call to FaceTracker::ResetTracker, a new face will be selected as
      per FaceTracker::SelectFace2Track.

      \returns Bounding rectangle of the face
    */
    QRect GetFacePosition();

    /*! \brief Returns a list of bounding rectangles of all of the faces

      Bounding rectangles of all faces currently in view are returned.
      \warning No confidence information is supplied with each face, but all
               detected faces meet the FaceTracker::minNeighbors factor
      \returns List of bounding rectangles of all faces
    */
    QList<QRect> GetAllFacesPositions();

    /*! \brief Returns the bounding rectangle of the face with the highest
               confidence factor
      If there are multiple faces, only the bounding rectangle of the face with
      the highest confidence is returned.
      \returns Bounding rectangle of face with highest confidence factor
    */
    QRect GetBestFacePosition();

    /*! \brief Selects which face to track from a list
      Given a list of bounding rectangles, this function select which face will
      be tracked from here on out.
      \param faceRects list of faces to select from
      \returns Bounding rectangle of selected face
    */
    QRect SelectFace2Track(std::vector<cv::Rect> faceRects);



    //Getters and setters for Face Detection tuning parameters
    //! \brief Getter for minimum feature size used for cv::CascadeClassifier::detectMultiScale()
    int GetMinFeatureSize();
    //! \brief Setter for minimum feature size used for cv::CascadeClassifier::detectMultiScale()
    void SetMinFeatureSize(int minFeatureSize);

    //! \brief Getter for search scale factor used for cv::CascadeClassifier::detectMultiScale()
    float GetSearchScaleFactor();
    //! \brief Setter for search scale factor used for cv::CascadeClassifier::detectMultiScale()
    void SetSearchScaleFactor(float searchScaleFactor);

    //! \brief Getter for minimum neighbors cutoff used for cv::CascadeClassifier::detectMultiScale()
    int GetMinNeighbors();
    //! \brief Setter for minimum neighbors cutoff used for cv::CascadeClassifier::detectMultiScale()
    void SetMinNeighbors(int minNeighbors);

    /*! \brief Sets the dimensions for the image to be processed by OpenCV
      This option may effect performance and reliability of the face detection
      algorithms of OpenCV.
    */
    void SetProcessingImageDimensions(int width, int height);

    /*! \brief Getter for flags used for cv::CascadeClassifier::detectMultiScale()
     These flags are appended to the flags already used for the function call.
     For example, FaceTracker::GetBestFacePosition will use
     \code CASCADE_FIND_BIGGEST_OBJECT | CASCADE_DO_ROUGH_SEARCH | m_additionalFlags \endcode
     for the flags parameter of the cv::CascadeClassifier::detectMultiScale()
     function call
    */
    unsigned int GetAdditionalFlags();

    /*! \brief Adds flag to the set of additional flags
      \code m_additionalFlags = m_additionalFlags | flag; \endcode
      \param flag Flag to be added to the set of additional flags
    */
    void SetAdditionalFlag(unsigned int flag);

    /*! \brief Clears the passed flag from the additional flags
      \code m_additionalFlags = m_additionalFlags & (~flag); \endcode
      \param flag Flag to be cleared
    */
    void ClearAdditionalFlag(unsigned int flag);

    /*! \brief Sets the additional flags to flags
      \code m_additionalFlags = flags \endcode
      \warning Currently set additional flags are lost!
      \param flags The new additional flags
    */
    void SetAdditionalFlags(unsigned int flags);

private:
    /*! \brief Initialization function

      Performs the initialization of the class, should only be called
      from constructors
      \param deviceID Device to be used for VideoCapture and image acquisition
    */
    void Init(int deviceID);

    /*! \brief Finds rectangle with center closest to point
      \param rects List of rectangles
      \param point Target point
    */
    QRect findClosest(const std::vector<cv::Rect> &rects, QPoint point);

    cv::VideoCapture m_vc;   //!< Used for acquiring images from camera
    cv::CascadeClassifier faceDetector; //!< Used for face detection
    std::string classifier_xml_filename;//!< The filename of the XML containing the classifier data

    //Face tracking members
    QRect m_lastPosition;   //!< Stores the last bounding rectangle of the tracked face

    //Parameters for tuning face detection
    cv::Size m_minFeatureSize;  //!< Minimum feature size used for cv::CascadeClassifier::detectMultiScale()
    float m_searchScaleFactor;  //!< Search scale factor used for cv::CascadeClassifier::detectMultiScale()
    int m_minNeighbors;         //!< Minimum number of Neighbors to be classified as a face
    unsigned int m_additionalFlags; //!< Additional flags passed to cv::CascadeClassifier::detectMultiScale()
    unsigned int m_imageWidth;      //!< Specifies the width of the image on which face detection is performed
    unsigned int m_imageHeight;     //!< Specifies the height of the image on which face detection is performed


    static const QRect InvalidQRect;    //!< Easy way to create an InvalidRect
};

#endif // FACETRACKER_H
