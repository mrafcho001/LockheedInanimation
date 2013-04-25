#include "../CommunicationProtocol.h"

const int vert_extend = 12;        // pin 3 controls the extention of the vertical actuator
const int vert_retract = 11;       // pin 5 controls the retraction of the vertical actuator
const int horiz_extend = 7;       // pin 7 controls the extention of the horizontal actuator
const int horiz_retract = 9;      // pin 9 controls the retraction of the horizotnal actuator
const int vert_wiper = A1;
const int horiz_wiper = A2;
const int joy_vert = A5;
const int joy_horiz = A4;
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

unsigned int requestQueue = 0;
#define JOYSTICK_PRESSED_QUEUE              (1<<0)
#define POSITION_H_REACHED_QUEUE            (1<<1)
#define POSITION_V_REACHED_QUEUE            (1<<2)

unsigned long lastPress = 0;
int debounceDelay = 150;

Message lastMSG;
bool manualModeEnabled = false;
bool adjustingH = false;
int requestedH = 0;
int requestedV = 0;
bool adjustingV = false;

const int actuatorVMax = 95;
const int actuatorVMin = 22;
const int actuatorHMax = 130;
const int actuatorHMin = 20;

enum ActuatorDirection { ACTUATOR_FORWARD, ACTUATOR_STOP, ACTUATOR_REVERSE };

//function prototypes

void joystickPressed();
void processQueue(bool onlyOne = false);
void processManualMode();
void performAdjust();
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
    digitalWrite(vert_extend, 0);

    pinMode(vert_retract, OUTPUT);
    digitalWrite(vert_retract, 0);

    pinMode(horiz_extend, OUTPUT);
    digitalWrite(horiz_extend, 0);

    pinMode(horiz_retract, OUTPUT);
    digitalWrite(horiz_retract, 0);

    pinMode(joy_vert, INPUT);
    pinMode(joy_horiz, INPUT);
    pinMode(joy_sel, INPUT);
    digitalWrite(joy_sel, HIGH);   // turn on the pull-up resistor for the joy_sel line

    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);
    Serial.begin(9600);

    attachInterrupt(0, joystickPressed, FALLING);
    manualModeEnabled = true;

}
void loop()
{
    processQueue();
    if(getMessage(lastMSG))
        performSimple(lastMSG);

    if(manualModeEnabled)
        processManualMode();
    if(adjustingH || adjustingV)
        performAdjust();
}

bool getMessage(Message &msg)
{
    if(Serial.available() >= 2)
    {
        byte lsb = Serial.read();
        byte msb = Serial.read();
        msg.msg = word(msb, lsb);
        //msg.param1 = Serial.parseInt();
        //Serial.print("Got: "); Serial.println(msg.msg);
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
    //Serial.print("Sending:" ); Serial.println(msg.msg, DEC);
    if(GET_MSG_PARAM_COUNT(msg.msg) >= 1)
        Serial.write(msg.param1);
    if(GET_MSG_PARAM_COUNT(msg.msg) >= 2)
        Serial.write(msg.param2);
    Serial.flush();
}

bool performSimple(Message &msg)
{
    bool performed = true;
    Message response;
    switch(msg.msg)
    {
    case MESSAGE_ECHO_REQUEST:
        response.msg = MESSAGE_ECHO_RESPONSE;
        break;

    case MESSAGE_POSITION_H_REQUEST:
        response.msg = MESSAGE_POSITION_H_RESPONSE;
        response.param1 = map(getHPosition(), actuatorHMin, actuatorHMax, 0, 255);
        break;

    case MESSAGE_POSITION_V_REQUEST:
        response.msg = MESSAGE_POSITION_V_RESPONSE;
        response.param1 = map(getVPosition(), actuatorVMin, actuatorVMax, 0, 255);
        break;

    case MESSAGE_POSITION_REQUEST:
        response.msg = MESSAGE_POSITION_RESPONSE;
        response.param1 = map(getHPosition(), actuatorHMin, actuatorHMax, 0, 255);
        response.param2 = map(getVPosition(), actuatorVMin, actuatorVMax, 0, 255);
        break;

    case MESSAGE_ENABLE_MANUAL_CONTROLS:
        adjustingH = false;
        adjustingV = false;
        setVActuatorDirection(ACTUATOR_STOP);
        setHActuatorDirection(ACTUATOR_STOP);
        manualModeEnabled = true;
        response.msg = MESSAGE_ACK;
        break;

    case MESSAGE_DISABLE_MANUAL_CONTROLS:
        adjustingH = false;
        adjustingV = false;
        setVActuatorDirection(ACTUATOR_STOP);
        setHActuatorDirection(ACTUATOR_STOP);

        manualModeEnabled = false;
        response.msg = MESSAGE_ACK;
        break;

    case MESSAGE_ADJUST_H_POSITION:
        requestedH = msg.param1;
        if(requestedH < actuatorHMin)
            requestedH = actuatorHMin;
        if(requestedH > actuatorHMax)
            requestedH = actuatorHMax;

        adjustingH = true;

        response.msg = MESSAGE_ACK;
        break;

    case MESSAGE_ADJUST_V_POSITION:
        requestedV = msg.param1;
        if(requestedV < actuatorVMin)
            requestedV = actuatorVMin;
        if(requestedV > actuatorVMax)
            requestedV = actuatorVMax;

        adjustingV = true;

        response.msg = MESSAGE_ACK;
        break;

    default:
        response.msg = MESSAGE_NACK;
        performed = false;
        break;
    }

    sendMessage(response);
    return performed;
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

    //Serial.print("v: "); Serial.print(vertical);
    //Serial.print("     h: "); Serial.print(horizontal);
    //Serial.print("     av: "); Serial.print(getVPosition());
    //Serial.print("     ah: "); Serial.println(getHPosition());

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

void performAdjust()
{
    int vPos = getVPosition();
    int hPos = getHPosition();

    if(adjustingH && abs(requestedH - hPos) > 3)
    {
        //adjust h
        if(requestedH < hPos)
            setHActuatorDirection(ACTUATOR_REVERSE);
        else if(requestedH > hPos)
            setHActuatorDirection(ACTUATOR_FORWARD);
        else
            setHActuatorDirection(ACTUATOR_STOP);
    }
    else if(adjustingH)
    {
        adjustingH = false;
        setHActuatorDirection(ACTUATOR_STOP);
        requestQueue |= POSITION_H_REACHED_QUEUE;
    }
    else
        setHActuatorDirection(ACTUATOR_STOP);

    if(adjustingV && abs(requestedV - vPos) > 3)
    {
        //adjust v
        if(requestedV < vPos)
            setVActuatorDirection(ACTUATOR_REVERSE);
        else if(requestedV > vPos)
            setVActuatorDirection(ACTUATOR_FORWARD);
        else
            setVActuatorDirection(ACTUATOR_STOP);
    }
    else if(adjustingV)
    {
        adjustingV = false;
        setVActuatorDirection(ACTUATOR_STOP);
        requestQueue |= POSITION_V_REACHED_QUEUE;
    }
    else
        setVActuatorDirection(ACTUATOR_STOP);
}

void processQueue(bool onlyOne)
{
    if(requestQueue == 0)
        return;

    Message msg;
    if(requestQueue & JOYSTICK_PRESSED_QUEUE)
    {
        msg.msg = MESSAGE_MODE_SWITCH;
        sendMessage(msg);
        requestQueue &= ~JOYSTICK_PRESSED_QUEUE;
        //Serial.println("Shenanigans");
    }
    if(requestQueue & POSITION_H_REACHED_QUEUE)
    {
        msg.msg = MESSAGE_POSITION_H_REACHED;
        msg.param1 = map(getHPosition(), actuatorHMin, actuatorHMax, 0,255);
        sendMessage(msg);
        requestQueue &= ~POSITION_H_REACHED_QUEUE;
    }
    if(requestQueue & POSITION_V_REACHED_QUEUE)
    {
        msg.msg = MESSAGE_POSITION_V_REACHED;
        msg.param1 = map(getVPosition(), actuatorVMin, actuatorVMax, 0,255);
        sendMessage(msg);
        requestQueue &= ~POSITION_V_REACHED_QUEUE;
    }
}

void joystickPressed()
{
    if((millis() - lastPress) > debounceDelay)
    {
        requestQueue |= JOYSTICK_PRESSED_QUEUE;
        digitalWrite(5, HIGH);
        delay(500);
        digitalWrite(5, LOW);
    }

}
