#ifndef INC_SPX3_COMM_H_
#define INC_SPX3_COMM_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <user.h>

#include "board.h"
#include "pinout.h"
#include "debug.h"


// Default readout with 1 SpacePix is SPXA
typedef enum {
	SPXA = 0,		// SpacePix A
	SPXB = 1		// SpacePix B
} SPACEPIX_ID;

// Default readout with 1 SpacePix is SPXA
typedef enum {
	SPI_MODE =  0,		// SPI CMOS mode
	LVDS_MODE = 1		// "SPI" LVDS mode
} SPACEPIX_READOUT_MODE;


typedef enum {
	SPX_OK = 0,
	SPX_FAILED = 1,
	SPX_HDR_FAIL = 2,
	SPX_FIN_FAIL = 3,
} SPX3_RET_CODE;

typedef enum {
	TDAC_DEFAULT = 0,
	TDAC_MAX = 1,
	TDAC_MIN = 2,
} SPX3_TDAC;

typedef enum {
    VBP_CSA          		 	= 0,
    VBN_CSA          		 	= 1,
    VFB_CSA          		 	= 2,
    VBN_PDH          		 	= 3,
    VBP_HYST         		 	= 4,
    VBP_COMP         		 	= 5,
    VBN_TDAC         		 	= 6,
    VBP_LCC          		 	= 7,
    VTHR             		 	= 8,
    VIN_N            		 	= 9,
    BAL              		 	= 10,
    SAMPLE_LENGTH    		 	= 11,
    DRIVER_BIAS      		 	= 12,
    BUFFER_BIAS      		 	= 13,
    LVDS_CM          		 	= 14,
    LVDS_STRENGTH    		 	= 15,
    VBP_AMP          		 	= 16,
    VBN_AMP          		 	= 17,
    SF               		 	= 18,
    TAIL             		 	= 19,
    TEST             		    = 20,
    SPI_CLK_FE_SEL_DATA         = 21,
    BACKSIDE_DEBUG_EN_DATA      = 22,
    VREF_EN_DATA                = 23,
    BACKSIDE_LOW_LEAK_EN_DATA   = 24,
    BACKSIDE_INJECT_EN_DATA     = 25,
    ANALOG_OUT_0_EN_DATA        = 26,
    ANALOG_OUT_1_EN_DATA        = 27,
    ANALOG_OUT_2_EN_DATA        = 28,
    ANALOG_OUT_3_EN_DATA        = 29,
    BACKSIDE_EN_DATA            = 30,
    TEMP_SENS_EN_DATA           = 31,
    ADC_PIN_EN_DATA             = 32,
} SPX3_DACS;


typedef struct{
	uint8_t global_config[128];		// 1024 bits
	uint8_t global_config_default[128];		// 1024 bits
	uint8_t pixel_matrix_config_default[128*64];
	uint16_t feadbackSpx3Dacs[33];
	float spx3_temp ;
	float spx3_vssa ;
	float spx3_adc_in;
} SPX3_CONFIG;

#define SPX3_INIT
#define SPX3_DIGITAl_TEST
#define SPX3_CHIP_ID
//#define HV

#define SPX3_ROW_SIZE	64
#define SPX3_COL_SIZE	64
#define SPX3_PX_MATRIX_BLOCK_SIZE	128		// 64px*16bit = 1024b on one row -> 128B
#define N				20
#define SPX3_ADC_SIZE	32

#define VTHR_DEFAULT	0x200
#define TEST_DEFAULT	0x80

#define SET_BITS 0x80
#define DEF_VAL		0x8

#define NUM_OF_SPX3_DACS		33

uint8_t spx3_init_dig(void); 				// Init SPX3 (basic config after startup) and make DIG. test of SPX3
uint8_t spx3_test(void);					// First testing og Spacepix
uint8_t spx3_basic_config(void);			// Configuration of GPIO init voltage levels
uint8_t spx3_pixel_matrix_config(uint8_t *tx_buf, uint8_t *rx_buf);		// Pixel matrix configuration
uint8_t spx3_global_config(uint8_t *tx_buf, uint8_t *rx_buf);			// Global config, should be done after Pixel matrix configuration
void spx3_default_global_config(uint8_t *data);
void spx3_set_dac(uint8_t *data, SPX3_DACS dac, uint16_t value);
uint8_t spx3_data_readout(uint8_t *rx_buf, uint32_t acq_time);
uint8_t spx3_data_readout_inj(uint8_t *rx_buf, uint32_t acq_time, uint16_t inject);
uint8_t Spx3SetVthr(uint16_t dac_value);
uint8_t spx3_digital_test(void);
uint8_t spx3_set_tdac(uint8_t *tx_buf, SPX3_TDAC tdac);
void UpdateSpx3Dacs( uint16_t *spx3Dacs, uint8_t tpx2DacsLength);

void spx3_get_vssa(float *vssa);
void spx3_get_temp(float *vssa);
void spx3_data_readout_adcin(uint8_t *rx_buf, uint32_t acq_time, uint32_t adc_value);


// Define constants for the configuration values using typedef enum with specific values

#if 0
typedef enum {
    VBP_CSA_3_0           = 0x0,
    VBP_CSA_7_4           = 0x8,
    VBN_CSA_3_0           = 0x0,
    VBN_CSA_7_4           = 0x8,
    VFB_CSA_3_0           = 0x0,
    VFB_CSA_7_4           = 0x8,
    VBN_PDH_3_0           = 0x0,
    VBN_PDH_7_4           = 0x8,
    VBP_HYST_3_0          = 0x0,
    VBP_HYST_7_4          = 0x8,
    VBP_COMP_3_0          = 0x0,
    VBP_COMP_7_4          = 0x8,
    VBN_TDAC_3_0          = 0x0,
    VBN_TDAC_7_4          = 0x8,
    VBP_LCC_3_0           = 0x0,
    VBP_LCC_7_4           = 0x8,
    VTHR_4_0              = 0x0,
    VTHR_9_5              = 0x8,
    VIN_N_4_0             = 0x0,
    VIN_N_9_5             = 0x8,
    BAL_4_0               = 0x0,
    BAL_9_5               = 0x8,
    SAMPLE_LENGTH_3_0     = 0x0,
    SAMPLE_LENGTH_7_4     = 0x8,
    DRIVER_BIAS_3_0       = 0x0,
    DRIVER_BIAS_7_4       = 0x8,
    BUFFER_BIAS_3_0       = 0x0,
    BUFFER_BIAS_7_4       = 0x8,
    LVDS_CM_3_0           = 0x0,
    LVDS_CM_7_4           = 0x8,
    LVDS_STRENGTH_3_0     = 0x0,
    LVDS_STRENGTH_7_4     = 0x8,
    VBP_AMP_3_0           = 0x0,
    VBP_AMP_7_4           = 0x8,
    VBN_AMP_3_0           = 0x0,
    VBN_AMP_7_4           = 0x8,
    SF_3_0                = 0x0,
    SF_7_4                = 0x8,
    TAIL_3_0              = 0x0,
    TAIL_7_4              = 0x8,
    TEST_3_0              = 0x0,
    TEST_7_4              = 0x8,
    SPI_CLK_FE_SEL        = 0x0,
    BACKSIDE_DEBUG_EN     = 995,
    VREF_EN               = 996,
    BACKSIDE_LOW_LEAK_EN  = 997,
    BACKSIDE_INJECT_EN    = 998,
    ANALOG_OUT_0_EN       = 1008,
    ANALOG_OUT_1_EN       = 1009,
    ANALOG_OUT_2_EN       = 1010,
    ANALOG_OUT_3_EN       = 1011,
    BACKSIDE_EN           = 1012,
    TEMP_SENS_EN          = 1013,
    ADC_PIN_EN            = 1014
} SPX_GLOBAL_CONFIG;
#endif
// Define constants for the data values using typedef enum

// ANALOG_OUT_x_EN: 1: EN, 0: DIS // Def. : 0
#define ANALOG_OUT_0_EN		0
#define ANALOG_OUT_1_EN		0
#define ANALOG_OUT_2_EN		0
#define ANALOG_OUT_3_EN		0

#define BACKSIDE_EN			0	// Def.: 0
#define TEMP_SENS_EN		0   // Def.: 0
#define ADC_PIN_EN			0	// Def.: 0

#define SPI_CLK_FE_SEL		0
#define BACKSIDE_DEBUG_EN	0
#define VREF_EN				1
#define BACKSIDE_LOW_LEAK_EN 0
#define BACKSIDE_INJECT_EN  0





#endif /* INC_SPX3_COMM_H_ */
