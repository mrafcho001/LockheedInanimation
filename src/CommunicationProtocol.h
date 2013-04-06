#ifndef COMMUNICATIONPROTOCOL_H
#define COMMUNICATIONPROTOCOL_H


#define MESSAGE_TYPE_REQUEST            (1<<0)
#define MESSAGE_TYPE_RESPONSE   	    (1<<1)
#define MESSAGE_TYPE_ASYNC      		(1<<2)
#define MESSAGE_ECHO            		(1<<3)
#define MESSAGE_DIRECTION_H     		(1<<4)
#define MESSAGE_DIRECTION_V     		(1<<5)
#define MESSAGE_BOOL_TRUE       		(1<<6)
#define MESSAGE_BOOL_FALSE      		(1<<7)
#define MESSAGE_ADJUST_POSITION 		(1<<8)
#define MESSAGE_MANUAL_CONTROL  		(1<<9)
#define MESSAGE_POSITION        		(1<<10)
#define MESSAGE_ACKNOWLEDGEMENT 		(1<<11)
#define MESSAGE_MODE_SWITCH_BIT   		(1<<12)
#define MESSAGE_POSITION_REACHED_BIT    (1<<13)

//Bits 15, 14 are reserved for param count!
#define MESSAGE_PARAM_COUNT_0           (0<<14)
#define MESSAGE_PARAM_COUNT_1           (1<<14)
#define MESSAGE_PARAM_COUNT_2           (1<<15)
#define MESSAGE_PARAM_MASK              (0xC000)



#define MESSAGE_ECHO_REQUEST        (MESSAGE_ECHO | MESSAGE_TYPE_REQUEST | MESSAGE_PARAM_COUNT_0)
#define MESSAGE_ECHO_RESPONSE       (MESSAGE_ECHO | MESSAGE_TYPE_RESPONSE | MESSAGE_PARAM_COUNT_0)

#define MESSAGE_ACK                 (MESSAGE_ACKNOWLEDGEMENT | MESSAGE_BOOL_TRUE | MESSAGE_TYPE_RESPONSE | MESSAGE_PARAM_COUNT_0)
#define MESSAGE_NACK                (MESSAGE_ACKNOWLEDGEMENT | MESSAGE_BOOL_FALSE | MESSAGE_TYPE_RESPONSE | MESSAGE_PARAM_COUNT_0)

#define MESSAGE_ENABLE_MANUAL_CONTROLS (MESSAGE_MANUAL_CONTROL | MESSAGE_BOOL_TRUE | MESSAGE_TYPE_REQUEST | MESSAGE_PARAM_COUNT_0)
#define MESSAGE_DISABLE_MANUAL_CONTROLS (MESSAGE_MANUAL_CONTROL | MESSAGE_BOOL_FALSE | MESSAGE_TYPE_REQUEST | MESSAGE_PARAM_COUNT_0)

#define MESSAGE_MODE_SWITCH (MESSAGE_MODE_SWITCH_BIT | MESSAGE_TYPE_ASYNC | MESSAGE_PARAM_COUNT_0)

#define MESSAGE_ADJUST_H_POSITION   (MESSAGE_ADJUST_POSITION | MESSAGE_DIRECTION_H | MESSAGE_TYPE_REQUEST | MESSAGE_PARAM_COUNT_1)
#define MESSAGE_ADJUST_V_POSITION   (MESSAGE_ADJUST_POSITION | MESSAGE_DIRECTION_V | MESSAGE_TYPE_REQUEST | MESSAGE_PARAM_COUNT_1)

#define MESSAGE_POSITION_REQUEST    (MESSAGE_POSITION | MESSAGE_TYPE_REQUEST | MESSAGE_PARAM_COUNT_0)
#define MESSAGE_POSITION_RESPONSE   (MESSAGE_POSITION | MESSAGE_TYPE_RESPONSE | MESSAGE_PARAM_COUNT_2)
#define MESSAGE_POSITION_H_REQUEST  (MESSAGE_POSITION | MESSAGE_DIRECTION_H | MESSAGE_TYPE_REQUEST | MESSAGE_PARAM_COUNT_0)
#define MESSAGE_POSITION_V_REQUEST  (MESSAGE_POSITION | MESSAGE_DIRECTION_V | MESSAGE_TYPE_REQUEST | MESSAGE_PARAM_COUNT_0)
#define MESSAGE_POSITION_H_RESPONSE  (MESSAGE_POSITION | MESSAGE_DIRECTION_H | MESSAGE_TYPE_RESPONSE | MESSAGE_PARAM_COUNT_1)
#define MESSAGE_POSITION_V_RESPONSE  (MESSAGE_POSITION | MESSAGE_DIRECTION_V | MESSAGE_TYPE_RESPONSE | MESSAGE_PARAM_COUNT_1)
#define MESSAGE_POSITION_UPDATE     (MESSAGE_POSITION | MESSAGE_TYPE_ASYNC | MESSAGE_PARAM_COUNT_2)

#define MESSAGE_POSITION_REACHED    (MESSAGE_POSITION_REACHED_BIT | MESSAGE_TYPE_ASYNC | MESSAGE_PARAM_COUNT_0)
#define MESSAGE_POSITION_H_REACHED  (MESSAGE_POSITION_REACHED_BIT | MESSAGE_DIRECTION_H | MESSAGE_TYPE_ASYNC | MESSAGE_PARAM_COUNT_0)
#define MESSAGE_POSITION_V_REACHED  (MESSAGE_POSITION_REACHED_BIT | MESSAGE_DIRECTION_V | MESSAGE_TYPE_ASYNC | MESSAGE_PARAM_COUNT_0)

//useful macros
#define GET_MSG_PARAM_COUNT(msg)    ((unsigned)(msg & MESSAGE_PARAM_MASK)>>14)


#endif // COMMUNICATIONPROTOCOL_H