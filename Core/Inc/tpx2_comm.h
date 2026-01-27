/*
 * tpx2_comm.h
 *
 *  Created on: Sep 2, 2022
 *      Author: opavl
 */

#ifndef INC_TPX2_COMM_H_
#define INC_TPX2_COMM_H_

#define SIZE_OF_COUNTER_AB			10
#define SIZE_OF_COUNTER_CD			4
#define SIZE_OF_COUNTER_AB_BYTES	81920
#define SIZE_OF_COUNTER_CD_BYTES	32768
#define SIZE_OF_MATRIX				65536
#define SIZE_OF_COL					256
#define SIZE_OF_ROW					256
#define LENGHT_COUNTER_AB			655360
#define LENGHT_COUNTER_CD			262144
#define BUF_SIZE_AB					320
#define BUF_SIZE_CD					128
#define SIZE_OF_4BIT_COUNTER		32768

#define TPX2_PKT_COM_OFFSET			0
#define TPX2_PKT_COM_LENGTH			2

#define TPX2_PKT_DTAT_OFFSET		6

#define TPX2_PKT_WAIT_LENGTH		1
#define TPX2_PKT_CMD_SET_PAYLOAD_OFFSET		(TPX2_PKT_COM_LENGTH + TPX2_PKT_WAIT_LENGTH)

#define TPX2_PKT_ECHO_COM_OFFSET	(TPX2_PKT_COM_LENGTH + TPX2_PKT_WAIT_LENGTH)
#define TPX2_PKT_ECHO_COM_LENGTH	2
#define TPX2_PKT_ACK_OFFSET			(TPX2_PKT_ECHO_COM_OFFSET + TPX2_PKT_ECHO_COM_LENGTH)
#define TPX2_PKT_ACK_LENGTH			1
#define TPX2_PKT_CMD_GET_REGISTER_OFFSET	(TPX2_PKT_ACK_OFFSET + TPX2_PKT_ACK_LENGTH)
#define TPX2_PKT_FIN_LENGTH			1

#define TPX2_PKT_PAYLOAD8_LENGTH	1
#define TPX2_PKT_PAYLOAD16_LENGTH	2
#define TPX2_PKT_PAYLOAD24_LENGTH	3
#define TPX2_PKT_PAYLOAD32_LENGTH	4
#define TPX2_PKT_PAYLOAD256_LENGTH  32

// CMD - SET
#define TPX2_PTK_CMD_SET_NOPAYLOAD_LENGTH	(TPX2_PKT_COM_LENGTH + TPX2_PKT_WAIT_LENGTH + TPX2_PKT_ECHO_COM_LENGTH + TPX2_PKT_ACK_LENGTH+ TPX2_PKT_FIN_LENGTH)
#define TPX2_PKT_CMD_SET_COUNTER		(TPX2_PKT_COM_LENGTH + TPX2_PKT_WAIT_LENGTH)
#define TPX2_PKT_CMD_SET_8_LENGTH		(TPX2_PTK_CMD_SET_NOPAYLOAD_LENGTH + TPX2_PKT_PAYLOAD8_LENGTH)
#define TPX2_PKT_CMD_SET_16_LENGTH		(TPX2_PTK_CMD_SET_NOPAYLOAD_LENGTH + TPX2_PKT_PAYLOAD16_LENGTH)
#define TPX2_PKT_CMD_SET_32_LENGTH		(TPX2_PTK_CMD_SET_NOPAYLOAD_LENGTH + TPX2_PKT_PAYLOAD32_LENGTH)
#define TPX2_PKT_CMD_SET_256_LENGTH		(TPX2_PTK_CMD_SET_NOPAYLOAD_LENGTH + TPX2_PKT_PAYLOAD256_LENGTH)
// CMD - GET
#define TPX2_PTK_CMD_GET_NOPAYLOAD_LENGTH	(TPX2_PKT_COM_LENGTH + TPX2_PKT_WAIT_LENGTH + TPX2_PKT_ECHO_COM_LENGTH + TPX2_PKT_ACK_LENGTH + TPX2_PKT_FIN_LENGTH)
#define TPX2_PKT_CMD_GET_HDR_LENGTH		(TPX2_PKT_COM_LENGTH + TPX2_PKT_WAIT_LENGTH + TPX2_PKT_ECHO_COM_LENGTH + TPX2_PKT_ACK_LENGTH)
#define TPX2_PKT_CMD_GET_8_LENGTH		(TPX2_PTK_CMD_GET_NOPAYLOAD_LENGTH + TPX2_PKT_PAYLOAD8_LENGTH)
#define TPX2_PKT_CMD_GET_16_LENGTH		(TPX2_PTK_CMD_GET_NOPAYLOAD_LENGTH + TPX2_PKT_PAYLOAD16_LENGTH)
#define TPX2_PKT_CMD_GET_24_LENGTH		(TPX2_PTK_CMD_GET_NOPAYLOAD_LENGTH + TPX2_PKT_PAYLOAD24_LENGTH)
#define TPX2_PKT_CMD_GET_32_LENGTH		(TPX2_PTK_CMD_GET_NOPAYLOAD_LENGTH + TPX2_PKT_PAYLOAD32_LENGTH)
#define TPX2_PKT_CMD_GET_256_LENGTH		(TPX2_PTK_CMD_GET_NOPAYLOAD_LENGTH + TPX2_PKT_PAYLOAD256_LENGTH)
#define TPX2_PKT_CMD_GET_2_LENGTH		2

static uint8_t spi_dummy_rxbuf[256] = {0};
static uint8_t spi_dummy_txbuf[256] = {0};

#define DEFAULT_OMR			0x17C

#define SAFE_FREE( x )		{ free( x ); x = NULL; }

#ifdef ENABLE_TPX_LOG
	#define tpx_log_debug(...)   	log_debug(__VA_ARGS__)
	#define tpx_log_error(...)		log_error(__VA_ARGS__)
	#define tpx_log_warning(...)	log_warning(__VA_ARGS__)
#else
	#define tpx_log_debug(...)
	#define tpx_log_error(...)
	#define tpx_log_warning(...)
#endif

// define config of tpx2 connect chips
#define TPX2_TOP
//#define TPX2_BOT
//#define TPX2_MATRIX_READ
//#define TPX2_TOP_HV_EN
//#define TPX2_BOT_HV_EN
//#define TESTING
//#define READ_AD
//#define REG_TEST
#define TPX2_INIT
#define DIGITAl_TEST
#define CHIP_ID
// Multi-layer
typedef enum {
	TPXA = 0,		// TPX2_TOP
	TPXB = 1		// TPX2_BOT
} TIMEPIX_ID;


typedef enum {
	TPX_OK = 0,
	TPX_FAILED = 1,
	TPX_HDR_FAIL = 2,
	TPX_FIN_FAIL = 3,
} TPX2_RET_CODE;

typedef struct {
	uint8_t VBIAS_SLVS;
	uint8_t VCM_SLVS;
} TIMEPIX2_CONFIG;

typedef struct {
	float VBIAS_SLVS;
	float VCM_SLVS;
} TIMEPIX2_DACOUT;

typedef enum {
	PATTERN_FF = 1,
	PATTERN_00 = 4,
	PATTERN_DEC = 2,
	PATTERN_INC = 3,
	PATTERN_AA	= 0,
	PATTERN_8 = 5,
} TEST_PATTERN_ID;

typedef enum {
	ZCS_A = 0,
	ZCS_B = 1,
	ZCS_C = 2,
	ZCS_D = 3,
} ZCS;

typedef enum {
	COUNTER_A = 0,
	COUNTER_B = 1,
	COUNTER_C = 2,
	COUNTER_D = 3,
} COUNTER;

// GET command for TIMEPIX2
typedef enum
{
	GET_OMR        			= 0xA0,
	GET_TOAFREQSEL 			= 0xA1,
	GET_DACOUTSEL  			= 0xA2,
	GET_NTESTPULSES 		= 0xA3,
	GET_TPLENGTH 			= 0xA4,
	GET_COLHITTHRESHOLD 	= 0xA5,
    GET_DIGPROBESEL_0 		= 0xA6,
	GET_DIGPROBESEL_1 		= 0xA7,
    GET_COLMASKREG 			= 0xA8,
	GET_CTPR 				= 0xA9,
	GET_COLHITREG_S0 		= 0xAA,
	GET_COLHITREG_S1 		= 0xAB,
    GET_COLHITCTR_S0 		= 0xAC,
	GET_COLHITCTR_S1 		= 0xAD,
	GET_SHUTTIMEREG_S0 		= 0xAE,
	GET_SHUTTIMEREG_S1 		= 0xAF,
	GET_VBIAS_PREAMP_ON 	= 0xB0,
	GET_VBIAS_PREAMP_OFF 	= 0xB1,
	GET_VBIAS_LS_ON 		= 0xB2,
	GET_VBIAS_LS_OFF 		= 0xB3,
	GET_VCASC_PREAMP 		= 0xB4,
	GET_VFBK 				= 0xB5,
	GET_VTHCOARSE 			= 0xB6,
	GET_VTHFINE 			= 0xB7,
	GET_VBIAS_IKRUM 		= 0xB8,
	GET_VBIAS_DISCPMOS 		= 0xB9,
	GET_VBIAS_DISCNMOS 		= 0xBA,
	GET_VCASC_DISC 			= 0xBB,
	GET_VBIAS_THS 			= 0xBC,
	GET_VGND 				= 0xBD,
	GET_VTPCOARSE 			= 0xBE,
	GET_VTPFINE 			= 0xBF,
	GET_VBIAS_SLVS			= 0xC8,
	GET_VCM_SLVS 			= 0xC9,
	GET_VBIAS_RES 			= 0xCA,
	GET_CHIPID 				= 0xC3,
	GET_CHIPIDADDR 			= 0xC4,
	GET_MASKZCS 			= 0xC5,
	GET_PREAMP_OUT_SEL 		= 0xC6,
	GET_COUNTER_A			= 0xE0,
	GET_COUNTER_B			= 0xE1,
	GET_COUNTER_C			= 0xE2,
	GET_COUNTER_D			= 0xE3,
	GET_COUNTER_A_ZCS		= 0xE8,
	GET_COUNTER_B_ZCS		= 0xE9,
	GET_COUNTER_C_ZCS		= 0xEA,
	GET_COUNTER_D_ZCS		= 0xEB,
	GET_CONF				= 0xD8,
	GET_TRIM				= 0xD9,
	GET_COUNTER_A_DIGPIX	= 0xEC,
	GET_COUNTER_B_DIGPIX	= 0xED,
	GET_COUNTER_C_DIGPIX	= 0xEE,
	RESET_COUNTER_A			= 0xFA,
	RESET_COUNTER_B			= 0xFB,
	RESET_COUNTER_C			= 0xFC,
	RESET_COUNTER_D			= 0xFD,
	GET_COUNTER_D_DIGPIX	= 0xEF,
	GET_VDD23				= 0x14,
	GET_VDD13				= 0x13,
	GET_VDDA23				= 0x12,
	GET_BIAS_DAC			= 0x10,
	GET_BIAS_DAC_CAS		= 0x10,
	GET_VBG					= 0x0E,
	GET_VBG_TEMP			= 0x0D
} TPX2_GET_CMD;

// SET command for TIMEPIX2
typedef enum
{
	SET_OMR					= 0x80,
	SET_TOAFREQSEL			= 0x81,
	SET_DACOUTSEL			= 0x82,
	SET_NTESTPULSES			= 0x83,
	SET_TPLENGTH			= 0x84,
	SET_COLHITTHRESHOLD		= 0x85,
	SET_DIGPROBESEL_0		= 0x86,
	SET_DIGPROBESEL_1		= 0x87,
	SET_VBIAS_PREAMP_ON		= 0x90,
	SET_VBIAS_PREAMP_OFF	= 0x91,
	SET_VBIAS_LS_ON			= 0x92,
	SET_VBIAS_LS_OFF		= 0x93,
	SET_VCASC_PREAMP		= 0x94,
	SET_VFBK				= 0x95,
	SET_VTHCOARSE 			= 0x96,
	SET_VTHFINE				= 0x97,
	SET_VBIAS_IKRUM			= 0x98,
	SET_VBIAS_DISCPMOS		= 0x99,
	SET_VBIAS_DISCNMOS		= 0x9A,
	SET_VCASC_DISC			= 0x9B,
	SET_VBIAS_THS			= 0x9C,
	SET_TRIM				= 0xD1,
	SET_CONF				= 0xD0,
	SET_VGND				= 0x9D,
	SET_VTPCOARSE			= 0x9E,
	SET_VTPFINE				= 0x9F,
	SET_VBIAS_SLVS			= 0x88,
	SET_VCM_SLVS			= 0x89,
	SET_VBIAS_RES			= 0x8A,
	SET_CHIPIDADDR			= 0xC0,
	SET_MASKZCS				= 0xC1,
	SET_PREAMP_OUT_SEL		= 0xC2,
	SET_COUNTER_A			= 0xF6,
	SET_COUNTER_B			= 0xF7,
	SET_COUNTER_C			= 0xF8,
	SET_COUNTER_D			= 0xF9
} TPX2_SET_CMD;

typedef enum{
      VBIAS_PREAMP_ON 	= 0x00,		// 0
      VBIAS_PREAMP_OFF 	= 0x15,		// 1
      VBIAS_LS_ON 		= 0x17,		// 2
	  VBIAS_LS_OFF 		= 0x18,		// 3
	  VCASC_PREAMP 		= 0x06,		// 4
	  VFBK 				= 0x07,		// 5
	  VTHCOARSE 		= 0x08,		// 6
	  VTHFINE 			= 0x08,		// 7
	  VBIAS_IKRUM 		= 0x01,		// 8
	  VBIAS_DISCPMOS 	= 0x16,		// 9
	  VBIAS_DISCNMOS 	= 0x02,		// 10
	  VCASC_DIS 		= 0x09,		// 11
	  VBIAS_THS			= 0x03,		// 12
	  VGND 				= 0x0A,		// 13
	  VTPCOARSE 		= 0x04,		// 14
	  VTPFINE 			= 0x05,		// 15
	  VBIAS_SLVLS 		= 0x0C,		// 16
	  VCM_SLVS 			= 0x0B,		// 17
	  VBIAS_RES			= 0xFF,		// 18
	 // EXTERNAL DACS
	  VDD23 			= 0x14,		// 19
	  VDD13 			= 0x13,		// 20
	  VDDA23 			= 0x12,		// 21
	  VDDA13 			= 0x11,		// 22
	  BIAS_DAC 			= 0x10,		// 23
	  BIAS_DAC_CAS 		= 0x0F,		// 24
	  VBG 				= 0x0E,		// 25
	  VBG_TEMP 			= 0x0D		// 26
} TPX2_DACOUTSEL;		// DACOUTSEL CODE for choosing what will be on DACOUT


typedef enum{
   Divide1 = 0x00,    /// f(ToA) = f(ToT) / 2^1
   Divide2 = 0x01,    /// f(ToA) = f(ToT) / 2^2
   Divide3 = 0x02,    /// f(ToA) = f(ToT) / 2^3
   Divide4 = 0x03,    /// f(ToA) = f(ToT) / 2^4
   Divide5 = 0x04,    /// f(ToA) = f(ToT) / 2^5
   Divide6 = 0x05,    /// f(ToA) = f(ToT) / 2^6
   Divide7 = 0x06,    /// f(ToA) = f(ToT) / 2^7
   Divide8 = 0x07,    /// f(ToA) = f(ToT) / 2^8
   Divide9 = 0x08,    /// f(ToA) = f(ToT) / 2^9
   Divide10 = 0x09,   /// f(ToA) = f(ToT) / 2^10
   Divide11 = 0x0A,   /// f(ToA) = f(ToT) / 2^11
   Divide12 = 0x0B,   /// f(ToA) = f(ToT) / 2^12
   Divide13 = 0x0C,   /// f(ToA) = f(ToT) / 2^13
   Divide14 = 0x0D,   /// f(ToA) = f(ToT) / 2^14
   Divide15 = 0x0E,   /// f(ToA) = f(ToT) / 2^15
   Divide16 = 0x0F,   /// f(ToA) = f(ToT) / 2^16
   Divide17 = 0x10,   /// f(ToA) = f(ToT) / 2^17
   Divide18 = 0x11,   /// f(ToA) = f(ToT) / 2^18
   Divide19 = 0x12,   /// f(ToA) = f(ToT) / 2^19
   Divide20 = 0x13,   /// f(ToA) = f(ToT) / 2^20
   Divide21 = 0x14,   /// f(ToA) = f(ToT) / 2^21
   Divide22 = 0x15,   /// f(ToA) = f(ToT) / 2^22
   Divide23 = 0x16,   /// f(ToA) = f(ToT) / 2^23
   Divide24 = 0x17,   /// f(ToA) = f(ToT) / 2^24
   Divide25 = 0x18,   /// f(ToA) = f(ToT) / 2^25
   Divide26 = 0x19,   /// f(ToA) = f(ToT) / 2^26
   Divide27 = 0x1A,   /// f(ToA) = f(ToT) / 2^27
   Divide28 = 0x1B,   /// f(ToA) = f(ToT) / 2^28
   Divide29 = 0x1C,   /// f(ToA) = f(ToT) / 2^29
   Divide30 = 0x1D,   /// f(ToA) = f(ToT) / 2^30
   ToaBypass = 0x1E,  /// f(ToA) = f(ToT)
   ToaDisable = 0x1F, /// f(ToA) = 0 Hz
 } ToaFrequency;
//////////////////////////////////////////////////////////////////////////////////////////
typedef enum {
	VbiasPreampOn = 0,
	VbiasPreampOff  = 1,
	VbiasLsOn  = 2,
	VbiasLsOff  = 3,
	VcascPreamp  = 4,
	Vfbk  = 5,
	VthCoarse  = 6,
	VthFine  = 7,
	VbiasIkrum  = 8,
	VbiasDiscPmos  = 9,
	VbiasDiscNmos  = 10,
	VcascDisc  = 11,
	VbiasThs  = 12,
	Vgnd  = 13,
	VtpCoarse  = 14,
	VtpFine  = 15,
	VbiasSlvs  = 16,
	VcmSlvs  = 17,
	BandGap  = 18
} TPX2_DACS_INDEX;

typedef struct{
     uint16_t vbias_preamp_on;		// 0
     uint16_t vbias_preamp_off;		// 1
     uint16_t vbias_ls_on;			// 2
     uint16_t vbias_ls_off;			// 3
     uint16_t vcasc_preamp;			// 4
     uint16_t vfbk;					// 5
     uint16_t vthcoarse;			// 6
     uint16_t vthfine;				// 7
     uint16_t vbias_ikrum;			// 8
     uint16_t vbias_discpmos;		// 9
     uint16_t vbias_discnmos;		// 10
     uint16_t vcasc_dis;			// 11
     uint16_t vbias_ths;			// 12
     uint16_t vgnd;					// 13
     uint16_t vtpcoarse;			// 14
     uint16_t vtpfine;				// 15
     uint16_t vbias_slvls;			// 16
     uint16_t vcm_slvs;				// 17
     uint16_t vbias_res;			// 18 -> Not USE
	 // external TPX2 dacs
     uint16_t vdd23;				// 19
     uint16_t vdd13;				// 20
     uint16_t vdda23;				// 21
	 uint16_t vdda13;				// 22
	 uint16_t bias_dac;				// 23
	 uint16_t bias_dac_cas;			// 24
	 uint16_t vbg;					// 25	// band_gap
	 uint16_t vbg_temp;				// 26
} TPX2_DACS;		// store HEX value of DAC

typedef struct{
     float vbias_preamp_on;			// 0
     float vbias_preamp_off;		// 1
     float vbias_ls_on;				// 2
     float vbias_ls_off;			// 3
     float vcasc_preamp;			// 4
     float vfbk;					// 5
     float vthcoarse;				// 6
     float vthfine;					// 7
     float vbias_ikrum;				// 8
	 float vbias_discpmos;			// 9
	 float vbias_discnmos;			// 10
	 float vcasc_dis;				// 11
	 float vbias_ths;				// 12
	 float vgnd;					// 13
	 float vtpcoarse;				// 14
	 float vtpfine;					// 15
	 float vbias_slvls;				// 16
	 float vcm_slvs;				// 17
	 float vbias_res;				// 18
	 // external dacs
	 float vdd23;					// 19
	 float vdd13;					// 20
	 float vdda23;					// 21
	 float vdda13;					// 22
	 float bias_dac;				// 23
	 float bias_dac_cas;			// 24
	 float vbg;						// 25
	 float vbg_temp;				// 26
} TPX2_DACS_VALUE;	// sore REAL value of DAC



#define NUM_OF_DACS		27
static uint8_t dacsScanList[NUM_OF_DACS] = {		// Save CMD for coresponding DACOUTSEL code
		0x00, //VBIAS_PREAMP_ON		// 0
		0x15, //VBIAS_PREAMP_OFF	// 1
		0x17, //VBIAS_LS_ON			// 2
		0x18, //VBIAS_LS_OFF		// 3
		0x06, //VCASC_PREAMP		// 4
		0x07, //VFBK				// 5
		0x08, //VTHCOARSE			// 6
		0x08, //VTHFINE				// 7
		0x01, //VBIAS_IKRUM			// 8
		0x16, //VBIAS_DISCPMOS		// 9
		0x02, //VBIAS_DISCNMOS		// 10
		0x09, //VCASC_DIS			// 11
		0x03, //VBIAS_THS			// 12
		0x0A, //VGND				// 13
		0x04, //VTPCOARSE			// 14
		0x05, //VTPFINE				// 15
		0x0C, //VBIAS_SLVLS			// 16
		0x0B, //VCM_SLVS			// 17
		0xFF, //VBIAS_RES !! N/A	// 18
		0x14, //VDD23				// 19
		0x13, //VDD13				// 20
		0x12, //VDDA23				// 21
		0x11, //VDDA13				// 22
		0x10, //BIAS_DAC			// 23
		0x0F, //BIAS_DAC_CAS		// 24
		0x0E, //VBG					// 25
		0x0D  //VBG_TEMP			// 26
};

static uint8_t internal_dacsScanList[] = {		// Save CMD for coresponding DACOUTSEL code
		0x00, //VBIAS_PREAMP_ON
		0x15, //VBIAS_PREAMP_OFF
		0x17, //VBIAS_LS_ON
		0x18, //VBIAS_LS_OFF
		0x06, //VCASC_PREAMP
		0x07, //VFBK
		0x08, //VTHCOARSE
		0x08, //VTHFINE
		0x01, //VBIAS_IKRUM
		0x16, //VBIAS_DISCPMOS
		0x02, //VBIAS_DISCNMOS
		0x09, //VCASC_DIS
		0x03, //VBIAS_THS
		0x0A, //VGND
		0x04, //VTPCOARSE
		0x05, //VTPFINE
		0x0C, //VBIAS_SLVLS
		0x0B, //VCM_SLVS
		0xFF //VBIAS_RES		!! N/A
};

typedef enum {
    Frequency120 = 0, /// f(ToT) = 120 MHz
    Frequency100 = 1, /// f(ToT) = 100 MHz
    Frequency80 = 2,  /// f(ToT) = 80 MHz
    Frequency50 = 3,  /// f(ToT) = 50 MHz
    Frequency25 = 4,  /// f(ToT) = 25 MHz
    Frequency10 = 5,  /// f(ToT) = 10 MHz
    Frequency5 = 6,   /// f(ToT) = 5 MHz
	Frequency_HV_150V = 7,/// f -> ?
    TotDisable = 8,    /// f(ToT) = 0 Hz
} TOT_FREQ;

typedef struct{
	uint32_t chipId;
	uint8_t chipConfig[256*256];
	TPX2_DACS dacs;
	float temperature;
} TPX2_CONFIG;




// TPX2 INIT
// Try to inti TPX2 -> make GR and SET VCM_SLVS, VBIAS_SLVS register
int tpx2_init(TIMEPIX_ID tpxId, TIMEPIX2_CONFIG *timepix2Cfg);
uint8_t tpx2_init_dig(void); // Init TPX2 (basic config after startup) and make DIG. test of TPX2
void tpx2_basic_configuration( TIMEPIX_ID tpxId );

// TPX2 powerstate
void tpx2_set_powerstate( TIMEPIX_ID tpxId, uint8_t state );

// TPX2 SET
//function setting basic registers (8b, 16b, 32b, 256b)
uint8_t transmit_receive_set(TIMEPIX_ID tpxId, uint8_t cmd, uint8_t *data, int sizeof_payload );
uint8_t tpx2_set_reg_8b(TIMEPIX_ID tpxId, TPX2_SET_CMD cmd, uint8_t *data);
uint8_t tpx2_set_reg_16b(TIMEPIX_ID tpxId, TPX2_SET_CMD cmd, uint16_t *data);
uint8_t tpx2_set_reg_32b(TIMEPIX_ID tpxId, TPX2_SET_CMD cmd, uint32_t *data);
uint8_t tpx2_set_reg256b(TIMEPIX_ID tpxId, TPX2_SET_CMD cmd, uint8_t *data); // better to use setcounter function?

// function for write data into pixel matrix
/* tpxID -> TPXA
 * cmd -> SET_COUNTER_A, or B or C or D
 * buffer -> data which will be write
 * length -> length of COUNTER in bytes
 */
uint8_t tpx2_setcounter( TIMEPIX_ID tpxId, uint8_t cmd, uint8_t *buffer, uint32_t length);

// TPX2 GET
//function setting basic registers (8b, 16b, 32b, 256b)
uint8_t transmit_receive_get( TIMEPIX_ID tpxId, uint8_t cmd, uint32_t *reg_value, int sizeof_payload);
uint8_t tpx2_get_reg_8b(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint32_t *reg_value);
uint8_t tpx2_get_reg_16b(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint32_t *reg_value);
uint8_t tpx2_get_reg_24b(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint32_t *reg_value);
uint8_t tpx2_get_reg_32b(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint32_t *reg_value);
uint8_t tpx2_get_reg_256b(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd);	// better to use setcounter function?
// act_zcs store info about active column in zcs mode
uint8_t tpx2_get_reg_256b_ZCS(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint8_t *buffer, uint8_t *act_zcs);


// read raw chip id
uint8_t tpx2_get_chip_id(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint32_t *reg_value);

// function for write data into pixel matrix
/* tpxID -> TPXA
 * cmd -> GET_COUNTER_A, or B or C or D
 * buffer -> there will be store read out data
 * length -> length of COUNTER in bytes
 */
uint8_t tpx2_getcounter( TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint8_t *buffer, uint16_t column);
uint8_t tpx2_getcounter_ZCS( TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint8_t *buffer, uint32_t length);

// get temp. of chip TPX2 // based on readout DACS: VTEMP, VBG
uint8_t tpx2_get_temp(TIMEPIX_ID tpxId, float *data);


// DIGITAL TEST
/*
 * SET test value into whole pixel matrix (COUNTER A,B,C,D)
 * patter : 1) 0xff
 * 			2) 0x00
 * 			3) incremental
 * 			4) decremental
 * 			5) 0xAA
 *
 * 	if SET_value = GET_value -> DIGITAL TEST PASS
 */
uint8_t tpx2_digital_test( TIMEPIX_ID tpxId );

// HELP FUNCTION

// return size of counter in bites
uint32_t GetCounterBitSize( TPX2_GET_CMD cmd );

// extract value from get cmd
uint32_t tpx2_extract_value_from_get_cmd( uint8_t *data, uint8_t sizeof_payload);

// generate incremental pattern
uint8_t pattern_inc( uint32_t index );

// Decode CHIPID from RAW eFuseValue into stringId
void DecodeChipId( uint32_t eFuseValue, char *stringId );

// return length of data to read or write in one spi transaction
uint16_t get_size_of_buf(uint8_t cmd);

// transform COUNTER_C,D
/*
 * buffer -> RAW matrix of read out pixels from Timepix
 *  * transform_buffer -> reorganized buffer with real value (done transform from pseudorandom number into real value)
 * transform_buffer -> reorganized buffer
 * */
uint8_t transform_4bit_matrix_get(uint8_t *buffer,  uint8_t *transform_buffer);	// TODO poradna kontrola, melo by byt ale ok
uint8_t transform_4bit_matrix_get_optim(uint8_t *buffer, uint8_t *out_buf);


// transform COUNTER_A,B
/*
 * buffer -> RAW matrix of read out pixels from Timepix
 * transform_buffer -> reorganized buffer with real value (done transform from pseudorandom number into real value)
 * */
uint8_t transform_10bit_matrix_get(uint8_t *buffer, uint16_t *transform_buffer);		// TODO poradna kontrola, melo by byt ale ok
uint8_t transform_10bit_matrix_get_optim(uint8_t *buffer, uint16_t *out_buf);


// buffer = raw, set_buf = transform
uint8_t transform_set_counterAB(uint8_t *buffer, uint8_t *set_buf);		// TODO poradna kontrola, melo by byt ale ok
uint8_t transform_set_counterCD(uint8_t *buffer,  uint8_t *transform_buffer);		// TODO poradna kontrola, melo by byt ale ok


// Function to set the k-th bit of n
uint8_t setBit(uint32_t n, uint32_t k);


// Some help function // based on Milan "specification"
uint8_t Tpx2GetChipId(uint32_t *chipId);

uint8_t Tpx2ScanDacs(float *tpx2Dacs);

uint8_t Tpx2WriteDacToRegisterArray(uint16_t *dacArrayReg, uint8_t regIndex, uint16_t regData);
uint16_t Tpx2ReadDacFromRegisterArray( uint16_t *dacArrayReg, uint8_t regIndex );
uint8_t Tpx2SetDac( TPX2_SET_CMD dacChannel, uint16_t dacValue );

// GET HEX value of dac's registers
uint8_t Tpx2GetDac( TPX2_SET_CMD dacChannel, uint8_t index);
void UpdateTpx2Dacs( float *tpx2Dacs, uint8_t tpx2DacsLength);
uint8_t Tpx2SenseDacSel(uint8_t dacIndex);
void set_data_mode(uint8_t datamode);	// TODO TEST
uint8_t pixel_matrix_load_counter10(uint32_t *pixel_matrix, uint16_t *buffer, TPX2_GET_CMD cmd);	// seems OK
uint8_t pixel_matrix_load_counter4(uint32_t *pixel_matrix, uint8_t *buffer, TPX2_GET_CMD cmd);		// seems OK
uint8_t pixel_matrix_load_counterAB(uint32_t *pixel_matrix, uint16_t *buffer, COUNTER cmd);
uint8_t pixel_matrix_load_counterCD(uint32_t *pixel_matrix, uint8_t *buffer, COUNTER cmd);

// GET COUNTER help function, CounterX -> counter that will be read from Timepix
void tpx2_getcounterA(uint8_t *counterA);
void tpx2_getcounterB(uint8_t *counterB);
void tpx2_getcounterC(uint8_t *counterC);
void tpx2_getcounterD(uint8_t *counterD);

void tpx2_getcounterA_ZCS(uint8_t *counterA, uint16_t column);

// SET COUNTER help function, CounterX -> counter that will be write into Timepix
void tpx2_setcounterA(uint8_t *counterA);
void tpx2_setcounterB(uint8_t *counterB);
void tpx2_setcounterC(uint8_t *counterC);
void tpx2_setcounterD(uint8_t *counterD);

void tpx2_settrim(uint8_t *trim);	// SET TRIM
void tpx2_setconf(uint8_t *conf);	// SET CONF

void tpx2_gettrim(uint8_t *trim);	// GET TRIM
void tpx2_getconf(uint8_t *conf);	// GET CONF


// RESET COUNTER help function
void tpx2_reset_counterA(uint8_t *counterA);

uint8_t transform_set_counterAB_new(const uint8_t *bmc, uint8_t *trimStream);
uint8_t transform_set_counterCD_new(const uint8_t *bmc, uint8_t *confStream);

uint8_t tpx2_get_readready();

uint8_t tpx2_get_matrix_occ();

uint8_t count_active_column(uint16_t *active_column, uint8_t *zcs, uint8_t *act_zcs);

uint8_t ZCS_transform(TPX2_GET_CMD cmd, uint8_t *buffer, uint8_t *act_zcs, uint8_t *buffer_ZCS);

uint8_t zcs_number( TPX2_GET_CMD get_cmd);

uint8_t pattern_gen( TEST_PATTERN_ID testId, uint32_t index );

//look up table for Timepix2 4-bit counter value
static uint8_t LUT_4_bit[16] =
{
0, 		// 0
1,		// 1
2,		// 2
5,		// 3
3,		// 4
9,		// 5
6,		// 6
11,		// 7
15,		// 8
4,		// 9
8,		// 10
10,		// 11
14,		// 12
7,		// 13
13,		// 14
12};	// 15


/*look up table for Timepix2 10-bit counter value
 * 	  0				// 0
 * */
static uint16_t LUT_10_bit[1024] =
{0  ,		// 0
1   ,		// 1
2   ,		// .
78  ,		// .
3   ,
155 ,
79  ,
957 ,
4   ,
11  ,
156 ,
326 ,
80  ,
619 ,
958 ,
232 ,
5   ,
309 ,
12  ,
201 ,
157 ,
890 ,
327 ,
696 ,
81  ,
25  ,
620 ,
88  ,
959 ,
403 ,
233 ,
437 ,
6   ,
514 ,
310 ,
552 ,
13  ,
41  ,
202 ,
480 ,
158 ,
519 ,
891 ,
102 ,
328 ,
165 ,
697 ,
861 ,
82  ,
259 ,
26  ,
386 ,
621 ,
278 ,
89  ,
578 ,
960 ,
773 ,
404 ,
681 ,
234 ,
53  ,
438 ,
967 ,
7   ,
21  ,
515 ,
769 ,
311 ,
651 ,
553 ,
130 ,
14  ,
315 ,
42  ,
850 ,
203 ,
758 ,
481 ,
981 ,
159 ,
214 ,
520 ,
336 ,
892 ,
463 ,
103 ,
908 ,
329 ,
655 ,
166 ,
265 ,
698 ,
370 ,
862 ,
355 ,
83  ,
676 ,
260 ,
591 ,
27  ,
629 ,
387 ,
992 ,
622 ,
557 ,
279 ,
823 ,
90  ,
220 ,
579 ,
118 ,
961 ,
938 ,
774 ,
534 ,
405 ,
492 ,
682 ,
242 ,
235 ,
134 ,
54  ,
596 ,
439 ,
179 ,
968 ,
944 ,
1021,
8   ,
306 ,
22  ,
511 ,
516 ,
256 ,
770 ,
18  ,
312 ,
211 ,
652 ,
673 ,
554 ,
935 ,
131 ,
1018,
15  ,
1015,
316 ,
611 ,
43  ,
192 ,
851 ,
319 ,
204 ,
32  ,
759 ,
429 ,
482 ,
569 ,
982 ,
614 ,
160 ,
753 ,
215 ,
668 ,
521 ,
789 ,
337 ,
46  ,
893 ,
802 ,
464 ,
526 ,
104 ,
706 ,
909 ,
195 ,
330 ,
1009,
656 ,
643 ,
167 ,
297 ,
266 ,
854 ,
699 ,
634 ,
371 ,
900 ,
863 ,
780 ,
356 ,
322 ,
84  ,
98  ,
677 ,
846 ,
261 ,
819 ,
592 ,
207 ,
28  ,
798 ,
630 ,
794 ,
388 ,
728 ,
993 ,
35  ,
623 ,
662 ,
558 ,
421 ,
280 ,
835 ,
824 ,
762 ,
91  ,
392 ,
221 ,
927 ,
580 ,
452 ,
119 ,
432 ,
962 ,
350 ,
939 ,
564 ,
775 ,
447 ,
535 ,
485 ,
406 ,
732 ,
493 ,
342 ,
683 ,
715 ,
243 ,
572 ,
236 ,
291 ,
135 ,
413 ,
55  ,
141 ,
597 ,
985 ,
440 ,
997 ,
180 ,
811 ,
969 ,
540 ,
945 ,
1022,
153 ,
9   ,
617 ,
307 ,
888 ,
23  ,
401 ,
512 ,
39  ,
517 ,
163 ,
257 ,
276 ,
771 ,
51  ,
19  ,
649 ,
313 ,
756 ,
212 ,
461 ,
653 ,
368 ,
674 ,
627 ,
555 ,
218 ,
936 ,
490 ,
132 ,
177 ,
1019,
509 ,
16  ,
671 ,
1016,
609 ,
317 ,
427 ,
612 ,
666 ,
44  ,
524 ,
193 ,
641 ,
852 ,
898 ,
320 ,
844 ,
205 ,
792 ,
33  ,
419 ,
760 ,
925 ,
430 ,
562 ,
483 ,
340 ,
570 ,
411 ,
983 ,
809 ,
615 ,
399 ,
161 ,
49  ,
754 ,
366 ,
216 ,
175 ,
669 ,
425 ,
522 ,
896 ,
790 ,
923 ,
338 ,
807 ,
47  ,
173 ,
894 ,
805 ,
803 ,
871 ,
465 ,
873 ,
527 ,
284 ,
105 ,
467 ,
707 ,
737 ,
910 ,
875 ,
196 ,
381 ,
331 ,
529 ,
1010,
1004,
657 ,
286 ,
644 ,
839 ,
168 ,
107 ,
298 ,
67  ,
267 ,
469 ,
855 ,
112 ,
700 ,
709 ,
635 ,
61  ,
372 ,
739 ,
901 ,
828 ,
864 ,
912 ,
781 ,
498 ,
357 ,
877 ,
75  ,
323 ,
198 ,
85  ,
549 ,
99  ,
383 ,
678 ,
766 ,
847 ,
333 ,
262 ,
588 ,
820 ,
531 ,
593 ,
303 ,
208 ,
1012,
29  ,
750 ,
799 ,
1006,
631 ,
95  ,
795 ,
659 ,
389 ,
347 ,
729 ,
288 ,
994 ,
150 ,
36  ,
646 ,
624 ,
506 ,
663 ,
841 ,
559 ,
396 ,
422 ,
170 ,
281 ,
378 ,
836 ,
109 ,
825 ,
72  ,
763 ,
300 ,
92  ,
147 ,
393 ,
69  ,
222 ,
225 ,
928 ,
269 ,
581 ,
688 ,
453 ,
471 ,
120 ,
228 ,
433 ,
857 ,
963 ,
977 ,
351 ,
114 ,
940 ,
931 ,
565 ,
702 ,
776 ,
724 ,
448 ,
711 ,
536 ,
272 ,
486 ,
637 ,
407 ,
919 ,
733 ,
63  ,
494 ,
584 ,
343 ,
374 ,
684 ,
720 ,
716 ,
741 ,
244 ,
691 ,
573 ,
903 ,
237 ,
187 ,
292 ,
830 ,
136 ,
456 ,
414 ,
866 ,
56  ,
745 ,
142 ,
914 ,
598 ,
474 ,
986 ,
783 ,
441 ,
603 ,
998 ,
500 ,
181 ,
123 ,
812 ,
359 ,
970 ,
248 ,
541 ,
879 ,
946 ,
1023,
77  ,
154 ,
956 ,
10  ,
325 ,
618 ,
231 ,
308 ,
200 ,
889 ,
695 ,
24  ,
87  ,
402 ,
436 ,
513 ,
551 ,
40  ,
479 ,
518 ,
101 ,
164 ,
860 ,
258 ,
385 ,
277 ,
577 ,
772 ,
680 ,
52  ,
966 ,
20  ,
768 ,
650 ,
129 ,
314 ,
849 ,
757 ,
980 ,
213 ,
335 ,
462 ,
907 ,
654 ,
264 ,
369 ,
354 ,
675 ,
590 ,
628 ,
991 ,
556 ,
822 ,
219 ,
117 ,
937 ,
533 ,
491 ,
241 ,
133 ,
595 ,
178 ,
943 ,
1020,
305 ,
510 ,
255 ,
17  ,
210 ,
672 ,
934 ,
1017,
1014,
610 ,
191 ,
318 ,
31  ,
428 ,
568 ,
613 ,
752 ,
667 ,
788 ,
45  ,
801 ,
525 ,
705 ,
194 ,
1008,
642 ,
296 ,
853 ,
633 ,
899 ,
779 ,
321 ,
97  ,
845 ,
818 ,
206 ,
797 ,
793 ,
727 ,
34  ,
661 ,
420 ,
834 ,
761 ,
391 ,
926 ,
451 ,
431 ,
349 ,
563 ,
446 ,
484 ,
731 ,
341 ,
714 ,
571 ,
290 ,
412 ,
140 ,
984 ,
996 ,
810 ,
539 ,
152 ,
616 ,
887 ,
400 ,
38  ,
162 ,
275 ,
50  ,
648 ,
755 ,
460 ,
367 ,
626 ,
217 ,
489 ,
176 ,
508 ,
670 ,
608 ,
426 ,
665 ,
523 ,
640 ,
897 ,
843 ,
791 ,
418 ,
924 ,
561 ,
339 ,
410 ,
808 ,
398 ,
48  ,
365 ,
174 ,
424 ,
895 ,
922 ,
806 ,
172 ,
804 ,
870 ,
872 ,
283 ,
466 ,
736 ,
874 ,
380 ,
528 ,
1003,
285 ,
838 ,
106 ,
66  ,
468 ,
111 ,
708 ,
60  ,
738 ,
827 ,
911 ,
497 ,
876 ,
74  ,
197 ,
548 ,
382 ,
765 ,
332 ,
587 ,
530 ,
302 ,
1011,
749 ,
1005,
94  ,
658 ,
346 ,
287 ,
149 ,
645 ,
505 ,
840 ,
395 ,
169 ,
377 ,
108 ,
71  ,
299 ,
146 ,
68  ,
224 ,
268 ,
687 ,
470 ,
227 ,
856 ,
976 ,
113 ,
930 ,
701 ,
723 ,
710 ,
271 ,
636 ,
918 ,
62  ,
583 ,
373 ,
719 ,
740 ,
690 ,
902 ,
186 ,
829 ,
455 ,
865 ,
744 ,
913 ,
473 ,
782 ,
602 ,
499 ,
122 ,
358 ,
247 ,
878 ,
76  ,
955 ,
324 ,
230 ,
199 ,
694 ,
86  ,
435 ,
550 ,
478 ,
100 ,
859 ,
384 ,
576 ,
679 ,
965 ,
767 ,
128 ,
848 ,
979 ,
334 ,
906 ,
263 ,
353 ,
589 ,
990 ,
821 ,
116 ,
532 ,
240 ,
594 ,
942 ,
304 ,
254 ,
209 ,
933 ,
1013,
190 ,
30  ,
567 ,
751 ,
787 ,
800 ,
704 ,
1007,
295 ,
632 ,
778 ,
96  ,
817 ,
796 ,
726 ,
660 ,
833 ,
390 ,
450 ,
348 ,
445 ,
730 ,
713 ,
289 ,
139 ,
995 ,
538 ,
151 ,
886 ,
37  ,
274 ,
647 ,
459 ,
625 ,
488 ,
507 ,
607 ,
664 ,
639 ,
842 ,
417 ,
560 ,
409 ,
397 ,
364 ,
423 ,
921 ,
171 ,
869 ,
282 ,
735 ,
379 ,
1002,
837 ,
65  ,
110 ,
59  ,
826 ,
496 ,
73  ,
547 ,
764 ,
586 ,
301 ,
748 ,
93  ,
345 ,
148 ,
504 ,
394 ,
376 ,
70  ,
145 ,
223 ,
686 ,
226 ,
975 ,
929 ,
722 ,
270 ,
917 ,
582 ,
718 ,
689 ,
185 ,
454 ,
743 ,
472 ,
601 ,
121 ,
246 ,
954 ,
229 ,
693 ,
434 ,
477 ,
858 ,
575 ,
964 ,
127 ,
978 ,
905 ,
352 ,
989 ,
115 ,
239 ,
941 ,
253 ,
932 ,
189 ,
566 ,
786 ,
703 ,
294 ,
777 ,
816 ,
725 ,
832 ,
449 ,
444 ,
712 ,
138 ,
537 ,
885 ,
273 ,
458 ,
487 ,
606 ,
638 ,
416 ,
408 ,
363 ,
920 ,
868 ,
734 ,
1001,
64  ,
58  ,
495 ,
546 ,
585 ,
747 ,
344 ,
503 ,
375 ,
144 ,
685 ,
974 ,
721 ,
916 ,
717 ,
184 ,
742 ,
600 ,
245 ,
953 ,
692 ,
476 ,
574 ,
126 ,
904 ,
988 ,
238 ,
252 ,
188 ,
785 ,
293 ,
815 ,
831 ,
443 ,
137 ,
884 ,
457 ,
605 ,
415 ,
362 ,
867 ,
1000,
57  ,
545 ,
746 ,
502 ,
143 ,
973 ,
915 ,
183 ,
599 ,
952 ,
475 ,
125 ,
987 ,
251 ,
784 ,
814 ,
442 ,
883 ,
604 ,
361 ,
999 ,
544 ,
501 ,
972 ,
182 ,
951 ,
124 ,
250 ,
813 ,
882 ,
360 ,
543 ,
971 ,
950 ,
249 ,
881 ,
542 ,
949 ,
880 ,
948 ,	// 1022
947};	// 1023



#endif /* INC_TPX2_COMM_H_ */
