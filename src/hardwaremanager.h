#ifndef HARDWAREMANAGER_H
#define HARDWAREMANAGER_H

#include <QObject>
#include "CommunicationProtocol.h"
#include "serial.h"
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>


class HardwareComm;

class HardwareManager : public QObject
{
    Q_OBJECT
public:
    explicit HardwareManager(QObject *parent = 0);
    
signals:
    
public slots:
    
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

public slots:
    bool setVerticalPosition(qreal position);
    bool setHorizontalPosition(qreal position);

    bool enableManualControls(bool enable = true);

    void processAsyncEvent(Message msg);

signals:
    void verticalPositionChanged(qreal position);
    void horizontalPositionChanged(qreal position);

    void finalVerticalPositionReached(qreal position);
    void finalHorizontalPositionReached(qreal position);

    void modeSwitchTriggered();

private:
    bool m_setPositionHelper(Message &msg, qreal position);
    qreal m_hPosition;
    qreal m_vPosition;

    ThreadSafeAsyncSerial *m_serialComm;
};

Serial& operator<<(Serial& serial, const HardwareComm::Message &msg);
Serial& operator>>(Serial& serial, HardwareComm::Message &msg);

class ThreadSafeAsyncSerial : public QObject
{
    Q_OBJECT
public:
    explicit ThreadSafeAsyncSerial(QObject *parent = 0);
    bool sendMessage(const HardwareComm::Message &msg, HardwareComm::Message &response);

public slots:
    void begin();
    void stop();

signals:
    void AsyncMessage(HardwareComm::Message msg);
    void finished();

private:
    bool m_ceaseRequested;

    Serial *m_serial;

    struct Sender
    {
        QWaitCondition cond;
        HardwareComm::Message msg;
    };
    QQueue<Sender*> m_senderQueue;
    QMutex m_queueMutex;

};



#endif // HARDWAREMANAGER_H
