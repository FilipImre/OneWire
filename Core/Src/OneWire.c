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

// ===================================================================================
	  // Set PB10 to push-pull output, 10 MHz
	  //GPIOB->CRH &= ~(0xF << 8);     // clear bits 11:8
	  //GPIOB->CRH |=  (0x1 << 8);     // MODE=01 (10 MHz), CNF=00 (push-pull)

	  // Set PB10 to open-drain output, 10 MHz
	  //GPIOB->CRH &= ~(0xF << 8);     // clear bits 11:8
	  //GPIOB->CRH |=  (0x5 << 8);     // MODE=01, CNF=01 → open-drain

	  //uint8_t pin_state = (GPIOB->IDR >> 10) & 0x1;
	  //recved |= (bit << i);
// ===================================================================================

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


static inline void gpio_set_mode(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    volatile uint32_t *reg = (pin < 8) ? &port->CRL : &port->CRH;
    uint8_t shift = (pin % 8) * 4;

    uint32_t regToBeWritten = *reg;
    regToBeWritten &= ~(0xF << shift);
    regToBeWritten |= (0b11 | ((mode & 0x3) << 2)) << shift;
    *reg = regToBeWritten;   // single write → safe
}

// gpio_set_mode(GPIOB, 10, 0x01); <- open-drain
// gpio_set_mode(GPIOB, 10, 0x00); <- push-pull


uint8_t OneWire_Init(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET); // Falling edge
	delay_us(500); // Generate 500 usec RESET pulse
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET); // Rsising edge
	delay_us(500); // Safe wait margin before attempting to write anything

	// Set PB10 to push-pull output, 10 MHz
	gpio_set_mode(SingleWirePort, SingleWirepin, PIN_MODE_PUSH_PULL);
	delay_us(5); // Safe transition from open-drain to push-pull
	return 1;
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
	  delay_us(6);

	  uint16_t line_sampled = 0;
	  for(uint8_t samp=0; samp<16; samp++) // 16x sampling the line starting from 16 usec with a delay of 1 usec each
	  {
		  line_sampled |= (((GPIOB->IDR >> 10) & 0x1) << samp); // sample the line (LOW or HIGH) and insert into "line_sampled"
		  //delay_us(1); // 1 usec delay between each line sampling
	  }
	  RxByte |= (((__builtin_popcount(line_sampled) >= 8) ? 1 : 0) << i); // 1 if majority high, else 0, LSB first
	  delay_us(35); // Safe wait limit till the reading of the next bit
	}
    return RxByte;
}

void OneWire_WriteByte(uint8_t data)
{
	// Set PB10 to push-pull output, 10 MHz
	gpio_set_mode(SingleWirePort, SingleWirepin, PIN_MODE_PUSH_PULL);
	GPIOB->CRH &= ~(0xF << 8);     // clear bits 11:8
	GPIOB->CRH |=  (0x1 << 8);     // MODE=01 (10 MHz), CNF=00 (push-pull)
	delay_us(1);
    for (int i=0; i<8; i++)
    {
    	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET); // Falling edge
    	if((data & (1<<i)) != 0) // writing '1'
    	{
    		delay_us(5); // 6 usec LOW
    		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET); // Rising edge after 6 usec
    		delay_us(115); // 64 + 50 sec HIGH
    	}else // Writing '0'
    	{
    		delay_us(70); // 60 usec LOW
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET); // Rising edge after 60 usec
			delay_us(50); // 10 + 50 sec HIGH
    	}
    }
    delay_us(100); // add a safety delay and fill duration to ~500 usec
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
