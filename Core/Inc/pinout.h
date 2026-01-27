/*
 * pinout.h
 *
 *  Created on: Dec 9, 2021
 *      Author: opavl
 */

#ifndef INC_PINOUT_H_
#define INC_PINOUT_H_

// Note: Device Timepix2-Lite is STM32U5 based devices. Others board as mainboard are not develop.
// Note: Try to write code that will be easy to change, when others board will be develop or integrated

// Choose MAINBOARD type:
#define BOARD_STM32U5
//#define BOARD_STM32G4
//#define BOARD_VA416X0

// Choose CHIPBOARD type:
#define CHIPBOARD_TPX2	// Timepix2 chipboard
#define CHIPBOARD_SPX3	// SpacePix3 chipboard

// Conection status:
//#define TIMEPIX_CONNECT
#define SPACEPIX_CONNECT


#define GENERAL_UPDATE
//#define OLD

#if defined(BOARD_STM32U5)
	#include "stm32u5xx_hal.h"
// SPI
	extern 	SPI_HandleTypeDef  hspi1;
	#define SPI_PERIPHERY      &hspi1
	extern  SPI_HandleTypeDef  hspi2;
	#define SPI_TX	       	   &hspi2	// DATA_IN
	extern  SPI_HandleTypeDef  hspi3;
	#define SPI_RX	           &hspi3	// DATA_OUT
	// I2C
	extern  I2C_HandleTypeDef  hi2c1;
	#define I2C_PERIPHERY	   &hi2c1
	// UART
	#define UART &huart3
	#ifdef __GNUC__
	#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
	#else
	#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
	#endif
	// TIMER
	extern 	TIM_HandleTypeDef	htim15;
	#define MCLOCK_NUMBER		&htim15
	#define MCLOCK_CHANEL		TIM_CHANNEL_2
	#define MCLOCK_port			GPIOG
	#define MCLOCK_pin			GPIO_PIN_11
	// ADC
	extern 	ADC_HandleTypeDef  		    hadc1;
	#define ADC_HV   	   	   			&hadc1
	extern  ADC_HandleTypeDef  		    hadc4;
	#define ADC_DACOUT   	   	   		&hadc4

	#if defined(CHIPBOARD_TPX2)

	// TPX2 COMM
		// DATA_IN
		#define TPX2_nCS_IN_port						GPIOG
		#define TPX2_nCS_IN_pin							GPIO_PIN_12
		// DATA_OUT
		#define TPX2_nCS_OUT_port						GPIOD
		#define TPX2_nCS_OUT_pin						GPIO_PIN_5
		// I/O
		#define nCS_HV_port								GPIOC
		#define nCS_HV_pin								GPIO_PIN_14
		#define nCS_MX_port								GPIOE
		#define nCS_MX_pin								GPIO_PIN_0
		#define TPX2_DAC_OUT_port						GPIOF
		#define TPX2_DAC_OUT_pin						GPIO_PIN_15
		#define TPX2_BURN_EN_port						GPIOA
		#define TPX2_BURN_EN_pin						GPIO_PIN_8
		#define TPX2_BIAS_VOLT_MONITOR_port				GPIOB
		#define TPX2_BIAS_VOLT_MONITOR_pin				GPIO_PIN_2
		#define TPX2_EN_MONITOR_port					GPIOC
		#define TPX2_EN_MONITOR_pin						GPIO_PIN_6
		#define TPX2_SHUTTER_port						GPIOG
		#define TPX2_SHUTTER_pin						GPIO_PIN_3
		#define TPX2_READREADY_port						GPIOI
		#define TPX2_READREADY_pin						GPIO_PIN_10
		#define TPX2_MATRIX_OCC_port					GPIOI
		#define TPX2_MATRIX_OCC_pin						GPIO_PIN_13
		#define TPX2_GLOBAL_RESET_port					GPIOJ
		#define TPX2_GLOBAL_RESET_pin					GPIO_PIN_0
		#define EN_1V2_port								GPIOD
		#define EN_1V2_pin								GPIO_PIN_2

		#define TPX2_EN_2V5_port						GPIOI
		#define TPX2_EN_2V5_pin							GPIO_PIN_0
		#define TPX2_PWR_EN_port						GPIOJ
		#define TPX2_PWR_EN_pin							GPIO_PIN_7
	// USER
		#define LED_port								GPIOC
		#define LED_pin									GPIO_PIN_8
		// SET/RESET..
		#define GPIO_RESET 								0
		#define GPIO_SET 								1

		#define SPI3_SCK_port							GPIOG
		#define SPI3_SCK_pin							GPIO_PIN_9

		#define SPI2_SCK_port							GPIOD
		#define SPI2_SCK_pin							GPIO_PIN_3

		// HV
		#define HV_MIN_VALUE							0xff
		#define HV_TURN_OFF								0x00
		#define SIZE_ADC_HV_BUF							10
		#define ADC_HV_RESULOTION						14
		// voltage divider for HV : Vout = R2/(R2+R1) -> Vout is measuring by ADC
		#define R2										43000
		#define R1										2000000

		// DACOUT
		#define SIZE_ADC_DACOUT_BUF						1
		#define ADC_DACOUT_RESULOTION					12

		// TEMP
		#define TEMP_REG								0x00
		#define	CONFIG_REG								0x01
	#endif
	#if defined(CHIPBOARD_SPX3)
		//TODO definition of pin for SpacePix Chipboard
		// TPX2 COMM
		// DATA_IN
		#define SPX3_nCS_IN_port						GPIOG
		#define SPX3_nCS_IN_pin							GPIO_PIN_12
		// DATA_OUT
		#define TPX2_nCS_OUT_port						GPIOD
		#define TPX2_nCS_OUT_pin						GPIO_PIN_5
		// I/O
		#define nCS_HV_port								GPIOC
		#define nCS_HV_pin								GPIO_PIN_14
		#define nCS_MX_port								GPIOE
		#define nCS_MX_pin								GPIO_PIN_0
		#define TPX2_DAC_OUT_port						GPIOF
		#define TPX2_DAC_OUT_pin						GPIO_PIN_15

		#define TPX2_BIAS_VOLT_MONITOR_port				GPIOB
		#define TPX2_BIAS_VOLT_MONITOR_pin				GPIO_PIN_2

		//#define SPX3_MODE_SELECT_port					GPIOC
		//#define SPX3_MODE_SELECT_pin					GPIO_PIN_6
		#define SPX3_INJ_EN_port						GPIOC
		#define SPX3_INJ_EN_pin							GPIO_PIN_6

		#define SPX3_CNT_EN_port						GPIOG
		#define SPX3_CNT_EN_pin							GPIO_PIN_3
		#define SPX3_CLR_N_port							GPIOI
		#define SPX3_CLR_N_pin							GPIO_PIN_10
		#define SPX3_CNF_EN_port						GPIOI
		#define SPX3_CNF_EN_pin							GPIO_PIN_13
		#define SPX3_ROW_SHIFT_port						GPIOJ
		#define SPX3_ROW_SHIFT_pin						GPIO_PIN_0

		#define SPX3_OE_port							GPIOA
		#define SPX3_OE_pin								GPIO_PIN_8



		#define EN_1V2_port								GPIOD
		#define EN_1V2_pin								GPIO_PIN_2

		#define TPX2_EN_2V5_port						GPIOI
		#define TPX2_EN_2V5_pin							GPIO_PIN_0
		#define SPX3_PWR_EN_port						GPIOJ
		#define SPX3_PWR_EN_pin							GPIO_PIN_7
	// USER
		#define LED_port								GPIOC
		#define LED_pin									GPIO_PIN_8
		// SET/RESET..
		#define GPIO_RESET 								0
		#define GPIO_SET 								1

		#define SPI3_SCK_port							GPIOG
		#define SPI3_SCK_pin							GPIO_PIN_9

		#define SPI2_SCK_port							GPIOD
		#define SPI2_SCK_pin							GPIO_PIN_3

		// HV
		#define HV_MIN_VALUE							0xff
		#define HV_TURN_OFF								0x00
		#define SIZE_ADC_HV_BUF							10
		#define ADC_HV_RESULOTION						14
		// voltage divider for HV : Vout = R2/(R2+R1) -> Vout is measuring by ADC
		#define R2										43000
		#define R1										2000000

		// DACOUT
		#define SIZE_ADC_DACOUT_BUF						5
		#define ADC_DACOUT_RESULOTION					12

		// TEMP
		#define TEMP_REG								0x00
		#define	CONFIG_REG								0x01

	#endif


#elif defined(BOARD_STM32G4)
	#include "stm32g4xx_hal.h"

	// PERIPHERY
	extern 	SPI_HandleTypeDef hspi1;
	#define SPI_TPX2_TOP      &hspi1
	extern SPI_HandleTypeDef  hspi2;
	#define SPI_TPX2_BOT	  &hspi2
	extern SPI_HandleTypeDef  hspi3;
	#define SPI_PERIPHERY	  &hspi3

	extern I2C_HandleTypeDef  hi2c1;
	#define I2C_PERIPHERY	  &hi2c1

	extern TIM_HandleTypeDef  htim1;
	#define TPX2_TOP_MCLOCK   &htim1
	extern TIM_HandleTypeDef  htim3;
	#define TPX2_BOT_MCLOCK   &htim3

	////////////// TPX2 TOP comm pins/////////////
	#define TPX2_TOP_EN_1V8_port					GPIOC
	#define TPX2_TOP_EN_1V8_pin						GPIO_PIN_13
	#define TPX2_TOP_GLOBAL_RESET_port				GPIOB
	#define TPX2_TOP_GLOBAL_RESET_pin				GPIO_PIN_6
	#define TPX2_TOP_nCS_port						GPIOA
	#define TPX2_TOP_nCS_pin                		GPIO_PIN_4
	#define TPX2_TOP_SHUTTER_COUNTER_port			GPIOB
	#define TPX2_TOP_SHUTTER_COUNTER_pin			GPIO_PIN_5
	#define TPX2_TOP_DCLOCK_IN_port					GPIOA
	#define TPX2_TOP_DCLOCK_IN_pin					GPIO_PIN_5
	#define TPX2_TOP_DATA_IN_port					GPIOA
	#define TPX2_TOP_DATA_IN_pin					GPIO_PIN_7
	#define TPX2_TOP_DATA_OUT_port					GPIOA
	#define TPX2_TOP_DATA_OUT_pin					GPIO_PIN_6


	////////////// TPX2 TOP HV/////////////
	#define TPX2_TOP_HV_nCS_port					GPIOE
	#define TPX2_TOP_HV_nCS_pin						GPIO_PIN_5

	////////////// TPX2 BOT pins/////////////
	#define TPX2_BOT_EN_1V8_port					GPIOB
	#define TPX2_BOT_EN_1V8_pin						GPIO_PIN_7
	#define TPX2_BOT_GLOBAL_RESET_port				GPIOB
	#define TPX2_BOT_GLOBAL_RESET_pin				GPIO_PIN_12
	#define TPX2_BOT_nCS_port						GPIOB
	#define TPX2_BOT_nCS_pin                		GPIO_PIN_13
	#define TPX2_BOT_SHUTTER_COUNTER_port			GPIOE
	#define TPX2_BOT_SHUTTER_COUNTER_pin			GPIO_PIN_2

	////////////// TPX2 TOP HV/////////////
	#define TPX2_BOT_HV_nCS_port					GPIOE
	#define TPX2_BOT_HV_nCS_pin						GPIO_PIN_6

	////////////// TPX2 TOP BURN_EN/////////////
	#define TPX2_TOP_BURN_EN_port					GPIOC
	#define TPX2_TOP_BURN_EN_pin						GPIO_PIN_1

	// Variables
	#define SIZE_OF_BUF								11
	#define VCM_SLVS_SET							0x45
	#define VBIAS_SLVS_SET							0x45
	#define SIZE_8_BIT								8
	#define SIZE_16_BIT								16
	#define SIZE_24_BIT								24
	#define SIZE_32_BIT								32
	#define HV_VALUE								0x10		// TODO make struct: HV_100V <-> 0x53.. etc.

	//address for AD799x-x
	#define address_AD799X_0						0x28
	#define address_AD799X_1						0x29
	#define AD_VIN0									0x20
	#define AD_VIN1									0x40
	#define AD_VIN2									0x80

	// SET/RESET..
	#define GPIO_RESET 								0
	#define GPIO_SET 								1

	// PRINT TO SERIAL
	#define UART &huart3
	#ifdef __GNUC__
	#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
	#else
	#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
	#endif /* __GNUC__ */
	// enable debug print
	/*
	#define DEBUG_LOG
	#ifdef DEBUG_LOG
	#define dbg_printf(...)   printf(__VA_ARGS__)
	#else
	#define dbg_printf(...)
	#endif
	*/
#elif defined(BOARD_VA416X0)
	// TODO config all of pins for VA41680

	//PERIPHERY
	#define SPI_TPX2_TOP						((VOR_SPI_Type            *) VOR_SPI2_BASE)
	#define SPI_TPX2_BOT						((VOR_SPI_Type            *) VOR_SPI1_BASE)
	#define TPX2_TOP_MCLOCK						3
	#define TPX2_BOT_MCLOCK						11
	#define	SPI_PERIPHERY						((VOR_SPI_Type            *) VOR_SPI0_BASE)

	////////////// TPX2 TOP comm pins/////////////
	#define TPX2_TOP_EN_1V8_port				VOR_PORTA
	#define TPX2_TOP_EN_1V8_pin					13
	#define TPX2_TOP_GLOBAL_RESET_port 			VOR_PORTA
	#define TPX2_TOP_GLOBAL_RESET_pin			11
	#define TPX2_TOP_nCS_port					VOR_PORTA
	#define TPX2_TOP_nCS_pin					4
	#define TPX2_TOP_SHUTTER_COUNTER_port		VOR_PORTA
	#define TPX2_TOP_SHUTTER_COUNTER_pin		10

	////////////// TPX2 TOP HV/////////////
	#define TPX2_TOP_HV_nCS_port				GPIOA
	#define TPX2_TOP_HV_nCS_pin					GPIO_PIN_12

	////////////// TPX2 BOT comm pins/////////////
	#define TPX2_BOT_EN_1V8_port				VOR_PORTA
	#define TPX2_BOT_EN_1V8_pin					15
	#define TPX2_BOT_GLOBAL_RESET_port 			VOR_PORTF
	#define TPX2_BOT_GLOBAL_RESET_pin			11
	#define TPX2_BOT_nCS_port					VOR_PORTB
	#define TPX2_BOT_nCS_pin					7
	#define TPX2_BOT_SHUTTER_COUNTER_port		VOR_PORTF
	#define TPX2_BOT_SHUTTER_COUNTER_pin		7

	////////////// TPX2 BOT HV/////////////
	#define TPX2_BOT_HV_nCS_port				GPIOA
	#define TPX2_BOT_HV_nCS_pin					GPIO_PIN_14


	//Variables
	#define TPX2_TOP_MCLOCK_SPEED				500000		// 500 kHz
	#define	TPX2_TOP_MCLOCK_DUTY_50				50*1000		// Duty 50%

	#define TPX2_BOT_MCLOCK_SPEED				500000		// 500 kHz
	#define	TPX2_BOT_MCLOCK_DUTY_50				50*1000		// Duty 50%

	// WATCHDOG
	#define WDOG_MS (50)
	#define WDFEED() VOR_WATCH_DOG->WDOGINTCLR = 1

#else
	//		config of pins of diferent board
#endif


#endif /* INC_PINOUT_H_ */
