#include "hardwaremanager.h"
#include <QThread>
#include <QTimer>
#if defined(DEBUG_UNHANDLED_MESSAGES) || defined(DEBUG_QTHREADS)
#include <QDebug>
#endif

HardwareManager::HardwareManager(QObject *parent) :
    QObject(parent), m_comm(new HardwareComm(this))
{
    connect(m_comm, SIGNAL(modeSwitchTriggered()), this, SIGNAL(ModeSwitchTriggered()));
    connect(m_comm, SIGNAL(horizontalPositionChanged(qreal)), this, SLOT(m_updateHPosition(qreal)));
    connect(m_comm, SIGNAL(verticalPositionChanged(qreal)), this, SLOT(m_updateVPosition(qreal)));
    connect(m_comm, SIGNAL(finalHorizontalPositionReached(qreal)), this, SLOT(positionHReached(qreal)));
    connect(m_comm, SIGNAL(finalVerticalPositionReached(qreal)), this, SLOT(positionVReached(qreal)));

    m_monitorH_ROM = HardwareManager::DefaultHorizontalROM;
    m_monitorV_ROM = HardwareManager::DefaultVerticalROM;
    m_cameraH_FOV = HardwareManager::DefaultCameraHFOV;
    m_cameraV_FOV = HardwareManager::DefaultCameraVFOV;
    m_toleranceH = HardwareManager::DefaultHTolerance;
    m_toleranceV = HardwareManager::DefaultVTolerance;
}

bool HardwareManager::SetManualMode(bool manual_mode)
{
    return m_comm->enableManualControls(manual_mode);
}

void HardwareManager::UpdateFacePosition(QRect normalized_face_pos)
{
    if(!m_comm->isReady())
        return;

    quint8 hpos = m_comm->retrievePositionH();
    quint8 vpos = m_comm->retrievePositionV();
    emit PositionHUpdate(hpos);
    emit PositionVUpdate(vpos);

    QPointF center = normalized_face_pos.center();
    qDebug() << "Center: " << center;
    if(center.x() < 5 && center.y() < 5)
        return;
    center /= 100;
    //Determine if monitor position needs to be adjusted
    if(qAbs(center.x() - 0.5)*m_cameraH_FOV > m_toleranceH )//&& !m_hMotion)
    {
        //Adjust H position
        m_hMotion = true;
        qreal hdeg = (center.x() - 0.5)*m_cameraH_FOV;

        int newPosition = hpos + hdeg/m_monitorH_ROM*255;
        if(newPosition > 255)
            newPosition = 255;
        else if(newPosition < 0)
            newPosition = 0;

        m_hMotion = m_comm->setHorizontalPosition(newPosition);
        emit RequestingHPosition((quint8)newPosition);
    }
    if(qAbs(center.y() - 0.5)*m_cameraV_FOV > m_toleranceV)// && !m_hMotion)
    {
        //Adjust V position
        m_vMotion = true;
        qreal vdeg = (center.y() - 0.5)*m_cameraV_FOV;

        int newPosition = vpos + vdeg/m_monitorV_ROM*255;
        if(newPosition > 255)
            newPosition = 255;
        else if(newPosition < 0)
            newPosition = 0;

        m_vMotion = m_comm->setHorizontalPosition(newPosition);
        emit RequestingVPosition((quint8)newPosition);
    }
}

void HardwareManager::SetCameraHFOV(qreal fov)
{
    m_cameraH_FOV = fov;
}

void HardwareManager::SetCameraVFOV(qreal fov)
{
    m_cameraV_FOV = fov;
}

void HardwareManager::SetCameraHTolerance(qreal tolerance)
{
    m_toleranceH = tolerance;
}

void HardwareManager::SetCameraVTolerance(qreal tolerance)
{
    m_toleranceV = tolerance;
}

void HardwareManager::SetPanROM(qreal rom)
{
    m_monitorH_ROM = rom;
}

void HardwareManager::SetTiltROM(qreal rom)
{
    m_monitorV_ROM = rom;
}

void HardwareManager::SetSerialPort(std::string &port)
{
    m_comm->setSerialTTY(port);
}

void HardwareManager::positionHReached(qreal pos)
{
    m_hMotion = false;
}

void HardwareManager::positionVReached(qreal pos)
{
    m_vMotion = false;
}

void HardwareManager::m_updateHPosition(qreal pos)
{
    m_posH = pos;
    emit PositionChanged(m_posH, m_posV);
}

void HardwareManager::m_updateVPosition(qreal pos)
{
    m_posV = pos;
    emit PositionChanged(m_posH, m_posV);
}

HardwareComm::HardwareComm(QObject *parent) :
    QObject(parent), m_serialComm(new ThreadSafeAsyncSerial)
{
    m_serialCommThread = new QThread;

    m_serialComm->moveToThread(m_serialCommThread);
    connect(m_serialCommThread, SIGNAL(started()), m_serialComm, SLOT(begin()));
    connect(m_serialComm, SIGNAL(finished()), m_serialCommThread, SLOT(quit()));
    connect(m_serialCommThread, SIGNAL(finished()), m_serialComm, SLOT(deleteLater()));
    connect(m_serialCommThread, SIGNAL(finished()), m_serialCommThread, SLOT(deleteLater()));

    connect(m_serialComm, SIGNAL(AsyncMessage(HardwareComm::Message)),
            this, SLOT(processAsyncEvent(HardwareComm::Message)));
    m_serialCommThread->start();

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(setCommReady()));
    this->setSerialTTY(Serial::DefaultTTYDevice);

    qRegisterMetaType<HardwareComm::Message>("HardwareComm::Message");
}

HardwareComm::~HardwareComm()
{
    m_serialComm->stop();
    m_serialCommThread->quit();
    m_serialCommThread->wait();
}

qreal HardwareComm::positionH() const
{
    return m_hPosition;
}

qreal HardwareComm::positionV() const
{
    return m_vPosition;
}

qreal HardwareComm::retrievePositionH()
{
    if(!m_serialComm->isReady())
        return -1.0;

    Message msg(MESSAGE_POSITION_H_REQUEST);
    Message response;
    if(!m_serialComm->sendMessage(msg, response))
        return -1.0;

    m_hPosition = response.params.one;

    return m_hPosition;
}

qreal HardwareComm::retrievePositionV()
{
    Message msg(MESSAGE_POSITION_V_REQUEST);
    Message response;
    if(!m_serialComm->sendMessage(msg, response))
        return -1.0;

    m_vPosition = response.params.one;

    return m_vPosition;
}

bool HardwareComm::isReady() const
{
    return m_serialComm->isReady();
}

bool HardwareComm::setVerticalPosition(quint8 position)
{
    return m_setPositionHelper(MESSAGE_ADJUST_V_POSITION, position);
}

bool HardwareComm::setHorizontalPosition(quint8 position)
{
    return m_setPositionHelper(MESSAGE_ADJUST_H_POSITION, position);
}

bool HardwareComm::enableManualControls(bool enable)
{
    Message msg;
    if(enable)
        msg.msg = MESSAGE_ENABLE_MANUAL_CONTROLS;
    else
        msg.msg = MESSAGE_DISABLE_MANUAL_CONTROLS;

    Message response;
    return m_serialComm->sendMessage(msg, response);
}

void HardwareComm::processAsyncEvent(HardwareComm::Message msg)
{
    switch(msg.msg)
    {
    case MESSAGE_MODE_SWITCH:
        emit modeSwitchTriggered();
        break;
    case MESSAGE_POSITION_UPDATE:
        emit horizontalPositionChanged(msg.params.one/((qreal)255));
        emit verticalPositionChanged(msg.params.two/((qreal)255));
        break;
    case MESSAGE_POSITION_H_REACHED:
        emit finalHorizontalPositionReached(msg.params.one/((qreal)255));
        break;
    case MESSAGE_POSITION_V_REACHED:
        emit finalVerticalPositionReached(msg.params.one/((qreal)255));
        break;
    case MESSAGE_POSITION_REACHED:
        emit finalHorizontalPositionReached(msg.params.one/((qreal)255));
        emit finalVerticalPositionReached(msg.params.two/((qreal)255));
        break;

#ifdef DEBUG_UNHANDLED_MESSAGES
    default:
        qDebug() << "Unhandled message: " << msg.msg;
        break;
#endif
    }
}

void HardwareComm::setSerialTTY(const std::string &tty)
{
    m_serialComm->openSerialTTY(tty);
    m_timer->setSingleShot(true);
    m_timer->start(4000);
}

void HardwareComm::setCommReady()
{
    if(!m_serialComm->isReady())
    {
        m_serialComm->setReady();
        emit CommReady();
    }
}

bool HardwareComm::m_setPositionHelper(quint16 msg, quint8 position)
{
    if(!m_serialComm->isReady())
        return false;

    Message sendMsg;
    sendMsg.msg = msg;
    sendMsg.params.one = position ;

    Message response;
    return m_serialComm->sendMessage(sendMsg, response);
}

HardwareComm::Message::Message(quint16 msg, quint8 param1, quint8 param2)
{
    this->msg = msg;
    params.one = param1;
    params.two = param2;
}

HardwareComm::Message::Message(const HardwareComm::Message &msg)
{
    this->msg = msg.msg;
    this->params.param_mem = msg.params.param_mem;
}

int HardwareComm::Message::paramCount() const
{
    return GET_MSG_PARAM_COUNT(this->msg);
}

bool HardwareComm::Message::isAsync() const
{
    return this->msg & MESSAGE_TYPE_ASYNC;
}

QString printMsg(HardwareComm::Message msg)
{
    switch(msg.msg)
    {
    case MESSAGE_ECHO_REQUEST:
        return QString("MESSAGE_ECHO_REQUEST");
    case MESSAGE_ECHO_RESPONSE:
        return QString("MESSAGE_ECHO_RESPONSE");
    case MESSAGE_ACK:
        return QString("MESSAGE_ACK");
    case MESSAGE_NACK:
        return QString("MESSAGE_NACK");
    case MESSAGE_ENABLE_MANUAL_CONTROLS:
        return QString("MESSAGE_ENABLE_MANUAL_CONTROLS");
    case MESSAGE_DISABLE_MANUAL_CONTROLS:
        return QString("MESSAGE_DISABLE_MANUAL_CONTROLS");
    case MESSAGE_ADJUST_H_POSITION:
        return QString("MESSAGE_ADJUST_H_POSITION (%1)").arg(msg.params.one);
    case MESSAGE_ADJUST_V_POSITION:
        return QString("MESSAGE_ADJUST_V_POSITION (%1)").arg(msg.params.one);
    case MESSAGE_POSITION_REQUEST:
        return QString("MESSAGE_POSITION_REQUEST");
    case MESSAGE_POSITION_RESPONSE:
        return QString("MESSAGE_POSITION_RESPONSE (%1,%2)").arg(msg.params.one).arg(msg.params.two);
    case MESSAGE_POSITION_H_REQUEST:
        return QString("MESSAGE_POSITION_H_REQUEST");
    case MESSAGE_POSITION_V_REQUEST:
        return QString("MESSAGE_POSITION_V_REQUEST");
    case MESSAGE_POSITION_V_RESPONSE:
        return QString("MESSAGE_POSITION_V_RESPONSE (%1)").arg(msg.params.one);
    case MESSAGE_POSITION_H_RESPONSE:
        return QString("MESSAGE_POSITION_H_RESPONSE (%1)").arg(msg.params.one);
    case MESSAGE_POSITION_UPDATE:
        return QString("MESSAGE_POSITION_UPDATE (%1,%2)").arg(msg.params.one).arg(msg.params.two);
    case MESSAGE_POSITION_REACHED:
        return QString("MESSAGE_POSITION_REACHED (%1,%2)").arg(msg.params.one).arg(msg.params.two);
    case MESSAGE_POSITION_H_REACHED:
        return QString("MESSAGE_POSITION_H_REACHED (%1)").arg(msg.params.one);
    case MESSAGE_POSITION_V_REACHED:
        return QString("MESSAGE_POSITION_V_REACHED (%1)").arg(msg.params.one);
    case MESSAGE_MODE_SWITCH:
        return QString("MESSAGE_MODE_SWITCH");
    }
    return QString("unrecognized message! %1").arg(msg.msg);
}

Serial& operator<<(Serial &serial, const HardwareComm::Message &msg)
{
    serial << msg.msg;
    if(msg.paramCount() == 1)
        serial << msg.params.one;
    else if(msg.paramCount() == 2)
        serial << msg.params.one << msg.params.two;

    return serial;
}

Serial& operator>>(Serial &serial, HardwareComm::Message &msg)
{
    serial >> msg.msg;
    if(msg.paramCount() == 1)
        serial >> msg.params.one;
    else if(msg.paramCount() == 2)
        serial >> msg.params.one >> msg.params.two;

    return serial;
}

HardwareComm::Message &HardwareComm::Message::operator =(const HardwareComm::Message &source)
{
    this->msg = source.msg;
    this->params.param_mem = source.params.param_mem;

    return *this;
}


ThreadSafeAsyncSerial::ThreadSafeAsyncSerial(QObject *parent):
    QObject(parent), m_ceaseRequested(false), m_serial(new Serial), m_isReady(false)
{
    m_readyMutex.tryLock();
}

bool ThreadSafeAsyncSerial::sendMessage(const HardwareComm::Message &msg, HardwareComm::Message &response)
{
    QMutexLocker locker(&m_queueMutex);
    if(!m_isReady)
        return false;
#ifdef DEBUG_SERIAL_COMM
    qDebug() << "ThreadSafeAsyncSerial::sendMessage(): Sending: " << printMsg(msg);
#endif
    Sender sender;

    m_senderQueue.enqueue(&sender);
    *m_serial << msg;
    sender.cond.wait(&m_queueMutex);

#ifdef DEBUG_SERIAL_COMM
    qDebug() << "ThreadSafeAsyncSerial::sendMessage(): response: " << printMsg(sender.response);
#endif

    response = sender.response;
    return true;
}

bool ThreadSafeAsyncSerial::isReady() const
{
    return m_isReady;
}


void ThreadSafeAsyncSerial::begin()
{
#ifdef DEBUG_QTHREADS
    qDebug() << "begin(): called";
#endif
    HardwareComm::Message readMsg;
    while(!m_ceaseRequested)
    {
        if(!m_isReady)
        {
#ifdef DEBUG_QTHREADS
            qDebug() << "begin(): not ready, blocking on mutex...";
#endif
            m_readyMutex.lock();
            m_readyMutex.unlock();
            if(m_ceaseRequested)
                break;
        }
        if(!((*m_serial) >> readMsg))
            continue;

        if(readMsg.isAsync())
        {
#ifdef DEBUG_SERIAL_COMM
            qDebug() << "Async: " << printMsg(readMsg);
#endif
            emit AsyncMessage(readMsg);
        }
        else
        {
            //Check queue and return to proper receiver
            m_queueMutex.lock();
            if(!m_senderQueue.empty())
            {
                Sender *sender = m_senderQueue.dequeue();
                sender->response = readMsg;
                m_queueMutex.unlock();
                sender->cond.wakeOne();
            }
            else
            {
                qDebug() << "Non-async and non-response msg: " << printMsg(readMsg);
                m_queueMutex.unlock();
            }
        }
    }
#ifdef DEBUG_QTHREADS
    qDebug() << "ThreadSafeAsyncSerial::begin(): loop complete.";
#endif
    emit finished();
}

void ThreadSafeAsyncSerial::stop()
{
#ifdef DEBUG_QTHREADS
    qDebug() << "ThreadSafeAsyncSerial::stop(): attempting to stop read loop";
#endif
    m_ceaseRequested = true;
    if(!m_isReady)
        m_readyMutex.unlock();
    *m_serial << HardwareComm::Message();
}

bool ThreadSafeAsyncSerial::openSerialTTY(const std::string &tty)
{
    this->m_readyMutex.tryLock();
    m_isReady = false;
    m_serial->open(tty);
    return m_serial->is_open();
}


void ThreadSafeAsyncSerial::setReady()
{
#ifdef DEBUG_QTHREADS
    qDebug() << "ThreadSafeAsyncSerial::setReady(): ublocking read loop";
#endif

    this->m_isReady = true;
    this->m_readyMutex.unlock();
}
