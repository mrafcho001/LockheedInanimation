#include "serial.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <QDebug>
#include <stdio.h>

const std::string Serial::DefaultTTYDevice = "/dev/ttyACM0";

Serial::Serial() :
    m_tty(Serial::DefaultTTYDevice)
{
}

Serial::Serial(const std::string ttyDevice) :
    m_tty(ttyDevice)
{
}

bool Serial::is_open() const
{
    return ((fcntl(m_fd, F_GETFL) != -1) || (errno != EBADF));
}

bool Serial::open()
{
    if(is_open())
    {
        if(::close(m_fd) == false)
            return false;
    }

    m_fd = ::open(m_tty.c_str(), O_RDWR | O_NOCTTY);

    return (m_fd != -1);
}

bool Serial::open(const std::string &ttyDevice)
{
    m_tty = ttyDevice;
    return this->open();
}

bool Serial::close()
{
    if(is_open())
        return ::close(m_fd) != -1;

    return true;
}

int Serial::error() const
{
    return errno;
}

int Serial::readBytes(char *buffer, int nbytes) const
{
    return read(m_fd, buffer, nbytes);
}

bool Serial::readInt(int &data) const
{
    data = 0;
    quint8 byte;
    for(unsigned int i = 0; i < sizeof(int); i++)
    {
        if(read(m_fd, (void*)&byte, sizeof(quint8)) != sizeof(int))
            return false;
        data |= ((unsigned int)byte)<<(i*8);
    }
    return true;
}

bool Serial::readChar(char &data) const
{
    return read(m_fd, (void*)&data, sizeof(char)) == sizeof(char);
}

bool Serial::readByte(quint8 &data) const
{
    return read(m_fd, (void*)&data, sizeof(quint8)) == sizeof(quint8);
}

int Serial::writeBytes(const char *buffer, int nbytes)
{
    return write(m_fd, (void*)buffer, nbytes);
}

bool Serial::writeInt(const int integer)
{
    unsigned int val = integer;
    bool retValue = true;
    for(unsigned int i = 0; i < sizeof(int); i++)
    {
        quint8 byte = val & 0xFF;
        retValue &= write(m_fd, (void*)&byte, sizeof(quint8)) == sizeof(quint8);
        val = val >> 8;
    }

    return retValue;
}

bool Serial::writeChar(const char character)
{
    return write(m_fd, (void*)&character, sizeof(char)) == sizeof(char);
}

bool Serial::writeByte(const quint8 &data) const
{
    return write(m_fd, (void*)&data, sizeof(quint8)) == sizeof(quint8);
}

Serial &Serial::operator <<(const int &integer)
{
    m_failbit = !this->writeInt(integer);
    return *this;
}

Serial &Serial::operator <<(const char &character)
{
    m_failbit = !this->writeChar(character);
    return *this;
}

Serial &Serial::operator <<(const quint16 &data)
{
    m_failbit = !this->writeByte((quint8)(data&0xFF));
    m_failbit |= !this->writeByte((quint8)(data>>8));

    return *this;
}

Serial &Serial::operator <<(const quint8 &data)
{
    m_failbit = !this->writeByte(data);
    return *this;
}

Serial &Serial::operator >>(int &integer)
{
    m_failbit = !this->readInt(integer);
    return *this;
}

Serial &Serial::operator >>(char &character)
{
    m_failbit = !this->readChar(character);
    return *this;
}

Serial &Serial::operator >>(quint16 &data)
{
    data = 0;
    quint8 lsb, msb;
    m_failbit = !this->readByte(lsb);
    m_failbit |= !this->readByte(msb);
    data = (quint16)lsb| (((quint16)msb) << 8);

    return *this;
}

Serial &Serial::operator >>(quint8 &data)
{
    m_failbit = !this->readByte(data);
    return *this;
}

Serial::operator bool()
{
    return !m_failbit;
}



