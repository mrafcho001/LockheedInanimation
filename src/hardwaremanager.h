#ifndef HARDWAREMANAGER_H
#define HARDWAREMANAGER_H

#include <QObject>
#include "CommunicationProtocol.h"
#include "serial.h"
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QRectF>
#include <QMetaType>
#include <QTimer>


class HardwareComm;

class HardwareManager : public QObject
{
    Q_OBJECT
public:
    explicit HardwareManager(QObject *parent = 0);

signals:
    void PositionChanged(qreal horizontal, qreal vertical);
    void PositionHUpdate(int hpos);
    void PositionVUpdate(int vpos);
    void ModeSwitchTriggered();

    void RequestingHPosition(int h);
    void RequestingVPosition(int v);

public slots:
    bool SetManualMode(bool manual_mode = true);

    void UpdateFacePosition(QRect normalized_face_pos);

    // Parameter setting
    /*! \brief Set the Camera horizontal FOV (Degrees) */
    void SetCameraHFOV(qreal fov);
    /*! \brief Set the Camera vertical FOV (Degrees) */
    void SetCameraVFOV(qreal fov);

    /*! \brief Set the face horizontal movement tolerance (camera Degrees) */
    void SetCameraHTolerance(qreal tolerance);
    /*! \brief Set the vertical face movement tolerance (camera Degrees) */
    void SetCameraVTolerance(qreal tolerance);

    /*! \brief Set the panning range of motion of the monitor (Degrees) */
    void SetPanROM(qreal rom);
    /*! \brief Set the tilting range of motion of the monitor (Degrees) */
    void SetTiltROM(qreal rom);

    /*! \brief Set serial port to use for Arduino communication */
    void SetSerialPort(std::string &port);

    void positionHReached(qreal pos);
    void positionVReached(qreal pos);

private slots:
    void m_updateHPosition(qreal pos);
    void m_updateVPosition(qreal pos);

private:
    HardwareComm *m_comm;

    qreal m_posH;   //!< Current horizontal monitor position (0.0 .. 1.0)
    qreal m_posV;   //!< Current vertical monitor position (0.0 .. 1.0)
    qreal m_monitorH_ROM; //!< Range of Motion for the monitor in the horizontal axis (in degrees)
    qreal m_monitorV_ROM; //!< Range of Motion for the monitor in the vertical axis (in degrees)


    qreal m_cameraH_FOV;//!< The horizontal field of view of the camera (in degrees)
    qreal m_cameraV_FOV;//!< The vertical field of view of the camera (in degrees)
    qreal m_toleranceH; //!< Tolerance for face movement before triggering monitor movement in the horizontal direction (in degrees)
    qreal m_toleranceV; //!< Tolerance for face movement before triggering monitor movement in the vertical direction (in degrees)

    bool m_hMotion;     //!< Indicates current motion
    bool m_vMotion;

public:
    static const qreal DefaultHorizontalROM = 30.0;
    static const qreal DefaultVerticalROM = 25.0;
    static const qreal DefaultCameraHFOV = 50.0;
    static const qreal DefaultCameraVFOV = 36.0;
    static const qreal DefaultHTolerance = 10.0;
    static const qreal DefaultVTolerance = 6.0;
};

class ThreadSafeAsyncSerial;

class HardwareComm : public QObject
{
    Q_OBJECT
public:
    explicit HardwareComm(QObject *parent = 0);
    ~HardwareComm();

    class Message
    {
    public:
        explicit Message(quint16 msg = MESSAGE_ECHO_REQUEST, quint8 param1 = 0, quint8 param2 = 0);
        Message(const Message & msg);
        int paramCount() const;
        bool isAsync() const;
        Message& operator=(const Message& source);

        quint16 msg;
        union {
            qint16 param_mem;
            struct
            {
                quint8 one;
                quint8 two;
            };
        }params;
    };
    qreal positionH() const;
    qreal positionV() const;

    qreal retrievePositionH();
    qreal retrievePositionV();

    bool isReady() const;

public slots:
    bool setVerticalPosition(quint8 position);
    bool setHorizontalPosition(quint8 position);

    bool enableManualControls(bool enable = true);

    void processAsyncEvent(HardwareComm::Message msg);
    void setSerialTTY(const std::string &tty);
    void setCommReady();

signals:
    void verticalPositionChanged(qreal position);
    void horizontalPositionChanged(qreal position);

    void finalVerticalPositionReached(qreal position);
    void finalHorizontalPositionReached(qreal position);

    void modeSwitchTriggered();

    void CommReady();

private:
    bool m_setPositionHelper(quint16 msg, quint8 position);
    qreal m_hPosition;
    qreal m_vPosition;

    ThreadSafeAsyncSerial *m_serialComm;
    QThread *m_serialCommThread;
    QTimer *m_timer;
};

Q_DECLARE_METATYPE(HardwareComm::Message)

Serial& operator<<(Serial& serial, const HardwareComm::Message &msg);
Serial& operator>>(Serial& serial, HardwareComm::Message &msg);

class ThreadSafeAsyncSerial : public QObject
{
    Q_OBJECT
public:
    explicit ThreadSafeAsyncSerial(QObject *parent = 0);
    bool sendMessage(const HardwareComm::Message &msg, HardwareComm::Message &response);
    bool isReady() const;

public slots:
    void begin();
    void stop();

    bool openSerialTTY(const std::string &tty);
    void setReady();

signals:
    void AsyncMessage(HardwareComm::Message msg);
    void finished();


private:
    bool m_ceaseRequested;

    Serial *m_serial;

    struct Sender
    {
        QWaitCondition cond;
        HardwareComm::Message response;
    };
    QQueue<Sender*> m_senderQueue;
    QMutex m_queueMutex;

    bool m_isReady;
    QMutex m_readyMutex;

};



#endif // HARDWAREMANAGER_H
