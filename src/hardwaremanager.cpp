#include "hardwaremanager.h"
#include <QThread>
#if defined(DEBUG_UNHANDLED_MESSAGES) || defined(DEBUG_QTHREADS)
#include <QDebug>
#endif

HardwareManager::HardwareManager(QObject *parent) :
    QObject(parent)
{
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
            this, SLOT(processAsyncEvent(Message)));
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
    else if(msg.paramCount() == 1)
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
    QObject(parent), m_ceaseRequested(false), m_serial(new Serial)
{
}

bool ThreadSafeAsyncSerial::sendMessage(const HardwareComm::Message &msg, HardwareComm::Message &response)
{
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
            Sender *sender = m_senderQueue.dequeue();
            sender->msg = readMsg;
            m_queueMutex.unlock();
            sender->cond.wakeOne();
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
