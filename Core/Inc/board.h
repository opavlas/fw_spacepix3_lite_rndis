/*
 * board.h
 *
 *  Created on: Dec 9, 2021
 *      Author: opavl
 */

#ifndef INC_BOARD_H_
#define INC_BOARD_H_

#include <stdint.h>
#include <stdbool.h>

#include "debug.h"
#include "pinout.h"
#include "tpx2_comm.h"
#include "spx3_comm.h"

//#define SPI_DMA
#define SPI_NORMAL
//#define HV_TEST
//#define HV_SCAN
//#define CHIP_ID
//#define MCLOCK
//#define DIGITAl_TEST
//#define TEST_DACOUT
///////////////////////////////// STM32 /////////////////////////////////////////
#if defined(BOARD_STM32U5)
	#include "pinout.h"
	#include "stm32u5xx_hal.h"
	#include "tpx2_comm.h"
	#define SPI_DEFAULT_TPX_TIMEOUT		100
	#define ADC_DEFAULT_TPX_TIMEOUT		1000
	#define I2C_DEFAULT_TPX_TIMEOUT		1000
#elif defined(BOARD_STM32G4)
	#include "pinout.h"
	#include "stm32g4xx_hal.h"
	#include "tpx2_comm.h"
	//#define STM32_SPI_DMA
	#define SPI_DEFAULT_TPX_TIMEOUT		1000
///////////////////////////////// Voragotech /////////////////////////////////////////
#elif defined(BOARD_VA416X0)
	#include "va416xx.h"
	#include "va416xx_hal.h"
	#include "va416xx_hal_adc.h"
	#include "va416xx_hal_clkgen.h"
	#include "va416xx_hal_dac.h"
	#include "va416xx_hal_dma.h"
	#include "va416xx_hal_uart.h"
	#include "va416xx_hal_timer.h"
	#include "va416xx_debug.h"
	#include "va416xx_hal_spi.h"
	#include "va416xx_hal_i2c.h"
	#include "cmd_interface.h"
	#include "dac_sine.h"
	#include "spi_fram.h"

	#define SPI_DEFAULT_TPX_TIMEOUT		1000
#else
	#error "Board is not defined!!"
#endif

typedef enum {
	BOARD_OK = 0,
	BOARD_FAILED = 1,
	HV_TPXA_FAILED = 2,
	HV_TPXB_FAILED = 3,
} BOARD_STATUS;

typedef enum {
	AD7991_0 = 0b0101000,
	AD7991_1 = 0b0101001,
	AD7995_0 = 0b0101000,
	AD7995_1 = 0b0101001,
	AD7999_1 = 0b0101001,
} AD_799x_ADDR;

// there is more option, look in DS
typedef enum {
	CH0 = 0b00010000,
	CH1 = 0b00100000,
	CH2 = 0b01000000,
	CH3 = 0b10000000,
} AD_799x_CH;

typedef enum {
	INJ_EN = 0b0000,
	BIAS_ADJ = 0b0001,
	ADC_IN = 0b0010,
	DACD = 0b0011,
} LTC2635_CH;

typedef enum {
	TMP112D_GND = 0x40,
	TMP112D_V 	= 0x41,
	TMP112D_SDA = 0x42,
	TMP112D_SCL = 0x43,
	TMP112D0	= 0x48,
} TMP112D_ADDR;

typedef enum {
	W_REG_N 				= 0b0000,
	UPDATE_REG_N 			= 0b0001,
	W_REG_N_UPDATE_REG_ALL 	= 0b0010,
	W_REG_N_UPDATE_REG_N 	= 0b0011,
	POWER_DOWN_N 			= 0b0100,
} LTC2635_CMD;

#define DAC_MAX_VALUE		1650			// coresponds to 1V on output

// Init board
uint8_t board_init(void);	// use only with different board than STM32

// GPIO functions TPX2
void board_set_1v2_pwr_en(TIMEPIX_ID tpxIndex, uint8_t state);											// PWR for MachXO2 Bank ONLY ! -> Timepix 2 has own 1V2 on Chipboard PCB
void board_tpx2_set_pwr_en1v2( TIMEPIX_ID tpxIndex, uint8_t state );
void board_tpx2_set_pwr_en2v5( TIMEPIX_ID tpxIndex, uint8_t state );
void board_tpx2_set_global_reset( TIMEPIX_ID tpxIndex, uint8_t state );
void board_tpx2_set_chip_select( TIMEPIX_ID tpxIndex, uint8_t state );
void board_tpx2_set_shutter_counter( TIMEPIX_ID tpxIndex, uint8_t state );
void board_tpx2_set_burn_en( TIMEPIX_ID tpxIndex, uint8_t state );
void board_tpx2_set_hv_monitor( TIMEPIX_ID tpxIndex, uint8_t state );

// GPIO functions SPX3
void board_spx3_set_pwr_en1v8(SPACEPIX_ID spxIndex, uint8_t state);
void board_spx3_set_chip_select(SPACEPIX_ID spxIndex, uint8_t state);
//void board_spx3_set_mode_select(SPACEPIX_ID spxIndex, SPACEPIX_READOUT_MODE state);
void board_spx3_set_inj_en(SPACEPIX_ID spxIndex, uint8_t state);
void board_spx3_set_cnt_en(SPACEPIX_ID spxIndex, uint8_t state);
void board_spx3_set_clr_n(SPACEPIX_ID spxIndex, uint8_t state);
void board_spx3_set_row_shift(SPACEPIX_ID spxIndex, uint8_t state);
void board_spx3_set_cnf_en(SPACEPIX_ID spxIndex, uint8_t state);
void board_spx3_set_en_hv(SPACEPIX_ID spxIndex, uint8_t state);
uint8_t board_spx3_get_temp(SPACEPIX_ID spxId, float *data);
uint8_t board_spx3_get_analog_out(SPACEPIX_ID spxId, float *data);
void board_spx3_set_oe(SPACEPIX_ID spxIndex, uint8_t state);


uint8_t read_ad799x_raw(AD_799x_ADDR address, AD_799x_CH channel, uint16_t *adcdata);
void convert_adc_raw(uint16_t adc_raw, float *voltage);
uint8_t write_ltc2635(LTC2635_CH channel, uint16_t data);
uint8_t read_tmp112x_raw(TMP112D_ADDR address, uint16_t *adcdata);
void convert_temp_raw(uint16_t temp_raw, float *temperature);

uint8_t test_AD_DAC(void);
void board_scan_pwm(void);

// Comm. function
uint8_t board_spi_transmit( TIMEPIX_ID tpxIndex, uint8_t *txbuf, uint16_t len );
uint8_t board_spi_receive( TIMEPIX_ID tpxIndex, uint8_t *rxbuf, uint16_t len );
uint8_t board_spi_transmit_receive( TIMEPIX_ID tpxIndex, uint8_t *txbuf, uint8_t *rxbuf, uint16_t len );
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi);
void process_shutter(uint32_t time);
void process_shutter_spx3(uint32_t time);
void process_shutter_spx3_inj(uint32_t time, uint16_t inject);
uint8_t SPI_IsActiveFlag_TXC (SPI_TypeDef *SPIx);
uint8_t SPI_IsActiveFlag_TXP(SPI_TypeDef *SPIx);

// Setup function

// start of clock : MCLOCK. Clock for ToA measurement
uint8_t board_start_mclock(TOT_FREQ tot);

// start of clock : MCLOCK. Clock for ToA measurement
uint8_t board_stop_mclock();

// set HV based on hexadecimal value
uint8_t board_tpx2_set_HV_hex(TIMEPIX_ID tpxId, uint8_t value);

// set HV based on float value -> function searching for closest number which corresponds to set voltage
uint8_t board_tpx2_set_HV(TIMEPIX_ID tpxId, float value);

// measure HV using ADC in MCU
uint8_t board_tpx2_get_HV(TIMEPIX_ID tpxId, float *data);

// get temp. from TMP100 sensor
uint8_t board_tpx2_get_temp(TIMEPIX_ID tpxId, float *data);

// measure voltage (DACOUT from TPX2) using ADC in MCU
uint8_t board_tpx2_get_dacout(TIMEPIX_ID tpxId, float *data);

// Delay
void Delay(uint32_t Delay);

// Some help function // based on Milan "specification"
uint8_t BoardGetBiasVoltage(float *bias);
uint8_t BoardSetBiasVoltage(float voltage);
uint8_t BoardGetTemp(float *temp);

uint8_t BoardGetSensorTemp(float *temp);



#define SIZE_OF_HV_ARR			256
// HV output value corresponds to hex. value which was send into MAX1932
static float hv_value_arr[SIZE_OF_HV_ARR] =
{
//HV output [V]      hex.		     means
0,					// 0x00		// HV turn OFF
149.579,			// 0x01
148.828,			// 0x02
148.422,			// .
147.671,			// .
147.706,
147.341,
146.646,
146.292,
145.852,
145.021,
144.909,
143.804,
143.905,
143.256,
142.305,
141.975,
141.906,
141.252,
140.417,
140.201,
139.896,
138.929,
139.045,
138.355,
138.059,
137.349,
136.889,
136.799,
136.333,
135.567,
135.115,
134.517,
134.420,
133.538,
133.251,
132.628,
132.216,
131.863,
131.687,
130.834,
130.937,
129.974,
129.657,
129.155,
128.634,
128.451,
127.513,
127.117,
126.674,
126.131,
125.901,
125.768,
124.771,
124.604,
123.576,
123.678,
122.857,
122.436,
122.066,
121.791,
121.480,
120.810,
120.015,
119.993,
119.198,
118.168,
118.215,
118.238,
117.595,
117.259,
116.734,
116.098,
115.863,
115.116,
114.458,
114.035,
113.906,
113.073,
112.787,
112.582,
111.812,
111.221,
110.750,
110.257,
109.884,
109.610,
109.076,
108.754,
108.141,
107.610,
107.172,
106.548,
106.223,
105.931,
105.271,
105.055,
104.455,
103.971,
103.418,
102.962,
102.282,
102.262,
101.347,
101.024,
100.711,
100.130,
99.669,
99.131,
98.624,
98.351,
98.064,
97.236,
96.952,
96.478,
95.865,
95.624,
95.159,
94.326,
94.125,
93.919,
93.378,
92.669,
92.078,
91.422,
91.124,
90.827,
90.445,
89.783,
89.297,
89.181,
88.348,
88.149,
87.505,
87.349,
86.639,
86.088,
85.808,
85.193,
84.656,
84.461,
84.136,
83.487,
82.880,
82.385,
81.884,
81.384,
81.219,
80.618,
79.825,
79.533,
78.963,
78.713,
78.360,
77.664,
77.097,
76.328,
76.085,
75.643,
75.102,
74.921,
74.382,
73.965,
73.318,
72.926,
72.522,
71.951,
71.308,
71.098,
70.654,
70.228,
69.802,
69.038,
68.831,
68.236,
67.539,
67.250,
66.951,
66.410,
65.905,
65.297,
64.798,
64.665,
63.958,
63.659,
63.014,
62.624,
62.000,
61.728,
61.027,
60.604,
60.235,
59.595,
59.439,
58.990,
58.344,
58.000,
57.395,
57.129,
56.425,
55.861,
55.509,
55.237,
54.721,
54.341,
53.626,
53.231,
52.703,
52.277,
51.864,
51.300,
50.877,
50.533,
49.905,
49.647,
48.902,
48.677,
48.098,
47.524,
47.070,
46.590,
46.054,
45.754,
45.332,
44.763,
44.317,
43.918,
43.145,
42.819,
42.455,
41.822,
41.543,
41.046,
40.530,
40.137,
39.605,
39.192,
38.812,
38.093,
37.685,
37.330,
36.669,
36.299,
35.941,
35.396,
34.980,
34.429,
34.044,
33.415,
33.008,
32.490,
32.142,
31.577,
31.092,
30.789,				// 0xfe
30.191};			// 0xff

#if defined(BOARD_VA416X0)
static uint8_t CLK_VA_Init(void);			// Importatnt: set up speed of CLK
static uint8_t HAL_VA_Init(void);
static uint8_t SPI2_VA_Init(void);			// TPX2_TOP SPI
static uint8_t GPIO_VA_Init(void);			// TPX2_TOP SPI
static uint8_t UART2_VA_Init(void);
static void ConfigureEdac(void);		//Set up memory Error Detection and Correction (EDAC)
static void EnableWatchdog(void);
void GPIO_WritePin(VOR_GPIO_Type* GPIOx,uint16_t GPIO_Pin, GPIO_PinState PinState);
#endif

#endif /* INC_BOARD_H_ */
