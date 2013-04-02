#ifndef SERIAL_H
#define SERIAL_H

#include <QtGlobal>
#include <string>


class Serial
{
public:
    Serial();
    Serial(const std::string ttyDevice);

    bool is_open() const;
    bool open();
    bool open(std::string ttyDevice);
    bool close();
    int error() const;

    int readBytes(char* buffer, int nbytes) const;
    bool readInt(int &data) const;
    bool readChar(char &data) const;
    bool readByte(quint8 &data) const;

    int writeBytes(const char* buffer, int nbytes);
    bool writeInt(const int integer);
    bool writeChar(const char character);
    bool writeByte(const quint8 &data) const;


    //some overloads for ease of use
    Serial& operator<< (const int &integer);
    Serial& operator<< (const char &character);
    Serial& operator<< (const quint16 &data);
    Serial& operator<< (const quint8 &data);

    Serial& operator>> (int &integer);
    Serial& operator>> (char &character);
    Serial& operator>> (quint16 &data);
    Serial& operator>> (quint8 &data);

    //allows usage: if(Serial >> int)
    operator bool();

private:
    std::string m_tty;
    int m_fd;
    bool m_failbit;

public:
    const static std::string DefaultTTYDevice;
};

#endif // SERIAL_H
