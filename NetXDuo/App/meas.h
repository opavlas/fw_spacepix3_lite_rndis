/*
 * Readout.h
 *
 *  Created on: Apr 18, 2024
 *      Author: opavl
 */

#ifndef APP_MEAS_H_
#define APP_MEAS_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "pinout.h"
#include "debug.h"
#include "board.h"
#include "tpx2_comm.h"

#define NX_DISABLE_UDP_INFO


#define readout_log_debug( fmt, ...)		printf( "Debug: %s:%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__ )
#define readout_log_warning( fmt, ...)		printf( "Warning: %s:%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__ )
#define readout_log_error( fmt, ...)		printf( "Error: %s:%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__ )

#define UDP_COMMAND_DATA_PORT		1555
#define UDP_MEASUREMENT_DATA_PORT	1556

/////// HW Definition /////////////////
#define READOUT_HW_TPX3_1LAYER_TYPE	0x01
#define READOUT_HW_TPX3_8LAYER_TYPE	0x03
#define READOUT_HW_TPX2_1LAYER_TYPE	0x02
#define READOUT_HW_TPX2_LITE		0x24		// Work with version of TL 1.5 and higer
#define READOUT_HW_REVISION			0x01
#define READOUT_HW_SERIAL_NUMBER	0xaa
#define READOUT_HW_FW_VERSION		0x01

#define NUM_OF_SEND_PIXELS			65536
#define NUM_OF_SEND_PIXELS_SPX3			4096
#define MATRIX_SIZE					65536
#define TEST_PATTERN_CIRCLE
#define TEST_SENDING
// 1536
#define UDP_BUF_SIZE				768
#define UDP_DBG_BUF_SIZE			256

typedef struct {
	// Timepix 2
	TPX2_CONFIG tpx2Cfg;
	// board
	float temperature;
	float biasVoltage;
	float feadbackTpx2Dacs[27];		// Analog value od DAC output

	uint32_t commStatus;
	float acqTimeLsb;
	uint32_t acqTime;
	uint32_t numberOfFrames;
	uint8_t col_trigger_en;			// EN - '1'. DIS - '0'
	uint8_t data_mode;
	ULONG sender_ip;
	uint16_t cmd_port;
	uint16_t data_port;
	bool conti_mode;
} READOUT_CONFIG;


typedef struct {
	bool measurementInProgress;
	uint32_t numberOfFrames;
	uint32_t eq_frames;
	bool set_freq;

	bool tx_done;
	bool open_shutter;

	int start_col_trig_time;
	int end_col_trig_time;
	int diff_col_trig_time;

	int start_shutter_time;
	int end_shutter_time;
	int diff_shutter_time;

	int initial_time;
	int end_time;
	int diff_time;

	uint64_t acq_start_time;
	uint64_t acq_end_time;

	bool first_frame;

	int info_start_time;
	int info_end_time;
	int info_diff_time;

	int info1_start_time;
	int info1_end_time;
	int info1_diff_time;

	int start_time_udp_send;
	int end_time_udp_send;
	int diff_time_udp_send;

	int start_time_transform;
	int end_time_transform;
	int diff_time_transform;

	int start_time_readout_counters;
	int end_time_readout_counters;
	int diff_time_readout_counters;

	bool init_boot_mode;
} READOUT_STATUS;




/// This enum determines mapping of logical names to indices in `DacArray`

typedef enum {
  ToT10_ToA18 = 0x0,       /// Simultaneous ToT (10-bit) and 1st hit ToA (18-bit) with sequential read/write
  ToT14_ToA14 = 0x1,       /// Simultaneous ToT (14-bit) and 1st hit ToA (14-bit) with sequential read/write
  ContToT10_Event4 = 0x2,  /// Continuous read/write ToT (10-bit) with supplementary event counting (4-bit)
  ContToT14 = 0x3,         /// Continuous read/write ToT (14-bit)
  ContToA10 = 0x4,         /// Continuous read/write 1st hit ToA (10-bit)
  ContToA14 = 0x5,         /// Continuous read/write 1st hit ToA (14-bit)
  ContEvent10 = 0x6,       /// Continuous read/write event counting (10-bit)
  ContEvent14 = 0x7,       /// Continuous read/write event counting (14-bit)
  iToT10_ToA18 = 0x8,      /// Simultaneous integral ToT (10-bit) and 1st hit ToA (18-bit) with sequential read/write
  iToT14_ToA14 = 0x9,      /// Simultaneous integral ToT (14-bit) and 1st hit ToA (14-bit) with sequential read/write
  ContiToT10_Event4 = 0xA, /// Continuous read/write integral ToT (10-bit) with supplementary event counting (4-bit)
  ContiToT14 = 0xB,        /// Continuous read/write integral ToT (14-bit)
} DATA_MODE;

typedef enum {
	SENSOR_CONFIG_REGISTERS_UPDATE = 0,
	INTERNAL_DAC_UPDATE = 1,
	INTERNAL_DAC_BACK_READ = 2,
	TIMER_READ = 3,
	TIMER_SET = 4,
	RESET_MATRIX_SEQUENTIAL = 5,
	SET_TRIM_CONF_REGISTER = 6,
	LOAD_COLUMN_TEST_PULSE_REGISTER = 7,
	READ_COLUMN_TEST_PULSE_REGISTER = 8,
	LOAD_PIXEL_REGISTER_CONFIGURATION = 9,
	READ_PIXEL_REGISTER_CONFIGURATION = 10,
	READ_PIXEL_MATRIX_SEQUENTIAL_SETTING = 11,
	READ_PIXEL_MATRIX_DATA_DRIVEN_SETTING = 12,
	CHIP_ID_READ = 13,
	OUTPUT_BLOCK_CONFIG_UPDATE = 14,
	DIGITAL_TEST = 15,
} HW_CMD_ID;

enum KATHERINE_CMD {
	CMD_ACQ_TIME_SETTING_LSB 		= 0x01,
	CMD_BIAS_SETTING 		 		= 0x02,
	CMD_ACQ_START 			 		= 0x03,
	CMD_INTERNAL_DAC_SETTINGS 		= 0x04,
	CMD_TPX2_SET_OMR 			= 0x05,	   // NOTE: numbering clash, seq. readout does not make sense for Timepix2 // CMD_TPX2_SET_OMR 			= 0x05,
	CMD_TPX2_SET_FRQ__STOP_ACQ 					= 0x06,// NOTE: numbering clash, interpretation depends on whether acquisition is currently ongoing 	//CMD_TPX2_SET_FRQ      		= 0x06,
	CMD_HW_COMMAND_START 			= 0x07,
	CMD_SENSOR_REGISTER_SETTING 	= 0x08,
	CMD_ACQ_MODE_SETTING 			= 0x09,
	CMD_ACQ_TIME_SETTING_MSB 		= 0x0A,
	CMD_ECHO_CHIP_ID 				= 0x0B,
	CMD_GET_BIAS_VOLTAGE 			= 0x0C,
	CMD_GET_ADC_VOLTAGE 			= 0x0D,
	CMD_GET_BACK_READ_REGISTER 		= 0x0E,
	CMD_INTERNAL_DAC_SCAN 			= 0x0F,
	CMD_SET_PIXEL_CONFIG            = 0x10,
	CMD_GET_PIXEL_CONFIG            = 0x11,
	CMD_SET_ALL_PIXEL_CONFIG 		= 0x12,
	CMD_NUMBER_OF_FRAMES_SETTING 	= 0x13,
	CMD_GET_ALL_DAC_SCAN 			= 0x14,
	CMD_GET_HW_READOUT_TEMPERATURE  = 0x15,
	CMD_LED_SETTINGS 				= 0x16,
	CMD_GET_READOUT_STATUS 			= 0x17,
	CMD_GET_COMMUNICATION_STATUS 	= 0x18,
	CMD_GET_SENSOR_TEMPERATURE 		= 0x19,
	CMD_DIGITAL_TEST 				= 0x20,
	CMD_ACQUISITION_SETUP 			= 0x21,
	CMD_GET_ACQUISITION_SETUP 		= 0x22,
	CMD_INTERNAL_TRIGGER_GENERATOR 	= 0x23,
	CMD_TPX2_SET_COL_TRIGGER		= 0x27,
	CMD_TOA_CALIBRATION_SETUP 		= 0x28,
	CMD_SET_NUMBER_OF_TOKENS 		= 0x29,
	CMD_GET_BIAS_CURRENT			= 0x30,
	CMD_INTERNAL_TDC_SETTINGS 		= 0x32,
	CMD_INTERNAL_TDC_READ_COUNTS 	= 0x33,
	CMD_INTERFACE_SELECT            = 0x50,
	CMD_CHANGE_PORTS 				= 0xF0,
};

enum DBG_CMD {
	CMD_DBG_UDP_ECHO = 0xAA,
	CMD_DBG_UDP_SPEED = 0xAB,
	CMD_DBG_TEST_MODE = 0xAC,
	CMD_DBG_INIT_BOOT_MODE = 0xDD,
	CMD_DBG_ACT_BOOT_MODE = 0xDE,
	CMD_DBG_NOT_ACT_BOOT_MODE = 0xDF,
	CMD_DBG_SPX3_MATRIX = 0xAD,
	CMD_DBG_SPX3_GET_ALL_DAC_SCAN = 0xAE,
	CMD_DBG_SPX3_GET_VSSA = 0xAF,
	CMD_DBG_SPX3_GET_TEMP = 0xB0,
	CMD_DBG_SPX3_GET_ADC_IN = 0xB1,
	CMD_DBG_SPX3_INJECT = 0xB2,
	CMD_DBG_SPX3_GET_ADC_IN_SCAN = 0xB3,
	CMD_DBG_HV = 0xB4,
	CMD_GET_STM32_ID = 0xB5,
};

typedef enum {
	RX_STATE_CMD_STATE,
	RX_CHIPCONFIG_STATE,
} RECEIVE_STATE;

typedef enum {
	NEW_FRAME_ESTABLISHED = 0x7,
	PIXEL_MEASUREMENT_DATA = 0x4,
	PIXEL_TIMESTAMP_OFFSET = 0x5,
	CURRENT_FRAME_FINISHED = 0xC,
	START_OF_FRAME_TIMESTAMP_LSB = 0x8,
	START_OF_FRAME_TIMESTAMP_MSB = 0x9,
	END_OF_FRAME_TIMESTAMP_LSB = 0xA,
	END_OF_FRAME_TIMESTAMP_MSB = 0xB,
	NUMBER_OF_LOST_PIXELS = 0xD,
	MEASUREMENT_ABORTED_NOTICE = 0xE
} MEASUREMENT_DATA_TYPE;



void ReadoutMainCmdHandler( NX_UDP_SOCKET *udp_socket, NX_PACKET *rx_data_packet );
UINT UDP_packet_pool();
UINT SendUdpPacket( NX_UDP_SOCKET *udp_socket, uint8_t *dataTx, uint32_t dataTxLength, ULONG dst_ip_address, UINT port );
void CommandInternal( HW_CMD_ID cmdId );
static void send_pixel_matrix(NX_UDP_SOCKET *udp_socket, ULONG dst_ip_address, uint8_t datamode,
							  uint16_t *counterA_decode, uint16_t *counterB_decode, uint8_t *counterC_decode, uint8_t *counterD_decode);
void MeasurementStart( NX_UDP_SOCKET *udp_socket, ULONG dst_ip_address, uint8_t datamode, uint8_t col_trigger_en);
uint8_t MeasurementProcess( NX_UDP_SOCKET *udp_socket, ULONG dst_ip_address, uint8_t datamode, uint8_t col_trigger_en, uint8_t *counterA_raw, uint8_t *counterB_raw, uint8_t *counterC_raw, uint8_t *counterD_raw,
		uint16_t *counterA_decode_get, uint16_t *counterB_decode_get, uint8_t *counterC_decode_get, uint8_t *counterD_decode_get);
void MeasurementStop( NX_UDP_SOCKET *udp_socket, ULONG dst_ip_address);
void read_out_counters(counterA_raw, counterB_raw, counterC_raw, counterD_raw);
void send_spx3_pixel_matrix(NX_UDP_SOCKET *udp_socket, ULONG dst_ip_address,uint8_t *rx_data);
//void ReadoutMainCmdHandler( NX_UDP_SOCKET *udp_socket, NX_PACKET *data_packet );

#endif /* APP_MEAS_H_ */
