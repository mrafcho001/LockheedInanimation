#include "../CommunicationProtocol.h"

const int vert_extend = 3;        // pin 3 controls the extention of the vertical actuator
const int vert_retract = 5;       // pin 5 controls the retraction of the vertical actuator
const int horiz_extend = 7;       // pin 7 controls the extention of the horizontal actuator
const int horiz_retract = 9;      // pin 9 controls the retraction of the horizotnal actuator
const int vert_wiper = A0;
const int horiz_wiper = A1;
const int joy_vert = A2;
const int joy_horiz = A3;
const int joy_sel = 2;           // pin 12 reads the push button of the joystick

int mode = 0;                     // 0: Manual, 1: Automatic, 2: FaceInvaders
int selCount = 0;                 // counts number of delays when joy_sel is HIGH
boolean modeChange = false;       // TRUE if mode changes via joy_sel
int select = 0;

int request = 0;                  // 2 bytes Raspberry Pi request

struct Message
{
    unsigned int msg;
    byte param1;
    byte param2;
};

unsigned int requestQueue;
#define JOYSTICK_PRESSED_QUEUE            (1<<0)

Message lastMSG;
bool manualModeEnabled = false;
bool adjustingH = false;
bool adjustingV = false;

const int actuatorVMax = 200;
const int actuatorVMin = 50;
const int actuatorHMax = 200;
const int actuatorHMin = 50;

enum ActuatorDirection { ACTUATOR_FORWARD, ACTUATOR_STOP, ACTUATOR_REVERSE };

//function prototypes

void joystickPressed();
void processQueue(bool onlyOne = false);
void processManualMode();
void setActuatorDirection(int extend, int retract, ActuatorDirection dir);
void setVActuatorDirection(ActuatorDirection dir);
void setHActuatorDirection(ActuatorDirection dir);
byte getVPosition();
byte getHPosition();
bool performSimple(Message &msg);
void sendMessage(Message &msg);
bool getMessage(Message &msg);

void setup()
{

    pinMode(vert_extend, OUTPUT);
    pinMode(vert_retract, OUTPUT);
    pinMode(horiz_extend, OUTPUT);
    pinMode(horiz_retract, OUTPUT);
    pinMode(13, OUTPUT);

    pinMode(joy_vert, INPUT);
    pinMode(joy_horiz, INPUT);
    pinMode(joy_sel, INPUT);
    digitalWrite(joy_sel, HIGH);   // turn on the pull-up resistor for the joy_sel line

    Serial.begin(9600);

    attachInterrupt(0, joystickPressed, FALLING);

}
void loop()
{
    processQueue();
    if(getMessage(lastMSG))
        performSimple(lastMSG);

    if(manualModeEnabled)
        processManualMode();
}

bool getMessage(Message &msg)
{
    if(Serial.available() >= 2)
    {
        byte lsb = Serial.read();
        byte msb = Serial.read();
        msg.msg = word(msb, lsb);
        //msg.msg = Serial.parseInt();
        while(GET_MSG_PARAM_COUNT(msg.msg) > Serial.available())
        {
            //wait extra bytes
        }
        if(GET_MSG_PARAM_COUNT(msg.msg) >= 1)
            msg.param1 = Serial.read();
        if(GET_MSG_PARAM_COUNT(msg.msg) >= 2)
            msg.param2 = Serial.read();
    }
    else
        return false;
    return true;
}
void sendMessage(Message &msg)
{
    Serial.write(lowByte(msg.msg));
    Serial.write(highByte(msg.msg));
    if(GET_MSG_PARAM_COUNT(msg.msg) >= 1)
        Serial.write(msg.param1);
    if(GET_MSG_PARAM_COUNT(msg.msg) >= 2)
        Serial.write(msg.param2);
    Serial.flush();
}

bool performSimple(Message &msg)
{
    Message response;
    switch(msg.msg)
    {
    case MESSAGE_ECHO_REQUEST:
        response.msg = MESSAGE_ECHO_RESPONSE;
        break;
    case MESSAGE_POSITION_H_REQUEST:
        response.msg = MESSAGE_POSITION_H_RESPONSE;
        response.param1 = getHPosition();
        break;
    case MESSAGE_POSITION_V_REQUEST:
        response.msg = MESSAGE_POSITION_V_RESPONSE;
        response.param1 = getVPosition();
        break;
    case MESSAGE_POSITION_REQUEST:
        response.msg = MESSAGE_POSITION_RESPONSE;
        response.param1 = getHPosition();
        response.param2 = getVPosition();
        break;
    case MESSAGE_ENABLE_MANUAL_CONTROLS:
        setVActuatorDirection(ACTUATOR_STOP);
        setHActuatorDirection(ACTUATOR_STOP);
        manualModeEnabled = true;
        response.msg = MESSAGE_ACK;
        digitalWrite(13,HIGH);
        break;
    case MESSAGE_DISABLE_MANUAL_CONTROLS:
        setVActuatorDirection(ACTUATOR_STOP);
        setHActuatorDirection(ACTUATOR_STOP);
        manualModeEnabled = false;
        response.msg = MESSAGE_ACK;
        digitalWrite(13,LOW);
        break;
    case MESSAGE_ADJUST_H_POSITION:
        response.msg = MESSAGE_NACK;

        break;
    case MESSAGE_ADJUST_V_POSITION:
        response.msg = MESSAGE_NACK;

        break;

    default:
        response.msg = MESSAGE_NACK;
        //return false;
    }

    sendMessage(response);
    return true;
}

byte getHPosition()
{
    return map(analogRead(horiz_wiper), 0, 1023, 0, 255);
}

byte getVPosition()
{
    return map(analogRead(vert_wiper), 0, 1023, 0, 255);
}

void setHActuatorDirection(ActuatorDirection dir)
{
    setActuatorDirection(horiz_extend, horiz_retract, dir);
}

void setVActuatorDirection(ActuatorDirection dir)
{
    setActuatorDirection(vert_extend, vert_retract, dir);
}

void setActuatorDirection(int extend, int retract, ActuatorDirection dir)
{
    if(dir == ACTUATOR_STOP)
    {
        digitalWrite(extend, LOW);
        digitalWrite(retract, LOW);
    }
    else if(dir == ACTUATOR_FORWARD)
    {
        digitalWrite(retract, LOW);
        digitalWrite(extend, HIGH);
    }
    else if(dir == ACTUATOR_REVERSE)
    {
        digitalWrite(extend, LOW);
        digitalWrite(retract, HIGH);
    }
}

void processManualMode()
{
    int vertical = analogRead(joy_vert);
    int horizontal = analogRead(joy_horiz);

    if(vertical > 700)
    {
        if(getVPosition() >= actuatorVMax)
            setVActuatorDirection(ACTUATOR_STOP);
        else
            setVActuatorDirection(ACTUATOR_FORWARD);
    }
    else if(vertical < 300)
    {
        if(getVPosition() <= actuatorVMin)
            setVActuatorDirection(ACTUATOR_STOP);
        else
            setVActuatorDirection(ACTUATOR_REVERSE);
    }
    else
        setVActuatorDirection(ACTUATOR_STOP);

    if(horizontal > 700)
    {
        if(getHPosition() >= actuatorHMax)
            setHActuatorDirection(ACTUATOR_STOP);
        else
            setHActuatorDirection(ACTUATOR_FORWARD);
    }
    else if(horizontal < 300)
    {
        if(getHPosition() <= actuatorHMin)
            setHActuatorDirection(ACTUATOR_STOP);
        else
            setHActuatorDirection(ACTUATOR_REVERSE);
    }
    else
        setHActuatorDirection(ACTUATOR_STOP);
}

void processQueue(bool onlyOne)
{
//#error not implemented
    if(requestQueue == 0)
        return;

    if(requestQueue & JOYSTICK_PRESSED_QUEUE)
    {
        Message msg;
        msg.msg = MESSAGE_MODE_SWITCH;
        sendMessage(msg);
        requestQueue = 0;
    }
}

void joystickPressed()
{
    requestQueue |= JOYSTICK_PRESSED_QUEUE;
}
