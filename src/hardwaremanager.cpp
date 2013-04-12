#include "hardwaremanager.h"
#include <QThread>
#if defined(DEBUG_UNHANDLED_MESSAGES) || defined(DEBUG_QTHREADS)
#include <QDebug>
#endif

HardwareManager::HardwareManager(QObject *parent) :
    QObject(parent), m_comm(new HardwareComm(this))
{
    connect(m_comm, SIGNAL(modeSwitchTriggered()), this, SIGNAL(ModeSwitchTriggered()));
    connect(m_comm, SIGNAL(horizontalPositionChanged(qreal)), this, SLOT(m_updateHPosition(qreal)));
    connect(m_comm, SIGNAL(verticalPositionChanged(qreal)), this, SLOT(m_updateVPosition(qreal)));

    m_monitorH_ROM = HardwareManager::DefaultHorizontalROM;
    m_monitorV_ROM = HardwareManager::DefaultVerticalROM;
    m_cameraH_FOV = HardwareManager::DefaultCameraHFOV;
    m_cameraV_FOV = HardwareManager::DefaultCameraVFOV;
    m_toleranceH = HardwareManager::DefaultHTolerance;
    m_toleranceV = HardwareManager::DefaultVTolerance;
}

bool HardwareManager::SetManualMode(bool manual_mode)
{
    return m_comm->enableManualControls(!manual_mode);
}

void HardwareManager::UpdateFacePosition(QRect normalized_face_pos)
{
    QPointF center = normalized_face_pos.center();
    //Determine if monitor position needs to be adjusted
    if(qAbs(center.x() - 0.5)*m_cameraH_FOV > m_toleranceH && !m_hMotion)
    {
        //Adjust H position
        m_hMotion = true;
        qreal newPosition = m_posH + (center.x() - 0.5)*(m_cameraH_FOV/m_monitorH_ROM);

        m_comm->setHorizontalPosition(newPosition);
    }
    if(qAbs(center.y() - 0.5)*m_cameraV_FOV > m_toleranceV && !m_hMotion)
    {
        //Adjust V position
        m_vMotion = true;
        qreal newPosition = m_posV + (center.y() - 0.5)*(m_cameraV_FOV/m_monitorV_ROM);

        m_comm->setHorizontalPosition(newPosition);
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
    QThread *thread = new QThread;

    m_serialComm->moveToThread(thread);
    connect(thread, SIGNAL(started()), m_serialComm, SLOT(begin()));
    connect(m_serialComm, SIGNAL(finished()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), m_serialComm, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    connect(m_serialComm, SIGNAL(AsyncMessage(HardwareComm::Message)),
            this, SLOT(processAsyncEvent(HardwareComm::Message)));
    thread->start();

    qRegisterMetaType<HardwareComm::Message>("HardwareComm::Message");
}

HardwareComm::~HardwareComm()
{
    m_serialComm->stop();
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
    Message msg(MESSAGE_POSITION_H_REQUEST);
    Message response;
    m_serialComm->sendMessage(msg, response);
    m_hPosition = response.params.one;

    return m_hPosition;
}

qreal HardwareComm::retrievePositionV()
{
    Message msg(MESSAGE_POSITION_V_REQUEST);
    Message response;
    m_serialComm->sendMessage(msg, response);
    m_vPosition = response.params.one;

    return m_vPosition;
}

bool HardwareComm::setVerticalPosition(qreal position)
{
    Message msg(MESSAGE_ADJUST_V_POSITION);
    return m_setPositionHelper(msg, position);
}

bool HardwareComm::setHorizontalPosition(qreal position)
{
    Message msg(MESSAGE_ADJUST_H_POSITION);
    return m_setPositionHelper(msg, position);
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

void HardwareComm::setSerialTTY(std::string &tty)
{
    m_serialComm->setSerialTTY(tty);
}

bool HardwareComm::m_setPositionHelper(Message &msg, qreal position)
{
    if(position > 1.0f)
        msg.params.one = 255;
    else if(position < 0.0f)
        msg.params.one = 0;
    else
        msg.params.one = (quint8)(position * 255);

    Message response;
    return m_serialComm->sendMessage(msg, response);
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

QString msgToString(quint16 msg)
{
    switch(msg)
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
        return QString("MESSAGE_ADJUST_H_POSITION");
    case MESSAGE_ADJUST_V_POSITION:
        return QString("MESSAGE_ADJUST_V_POSITION");
    case MESSAGE_POSITION_REQUEST:
        return QString("MESSAGE_POSITION_REQUEST");
    case MESSAGE_POSITION_RESPONSE:
        return QString("MESSAGE_POSITION_RESPONSE");
    case MESSAGE_POSITION_H_REQUEST:
        return QString("MESSAGE_POSITION_H_REQUEST");
    case MESSAGE_POSITION_V_REQUEST:
        return QString("MESSAGE_POSITION_V_REQUEST");
    case MESSAGE_POSITION_V_RESPONSE:
        return QString("MESSAGE_POSITION_V_RESPONSE");
    case MESSAGE_POSITION_H_RESPONSE:
        return QString("MESSAGE_POSITION_H_RESPONSE");
    case MESSAGE_POSITION_UPDATE:
        return QString("MESSAGE_POSITION_UPDATE");
    case MESSAGE_POSITION_REACHED:
        return QString("MESSAGE_POSITION_REACHED");
    case MESSAGE_POSITION_H_REACHED:
        return QString("MESSAGE_POSITION_H_REACHED");
    case MESSAGE_POSITION_V_REACHED:
        return QString("MESSAGE_POSITION_V_REACHED");
    }
    return QString("unrecognized message! %1").arg(msg);
}

Serial& operator<<(Serial &serial, const HardwareComm::Message &msg)
{
    qDebug() << "Writing: " << msgToString(msg.msg);
    serial << msg.msg;
    if(msg.paramCount() == 1)
        serial << msg.params.one;
    else if(msg.paramCount() == 2)
        serial << msg.params.one << msg.params.two;

    return serial;
}

Serial& operator>>(Serial &serial, HardwareComm::Message &msg)
{
    qDebug() << "Reading...";
    serial >> msg.msg;
    if(msg.paramCount() == 1)
        serial >> msg.params.one;
    else if(msg.paramCount() == 2)
        serial >> msg.params.one >> msg.params.two;
    qDebug() << "Read: " << msgToString(msg.msg);

    return serial;
}

HardwareComm::Message &HardwareComm::Message::operator =(const HardwareComm::Message &source)
{
    this->msg = source.msg;
    this->params.param_mem = source.params.param_mem;

    return *this;
}


ThreadSafeAsyncSerial::ThreadSafeAsyncSerial(QObject *parent):
    QObject(parent), m_ceaseRequested(false), m_serial(new Serial)
{
    if(m_serial->is_open())
        qDebug() << "Opened";
    else
        qDebug() << "Failed to open";
}

bool ThreadSafeAsyncSerial::sendMessage(const HardwareComm::Message &msg, HardwareComm::Message &response)
{
    qDebug() << "Sending: " << msg.msg;
    Sender sender;
    m_queueMutex.lock();

    m_senderQueue.enqueue(&sender);
    *m_serial << msg;
    sender.cond.wait(&m_queueMutex);

    m_queueMutex.unlock();

    response = sender.msg;
    if( (sender.msg.msg & MESSAGE_ACK) == MESSAGE_ACK )
        return true;
    //else if( (sender.msg.msg ^ MESSAGE_NACK) == 0 )
    return false;
}


void ThreadSafeAsyncSerial::begin()
{
    qDebug() << "Beginning...";
    HardwareComm::Message readMsg;
    while(!m_ceaseRequested)
    {
        if(!((*m_serial) >> readMsg))
            continue;

        if(readMsg.isAsync())
        {
            emit AsyncMessage(readMsg);
        }
        else
        {
            //Check queue and return to proper receiver
            m_queueMutex.lock();
            if(!m_senderQueue.empty())
            {
                Sender *sender = m_senderQueue.dequeue();
                sender->msg = readMsg;
                m_queueMutex.unlock();
                sender->cond.wakeOne();
            }
            else
                m_queueMutex.unlock();
        }
    }
    emit finished();
}

void ThreadSafeAsyncSerial::stop()
{
#ifdef DEBUG_QTHREADS
    qDebug() << "ThreadSafeAsyncSerial::stop()";
#endif
    m_ceaseRequested = true;
    *m_serial << HardwareComm::Message();
}

void ThreadSafeAsyncSerial::setSerialTTY(std::string &tty)
{
    m_serial->open(tty);
}
