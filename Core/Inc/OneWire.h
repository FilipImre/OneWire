/*
 * OneWire.h
 *
 *  Created on: Mar 8, 2026
 *      Author: Filip Imre
 */

#ifndef INC_ONEWIRE_H_
#define INC_ONEWIRE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "main.h"
#include "stm32f1xx_hal.h"

// SLAVE related
#define MAX_ONEWIRE_DEVICE_NUMBER 64

typedef struct
{
    uint8_t address[8];
} OneWireUID;

extern uint8_t OneWireDevsNo;
extern OneWireUID OneWireUIDs[MAX_ONEWIRE_DEVICE_NUMBER];


extern uint8_t OneWireConfig; // |AE_E|x|x|x|x|x|x|x|
#define OWCONF_ATOMIC_EXEC_EN 0

// GPIO output configuration
#define PIN_MODE_PUSH_PULL 0x00
#define PIN_MODE_OPEN_DRAIN 0x01

// ROM commands
#define ROM_READ 0x33
#define ROM_MATCH 0x55
#define ROM_SKIP 0xCC
#define ROM_SEARCH 0xF0


#define ENABLE_ONEWIRE()  (OneWireConfig |= (1<<OWCONF_ATOMIC_EXEC_EN))
#define DISABLE_ONEWIRE()  (OneWireConfig &= ~(1 << OWCONF_ATOMIC_EXEC_EN))
#define CHECK_OWENABLED()  ((OneWireConfig & (1 << OWCONF_ATOMIC_EXEC_EN)) ? 1 : 0)


#define PAUSE_INTERRUPTS(name)                 \
    uint32_t name = __get_PRIMASK();           \
    __disable_irq()

#define RESUME_INTERRUPTS(name)                \
    __set_PRIMASK(name)




extern void OneWireSetup(GPIO_TypeDef *port, uint16_t pinMask, uint8_t pin);
extern uint8_t OneWire_Init(void);
extern void OneWire_WriteByte(uint8_t data);
extern uint8_t OneWire_ReadByte(void);
extern void OneWire_Read_UID(void);
extern void OneWire_FindAllDevices(void);



#ifdef __cplusplus
}
#endif
#endif /* INC_ONEWIRE_H_ */
