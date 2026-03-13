# OneWire
If you are working on a budget project involving several OneWire sensors like DS18B20, but can't affort the expensive "waterproof" metal cap variant, instead you're relying on the cheap TO-92 package version which doesn't work with the standard stm32f103's USART in Half-Duplex mode; If you are about to invest several hours on the debugging the code and evaluating waveforms just to find out that the cheaper version dosn't satisfy standard time constraints and has several issues when the stm32 master force its pins in PUSH-PULL configuration for writing. I created this OneWrire bit-banging library with the intent to supply a stable and reliable library for varios types of Single Wire sensors. This library isn't the fastest but I configured bit timings in order to work with most of the sensors, even if they are not satisfying standard OneWire timings.
# Library functions
 * void OneWireSetup();
 * uint8_t OneWire_Init();
 * void OneWire_WriteByte();
 * uint8_t OneWire_ReadByte();
 * void OneWire_Read_UID();
 * void OneWire_FindAllDevices();
# How to install
0. (OPTIONAL) Create a blank STM32F103 project, or use your existing one and navigate to the 'Core' folder
1. Download '**Core/Inc/OneWire.h**' and put inside '**Inc**' folder
2. Download '**Core/Src/OneWire.c**' and put inside '**Src**' folder
3. In 'main.c' add ' **#include "OneWire.h**" ' between '**/*USER CODE BEGIN Includes*/**' and '**/*USER CODE END Includes*/**'
4. Don't forget to initialize OneWire library by calling '**OneWireSetup(GPIOB, GPIO_PIN_10, 10);**' before the while(1) loop, ideally inside /*USER CODE BEGIN 2*/
5. (OPTIONAL) Run my code from my 'main.c' before and inside the while(1) loop to test if it works
# How to use
 - Before reading or writing anything, the OneWire bus has to initialize slaves. Don't forget to call '**OneWire_Init()**;' before each transaction
 - Regardless of the number of OneWire devices connected to the bus, the protocol forces to select the slave after each bus initialization.
   If there is only one slave present on the bus, there can be used the SKIP-ROM command (0xCC), otherwise the MATCH-ROM command (0x55) has to be used to select the corresponding slave.
   Currently my example uses the SKIP-ROM command only, but I'm planning to add examples of using each ROM commands in the future.
 - Use '**OneWire_WriteByte(uint8_t byte)**' to write bytes to the slave
 - Use '**uint8_t OneWire_ReadByte()**' to read bytes from the slave. For this, firstly set the slave to enter reading mode with the READ-SCRATCHPAD command (0xBE)
 - For initializing the library you must call the '**OneWireSetup(GPIOB, GPIO_PIN_10, 10);**' function where you are defining the pin to be used for communication.
   For example, if you want to use the PC13 pin for communication, you should write '**OneWireSetup(GPIOC, GPIO_PIN_13, 13);**'
# Example
```
OneWireSetup(GPIOB, GPIO_PIN_10, 10);                                   // Set up OneWire on PB10

OneWire_Init();                                                         // Initialize communication
OneWire_WriteByte(0xCC);                                                // SKIP-ROM command (~MULTICAST)
OneWire_WriteByte(0x44);                                                // Send CONVERT-T command

HAL_Delay(800);                                                         // Wait DS18B20 to complete tempreature conversion

OneWire_Init();                                                         // Initialize communication
OneWire_WriteByte(0xCC);                                                // SKIP-ROM command, when only one DS18B20 is on the bus
OneWire_WriteByte(0xBE);                                                // Send 'Read scratchpad' command

uint16_t rawTemp = OneWire_ReadByte() | (OneWire_ReadByte() << 8);      // (MSB << 8) | (LSB)
float Temperature = rawTemp / 16.0f;
```
