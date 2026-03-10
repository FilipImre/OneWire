/*
 * OneWire.c
 *
 *  Created on: Mar 8, 2026
 *      Author: filip
 */

#include "OneWire.h"


GPIO_TypeDef *SingleWirePort;
uint16_t SingleWirepinMask;
uint8_t SingleWirepin;
OneWireUID OneWireUIDs[32] = { { {0} } };
uint8_t OneWireDevsNo = 0;


static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enabling monitoring unit
    DWT->CYCCNT = 0;                                // Reset counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;           // Activate counter
}

static void delay_us(uint32_t us)
{
    uint32_t startTick = DWT->CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000); // 72 tick per us at 72MHz
    while ((DWT->CYCCNT - startTick) < delayTicks);
}

// gpio_set_mode(GPIOB, 10, 0x01); <- open-drain
// gpio_set_mode(GPIOB, 10, 0x00); <- push-pull
static inline void gpio_set_mode(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    volatile uint32_t *reg = (pin < 8) ? &port->CRL : &port->CRH;
    uint8_t shift = (pin % 8) * 4;

    uint32_t regToBeWritten = *reg;
    regToBeWritten &= ~(0xF << shift);
    regToBeWritten |= (0b11 | ((mode & 0x3) << 2)) << shift;
    *reg = regToBeWritten;   // single write → safe
}

uint8_t OneWire_Init(void)
{
	uint8_t Presence_Byte = 0;

	// Generating RESET pulse
	gpio_set_mode(SingleWirePort, SingleWirepin, PIN_MODE_PUSH_PULL);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET); // Falling edge
	delay_us(500); // Generate 500 usec RESET pulse
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET); // Rsising edge

	// Checking presence pulse
	gpio_set_mode(SingleWirePort, SingleWirepin, PIN_MODE_OPEN_DRAIN);
	delay_us(10); // (t+10)
	Presence_Byte |= ( ((GPIOB->IDR >> 10) & 0x1) << 0); // should be still HIGH
	delay_us(50); // (t+60)
	Presence_Byte |= ( ((GPIOB->IDR >> 10) & 0x1) << 1); // should be LOW yet
	delay_us(14); // (t+74)
	Presence_Byte |= ( ((GPIOB->IDR >> 10) & 0x1) << 2); // should be still LOW
	delay_us(227); // (t+301)
	Presence_Byte |= ( ((GPIOB->IDR >> 10) & 0x1) << 3); // should be HIGH
	delay_us(49); // Safe wait margin before attempting to write anything (t~350 usec)

	// Set PB10 to push-pull output, 10 MHz
	//gpio_set_mode(SingleWirePort, SingleWirepin, PIN_MODE_PUSH_PULL);
	//delay_us(5); // Safe transition from open-drain to push-pull
	return (Presence_Byte == 0b00001001) ? 1 : 0; // HIGH->LOW->LOW->HIGH
}

/*
 * Must write the 'Read Scratchpad' command (0xBE) before reading, as this function deosn't initialize bus communication
 * */
uint8_t OneWire_ReadByte(void)
{
    uint8_t RxByte = 0;

    // PB10 to open-drain
    gpio_set_mode(SingleWirePort, SingleWirepin, PIN_MODE_OPEN_DRAIN);
	delay_us(1);

	// Reading a byte
	for(uint8_t i=0; i<8; i++)
	{
	  // Generating read pulse (10 usec pulse and read from t=16 usec)
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);
	  delay_us(10);
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET);
	  delay_us(6); //start sampling from t+16 usec

	  uint16_t line_sampled = 0;
	  for(uint8_t samp=0; samp<16; samp++) // 16x sampling the line starting from 16 usec with a delay of 1 usec each
	  {
		  line_sampled |= (((GPIOB->IDR >> 10) & 0x1) << samp); // sample the line (LOW or HIGH) and insert into "line_sampled"
		  //delay_us(1); // 1 usec delay between each line sampling
	  }
	  RxByte |= (((__builtin_popcount(line_sampled) >= 8) ? 1 : 0) << i); // 1 if majority high, else 0, LSB first
	  delay_us(45); // t>60 usec: Safe wait limit till the reading of the next bit
	}
    return RxByte;
}

void OneWire_WriteByte(uint8_t data)
{
	// Set PB10 to push-pull output, 10 MHz
	gpio_set_mode(SingleWirePort, SingleWirepin, PIN_MODE_PUSH_PULL);
	delay_us(1);
    for (int i=0; i<8; i++) // ~80 usec per bit, total 640 usec pe byte
    {
    	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET); // Falling edge
    	if((data & (1<<i)) != 0) // writing '1'
    	{
    		delay_us(10); // 5 usec LOW
    		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET); // Rising edge after 6 usec
    		delay_us(70); // 65 usec HIGH, total: ~70 usec
    	}else // Writing '0'
    	{
    		delay_us(70); // 60 usec LOW
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET); // Rising edge after 60 usec
			delay_us(10); // 10 usec HIGH, total: ~70 usec
    	}
    }
}

static uint8_t OneWire_ReadROMBits()
{
	uint8_t bits = 0; // bit_0 = ID_bit, bit_1 = Complement_ID_bit

	for(uint8_t bitPos=0; bitPos<2; bitPos++)
	{
		gpio_set_mode(SingleWirePort, SingleWirepin, PIN_MODE_PUSH_PULL); // PB10 to open-drain
		delay_us(1);
		// Generating read pulse (10 usec pulse and read from t=16 usec)
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);
		delay_us(10);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET);
		delay_us(6); //start sampling from t+16 usec
		gpio_set_mode(SingleWirePort, SingleWirepin, PIN_MODE_OPEN_DRAIN); // PB10 to open-drain
		delay_us(1);

		// Sampling the line
		uint16_t line_sampled = 0;
		for(uint8_t samp=0; samp<16; samp++) // 16x sampling the line starting from 16 usec with a delay of 1 usec each
		{
		  line_sampled |= (((GPIOB->IDR >> 10) & 0x1) << samp); // sample the line (LOW or HIGH) and insert into "line_sampled"
		  //delay_us(1); // 1 usec delay between each line sampling
		}
		bits |= (((__builtin_popcount(line_sampled) >= 8) ? 1 : 0) << bitPos); // 1 if majority high, else 0, LSB first
		delay_us(45); // t>60 usec: Safe wait limit till the reading of the next bit
	}

	return bits & 0b11;
}

static void OneWire_SetSearchDirection(uint8_t direction)
{
	gpio_set_mode(SingleWirePort, SingleWirepin, PIN_MODE_PUSH_PULL);
	delay_us(1); // Safe wait to reconfigure GPIO pin
	if(direction) // '1'
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET); // Falling edge
		delay_us(10); // 5 usec LOW
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET); // Rising edge after 6 usec
		delay_us(70); // 65 usec HIGH, total: ~70 usec
	}else // '0'
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET); // Falling edge
		delay_us(70); // 60 usec LOW
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET); // Rising edge after 60 usec
		delay_us(10); // 10 usec HIGH, total: ~70 usec
	}
}

void OneWire_FindAllDevices(void) // The implementation and adaptation of the original Dalla Semiconductor's fininf algorithm
{
	uint8_t LastDiscrepancy = 0;
	uint8_t LastDeviceFlag  = 0;

	uint8_t id_bit_number;
	uint8_t last_zero;
	uint8_t rom_byte_number;
	uint8_t rom_byte_mask;
	uint8_t bits, id_bit, comp_bit, search_direction;

	while (!LastDeviceFlag)
	{
		id_bit_number = 1;
		last_zero = 0;
		rom_byte_number = 0;
		rom_byte_mask = 1;

		// clear ROM buffer
		for (uint8_t i = 0; i < 8; i++)
			OneWireUIDs[OneWireDevsNo].address[i] = 0;

		// reset pulse - > check presence
		if (!OneWire_Init()) {
			return;
		}

		// send SEARCH ROM command
		OneWire_WriteByte(0xF0);

		// search 64 bits
		do {
			bits = OneWire_ReadROMBits();   // bit0 = id, bit1 = comp
			id_bit  =  bits & 1;
			comp_bit = (bits >> 1) & 1;

			if (id_bit && comp_bit)
				break; // error - > no devices responding

			if (id_bit != comp_bit) {
				// only one valid direction - > follow it
				search_direction = id_bit;
			} else {
				// discrepancy - > both 0 and 1 exist
				if (id_bit_number < LastDiscrepancy) {
					// before last discrepancy - > follow previous path
					search_direction =
						(OneWireUIDs[OneWireDevsNo].address[rom_byte_number] & rom_byte_mask) ? 1 : 0;
				} else {
					// at or beyond last discrepancy - > choose new branch
					search_direction = (id_bit_number == LastDiscrepancy);
				}

				// if choosing 0 - > remember this discrepancy
				if (search_direction == 0)
					last_zero = id_bit_number;
			}

			// write chosen direction bit
			OneWire_SetSearchDirection(search_direction);

			// store bit into ROM buffer
			if (search_direction)
				OneWireUIDs[OneWireDevsNo].address[rom_byte_number] |= rom_byte_mask;
			else
				OneWireUIDs[OneWireDevsNo].address[rom_byte_number] &= (uint8_t)~rom_byte_mask;

			// advance to next bit
			id_bit_number++;
			rom_byte_mask <<= 1;

			if (rom_byte_mask == 0) {
				rom_byte_number++;
				rom_byte_mask = 1;
			}

		} while (rom_byte_number < 8);

		// update search state
		LastDiscrepancy = last_zero;

		if (LastDiscrepancy == 0)
			LastDeviceFlag = 1;

		// validate ROM (not all zeros)
		uint8_t valid = 0;
		for (uint8_t i = 0; i < 8; i++)
			if (OneWireUIDs[OneWireDevsNo].address[i] != 0x00)
				valid = 1;

		if (!valid)
			return;

		OneWireDevsNo++;

		if (OneWireDevsNo >= MAX_ONEWIRE_DEVICE_NUMBER)
			return;
	}
}

void OneWire_Read_UID(void)
{
	OneWire_Init();
	OneWire_WriteByte(ROM_READ); // READ-ROM
	for(uint8_t i=0; i<8; i++)
	{
		OneWireUIDs[0].address[i] = OneWire_ReadByte();
	}
}

/*
 * port = GPIOA, GPIOB, GPIOC, ...
 * pinMask = GPIO_PIN_10, ...
 * pin = 10, ...
 * */
void OneWireSetup(GPIO_TypeDef *port, uint16_t pinMask, uint8_t pin)
{
	// Setting up definitions
	SingleWirePort = port;
	SingleWirepinMask = pinMask;
	SingleWirepin = pin;

	// GPIO setup
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(port, &GPIO_InitStruct);

	// Microsecond timer init
	DWT_Init();
}
