/*
 * OneWire.h
 *
 *  Created on: Mar 8, 2026
 *      Author: filip
 */

#ifndef INC_ONEWIRE_H_
#define INC_ONEWIRE_H_
#ifdef __cplusplus
extern "C" {
#endif



#include <stdio.h>
#include "main.h"
#include "stm32f1xx_hal.h"

#define PIN_MODE_PUSH_PULL 0x00
#define PIN_MODE_OPEN_DRAIN 0x01

extern void OneWireSetup(GPIO_TypeDef *port, uint16_t pinMask, uint8_t pin);
extern uint8_t OneWire_Init(void);
extern void OneWire_WriteByte(uint8_t data);
extern uint8_t OneWire_ReadByte(void);



#ifdef __cplusplus
}
#endif
#endif /* INC_ONEWIRE_H_ */
