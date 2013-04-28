LockheedInanimation
===================

Capstone Project sponsored by Lockheed Martin at The Pennsylvania State University

Doxygen generated documentation: http://mrafcho001.github.com/LockheedInanimation/


# Current state of the project
Software for the project is entirely functional and nearly complete. However, the processing power required by the cascade classifier in OpenCV (for face tracking) is simply too great for the Raspberry Pi. As such, the results of attempting to execute the project on a Raspberry Pi are less than ideal. 
With the Logitech C525 camera there are issues with capturing images as well. The timout of the v4l drivers is far too small, and as a result no images are capturing. This problem is aleviated by simply increasing the timeout.

# For future developers
The sponsors intend to create the initially proposed self contained unit, and in order for this to be possible, the Raspberry Pi must be replaced with far more capable hardware. The OpenCV cascade classifier tuning parameters where never explored greatly on the Raspberry Pi, and it is possible to find more optimal settings which produce better results, but I am doubtful that those results will be satisfactory.
The software should be able to run on any Linux based system. The important part that keeps this project bound to Linux is the serial communication between the Raspberry Pi and Arduino. To add Windows support to the software, it is only necessary to implement serial communication for the Windows stack.
Alternatively, to implement true cross-platform compatibility, the project should be ported to QT5, as it provides the QtSerialPort - fully cross-platform compatible serial communication module. At the time of the start of this project, QT5 was in alpha release and was not used.

To satisfy the sponsor's second request of being able to operate the hardware using a Windows laptop, only the serial communication module needs to be ported to Windows. Once that is done, a single executable file can be produced which can operate the hardware.

## Dependencies
This project has several dependencies in order to be built:
* OpenCV development headers & libraries
* QT development headers & libraries
* Arduino SDK

On Ubuntu based linux distributions, these requirements can be installed with:
```
$ sudo apt-get install libopencv-dev libopencv-core-dev libopencv-contrib-dev libopencv-calib3d-dev libopencv-features2d-dev libopencv-flann-dev libopencv-highgui-dev libopencv-imgproc-dev libopencv-legacy-dev libopencv-ml-dev libopencv-objdetect-dev libopencv-video-dev qt4-qmake qt4-dev-tools libqt4-dev libqt4-dev-bin libqt4-opengl-dev arduino-core arduino-mk
```

## Compiling
This is a QT project and is compiled like most other qt projects:
```
$ qmake
$ make
```
To build and upload the Arduino sketch onto the Arduino:
```
$ make arduino
$ make arduino_upload
```
The Arduino make file contains the Arduino serial port, by default it uses `/dev/ttyACM0`, if your Arduino is on a different port, please edit the make file located at `./src/Arduino/Makefile`

## Running
Once the software and arduino sketch are compiled and the arduino sketch is uploaded, simply run the compiled executable:
```
$ ./build/release/LockheedInanimation
```
