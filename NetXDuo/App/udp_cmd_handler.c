/*
 * udp_cmd_handler.c
 *
 *  Created on: Dec 12, 2024
 *      Author: opavl
 */

#include "app_netxduo.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <user.h>

#include "pinout.h"
#include "debug.h"
#include "board.h"
#include "tpx2_comm.h"

#include "meas.h"
#include "spi.h"

#include "udp_cmd_handler.h"


#if defined(TPX_CMD)
#endif
#if defined(SPX_CMD)
#endif

// Define in Readout.c
extern READOUT_STATUS readOutStatus;
extern READOUT_CONFIG readOutConfig;
extern SPX3_CONFIG spxReadOutConfig;
uint8_t status = 0;

#define RELASE


void Convert32bitTo4xByteArray( uint32_t in, uint8_t *out4)
{
	out4[0] = (in>>0) & 0xFF;
	out4[1] = (in>>8) & 0xFF;
	out4[2] = (in>>16) & 0xFF;
	out4[3] = (in>>24) & 0xFF;
}
uint32_t Convert4xByteArrayTo32bit( uint8_t *in4 )
{
	return (uint32_t)(in4[0]<<0)|(in4[1]<<8)|(in4[2]<<16)|(in4[3]<<24);
}

/** Receive data on a udp session */
void ReadoutMainCmdHandler( NX_UDP_SOCKET *udp_socket, NX_PACKET *data_packet )
{
	UINT source_port;
	ULONG sender_ip_address;

	static RECEIVE_STATE rxFsm = RX_STATE_CMD_STATE;
	static uint32_t chipConfigOffset = 0;


	uint8_t dataRx[6000] = {0};
	ULONG dataRxLen = 0;

	uint8_t dataTx[8] = {0};
	UINT ret;

	//TX_MEMSET(dataRx, '\0', sizeof(dataRx));

	/* data is available, read it into the data buffer */
	nx_packet_data_retrieve(data_packet, dataRx, &dataRxLen);

	nx_udp_source_extract(data_packet, &sender_ip_address, &source_port);

	readOutConfig.cmd_port = 1555;
	readOutConfig.data_port = 1556;
	readOutConfig.data_mode = ToT10_ToA18;		// only mode for spx3

	/* get info about the client address and port */

	if( rxFsm == RX_CHIPCONFIG_STATE ){
#if 1
		/* NOTE: Timepix2 pixel registers are laid out as follows:
		      bits 0-4 .. adjustment (5b)
		      bit 5    .. unknown / unused (1b)
		      bit 6    .. test (1b)
		      bit 7    .. mask (1b)
		*/
		if( dataRxLen == 4096 ){
			if( chipConfigOffset < sizeof( readOutConfig.tpx2Cfg.chipConfig ) ){
				memcpy( &readOutConfig.tpx2Cfg.chipConfig[chipConfigOffset], dataRx, dataRxLen );
				chipConfigOffset += dataRxLen;
				log_debug("Config Matrix - part: 0x%x", chipConfigOffset );

				if( sizeof( readOutConfig.tpx2Cfg.chipConfig ) == chipConfigOffset ){
					rxFsm = RX_STATE_CMD_STATE;
					// Ack
					dataTx[0] = 0x00;
					dataTx[1] = 0x00;
					dataTx[2] = 0x00;
					dataTx[3] = 0x00;
					dataTx[4] = 0x00;
					dataTx[5] = 0x00;
					dataTx[6] = CMD_SET_ALL_PIXEL_CONFIG;
					dataTx[7] = 0x00;
					SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
					log_debug("Config Matrix - Valid" , chipConfigOffset );
				}
			}
		} else {
			log_debug("Config Matrix Invalid");
			rxFsm = RX_STATE_CMD_STATE;
		}
#endif
	} else if( rxFsm == RX_STATE_CMD_STATE ){

		// TODO proccess what to do when T2M is under measuring

		uint8_t commandId = *(uint16_t *)(&dataRx[6]);
		memset( dataTx, 0x00, sizeof(dataTx) );
		switch (commandId){
			case CMD_ECHO_CHIP_ID :
				readOutConfig.cmd_port = (dataRx[0]<<0)|(dataRx[1]<<8);
				Tpx2GetChipId(&readOutConfig.tpx2Cfg.chipId);
				Convert32bitTo4xByteArray( readOutConfig.tpx2Cfg.chipId, &dataTx[0] );
				dataTx[4] = 0x00;
				dataTx[5] = 0x00;
				dataTx[6] = CMD_ECHO_CHIP_ID;			// CMD RESPONSE ID
				dataTx[7] = 0x00;
				// TODO if wanna connect multiple device need to answer on new port, but not change pernament
				// TODO -> connection of multiple devices
				readOutConfig.cmd_port = UDP_COMMAND_DATA_PORT;
				readOutConfig.data_port = UDP_MEASUREMENT_DATA_PORT;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port);

				log_debug("CMD_ECHO_CHIP_ID");
				log_debug("First CMD PORT %d", readOutConfig.cmd_port);
				break;

			case CMD_GET_READOUT_STATUS :
				uint32_t readoutStatus = (READOUT_HW_FW_VERSION<<24)|(READOUT_HW_SERIAL_NUMBER<<16)|(READOUT_HW_REVISION<<8)|(READOUT_HW_TPX2_LITE <<0);
				Convert32bitTo4xByteArray( readoutStatus, &dataTx[0] );
				dataTx[6] = CMD_GET_READOUT_STATUS;			// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_GET_READOUT_STATUS");
				break;

			case CMD_BIAS_SETTING :
				uint32_t value32 = (dataRx[0]<<0)|(dataRx[1]<<8)|(dataRx[2]<<16)|(dataRx[3]<<24);
				readOutConfig.biasVoltage = *(float *)&value32;
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_BIAS_SETTING;			// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				if(BoardSetBiasVoltage(readOutConfig.biasVoltage) != BOARD_OK){
					log_error("Setting of HV");
					Error_Handler();
				}
				log_debug("CMD_BIAS_SETTING");
				break;

			case CMD_GET_BIAS_VOLTAGE :	 // TODO look on PM documentation
				// NOTE: Section 1.2.7 of documentation specifies that only byte #4 should have biasIndex but that does not work for
				//       Gen2. The following structure was empirically tested to work.

				//readOutConfig.biasVoltage = 22;
				BoardGetBiasVoltage(&readOutConfig.biasVoltage);
				Convert32bitTo4xByteArray(  *(uint32_t *)&readOutConfig.biasVoltage, &dataTx[0] );
				dataTx[4] = 0x00;
				dataTx[5] = 0x00;
				dataTx[6] = CMD_GET_BIAS_VOLTAGE;			// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_GET_BIAS_VOLTAGE");
				break;

			case CMD_GET_COMMUNICATION_STATUS :	// TODO not doumented for TPX2?
				readOutConfig.commStatus = 0x1a081;
				Convert32bitTo4xByteArray( *(uint32_t *)&readOutConfig.commStatus, &dataTx[0] );
				dataTx[0] = 0x81;
				dataTx[1] = 0xa1;	// total_data_rate (this value * 5 = Total Data Rate [Mbs] between readout HW and sensor chip)
				dataTx[4] = 0x00;
				dataTx[5] = 0x00;
				dataTx[6] = CMD_GET_COMMUNICATION_STATUS;			// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_GET_COMMUNICATION_STATUS");
				break;

			case CMD_GET_HW_READOUT_TEMPERATURE :
				readOutConfig.temperature = 1;
				BoardGetTemp(&readOutConfig.temperature);

				Convert32bitTo4xByteArray( *(uint32_t *)&readOutConfig.temperature, &dataTx[0] );
				dataTx[4] = 0x00;
				dataTx[5] = 0x00;
				dataTx[6] = CMD_GET_HW_READOUT_TEMPERATURE;		// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_GET_HW_READOUT_TEMPERATURE");
				break;
			case CMD_GET_SENSOR_TEMPERATURE :
				float temp = 1;
				BoardGetSensorTemp(&temp);
				Convert32bitTo4xByteArray( *(uint32_t *)&temp, &dataTx[0] );
				dataTx[4] = 0x00;
				dataTx[5] = 0x00;
				dataTx[6] = CMD_GET_SENSOR_TEMPERATURE;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_GET_SENSOR_TEMPERATURE");
				break;

			case CMD_INTERNAL_DAC_SETTINGS :
				uint16_t dacValue = dataRx[0] | (dataRx[1]<<8);
				uint8_t dacIndex = dataRx[4]; // FOR TPX3 ONLY: dacIndex+1 - because Katherine manual
				Tpx2WriteDacToRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, dacIndex, dacValue);
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_INTERNAL_DAC_SETTINGS; // CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				//log_debug("CMD_INTERNAL_DAC_SETTINGS");
				break;

			case CMD_HW_COMMAND_START :
				HW_CMD_ID cmdId = dataRx[0];
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_HW_COMMAND_START;		// CMD RESPONSE ID
				dataTx[7] = 0x00;
				CommandInternal( cmdId );
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_HW_COMMAND_START");
				//CommandInternal( cmdId );

				break;

			case CMD_GET_ALL_DAC_SCAN :
				//const int responsePacketLength = 8;
				// update all DACs of TPX2

				UpdateTpx2Dacs(readOutConfig.feadbackTpx2Dacs, NUM_OF_DACS);	 // read all DACs value into feadbackTpx2Dacs
				for( uint8_t i = 0; i < NUM_OF_DACS; i++)
				{
					//uint8_t dacIndex = dacsScanList[i];
					float dacVoltage = readOutConfig.feadbackTpx2Dacs[i];
					Convert32bitTo4xByteArray( *(uint32_t *)&dacVoltage, &dataTx[0]);
					dataTx[4] = 0x00;
					dataTx[5] = 0x00;
					dataTx[6] = CMD_GET_ALL_DAC_SCAN;	// CMD RESPONSE ID
					dataTx[7] = 0x00;
					SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				}
				log_debug("CMD_GET_ALL_DAC_SCAN");
				break;

			case CMD_INTERNAL_DAC_SCAN :	// TODO ??
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_INTERNAL_DAC_SCAN;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_INTERNAL_DAC_SCAN");
				break;

			case CMD_SENSOR_REGISTER_SETTING :	// TODO not documented
				uint8_t regIndex = dataRx[4];
				uint8_t chipIndex = dataRx[5];
				uint32_t regValue = Convert4xByteArrayTo32bit( &dataRx[0] );

				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_SENSOR_REGISTER_SETTING;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_SENSOR_REGISTER_SETTING");
				break;

			case CMD_LED_SETTINGS :	// TODO Not documented
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_LED_SETTINGS;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_LED_SETTINGS");
				break;

			case CMD_GET_ADC_VOLTAGE :		// just for debugging
				uint8_t adcChannelId = dataRx[0];
				(void)adcChannelId;
				dataTx[0] = 0xe5;
				dataTx[1] = 0xa1;
				dataTx[2] = 0x84;
				dataTx[3] = 0x00;
				dataTx[4] = 0x00;
				dataTx[5] = 0x00;
				dataTx[6] = CMD_GET_ADC_VOLTAGE;		// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_GET_ADC_VOLTAGE");
				break;


			case CMD_SET_NUMBER_OF_TOKENS :
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_SET_NUMBER_OF_TOKENS;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_SET_NUMBER_OF_TOKENS");
				break;

			case CMD_GET_BACK_READ_REGISTER:
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_GET_BACK_READ_REGISTER;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_GET_BACK_READ_REGISTER");
				break;

			case CMD_GET_ACQUISITION_SETUP :
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_GET_ACQUISITION_SETUP;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_GET_ACQUISITION_SETUP");
				break;

			case CMD_INTERNAL_TRIGGER_GENERATOR :
				dataTx[0] = 0x00;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_INTERNAL_TRIGGER_GENERATOR;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_INTERNAL_TRIGGER_GENERATOR");
				break;

			case CMD_ACQ_MODE_SETTING :	// TODO Not so much documented
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_ACQ_MODE_SETTING;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_ACQ_MODE_SETTING");
				break;

			case CMD_TOA_CALIBRATION_SETUP :
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_TOA_CALIBRATION_SETUP;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_TOA_CALIBRATION_SETUP");
				break;

			case CMD_ACQ_TIME_SETTING_LSB :
				 // Gen2 has just one command instead of MSB/LSB. However, the command code matches the Gen1 LSB command.
				 // In the new command, time is passed as a float in seconds.
				uint32_t time = (dataRx[0]<<0)|(dataRx[1]<<8)|(dataRx[2]<<16)|(dataRx[3]<<24);
				readOutConfig.acqTimeLsb = *(float *)&time;
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_ACQ_TIME_SETTING_LSB;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_ACQ_TIME_SETTING_LSB");
				readOutConfig.acqTime = (uint32_t)((readOutConfig.acqTimeLsb)*1000); // ACQ. TIME, time in ms
				if(readOutConfig.acqTime <= 0){
					log_error("Acq. time problem");
				}
				log_debug("Acq. time: %d", readOutConfig.acqTime);
				break;

			case CMD_ACQ_TIME_SETTING_MSB : // Not using for TPX2
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_ACQ_TIME_SETTING_LSB;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_ACQ_TIME_SETTING_MSB");
				break;

			case CMD_NUMBER_OF_FRAMES_SETTING : // TODO
				readOutConfig.numberOfFrames = Convert4xByteArrayTo32bit(dataRx);
				dataTx[0] = 0x00;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_NUMBER_OF_FRAMES_SETTING;	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_NUMBER_OF_FRAMES_SETTING");
				if(readOutConfig.numberOfFrames <= 0){
					log_error("Num. of frame problem");
				}
				break;

			case CMD_ACQUISITION_SETUP :
				// TODO NOT DOCUMENTED
				// NOTE / WARNING: This command simply changes some bits of the GeneralConfig register
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_ACQUISITION_SETUP;	    // CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_ACQUISITION_SETUP");
				break;

			case CMD_TPX2_SET_OMR :	//  // TPX2 -> SET OMR registr  /// ODPOVEDET
				// CMD_TPX2_SET_OMR
				// NOTE: PB advises to leave bits #4 and 5 at zero
				// NOTE: bit #7 is hard-wired to 0, source: page 33 of Timepix2 manual v2.1
				// NOTE: PB advises to leave bit #11 at zero
				// NOTE: bits #12-15 are dedicated for DataMode but PB advises to keep them low, and use a dedicated command

				uint16_t omrValue[1] = {dataRx[0] | (dataRx[1]<<8)};		// GET OMR VALUE
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_TPX2_SET_OMR;			// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
#if defined (TPX_CMD)
				board_tpx2_set_shutter_counter(TPXA, 1);					// TPX2 manual pg. 33:  is best to set the IO pad SHUTTERn = 1 while changing the OMR
				//omrValue[0] = 0x734c;		// change to some own value
				status = tpx2_set_reg_16b(TPXA, SET_OMR, omrValue);			// SET OMR VALUE
				uint32_t get_omr = 0;
				status = tpx2_get_reg_16b(TPXA, GET_OMR, &get_omr);			// GET OMR VALUE
				log_debug("CMD_TPX2_SET_OMR");
				log_debug("GET OMR, datamode not set: 0x%x", get_omr);
#endif
				break;

			case CMD_ACQ_START :	// TODO	start measurment -> ONLY FOR TPX3??
				//uint8_t data_mode = dataRx[0];				// data mode is SET in CMD_TPX2_SET_OMR
				readOutConfig.data_mode = dataRx[0];
				uint8_t zero_suppresion = dataRx[1];		// if 1 -> ZCS enable // TODO
#if defined (TPX_CMD)
				set_data_mode(readOutConfig.data_mode);
				if(readOutConfig.data_mode > 0x1 && readOutConfig.data_mode < 0x8){
					readOutConfig.conti_mode = true;
				} else {
					readOutConfig.conti_mode = false;
				}

				uint32_t get_datamode = 0;
				status = tpx2_get_reg_16b(TPXA, GET_OMR, &get_datamode);			// GET OMR VALUE

				log_debug("ACQ START: GET OMR: 0x%x", get_datamode);
#endif
				// NOTE: does not produce ack	--> Measurement data…

				log_debug("CMD_ACQ_START");
				// TODO: type with what parametrs ACQ. start :....

				readOutStatus.measurementInProgress = true;
				readOutStatus.numberOfFrames = 0;
				readOutStatus.first_frame = true;
				readOutConfig.sender_ip = sender_ip_address;
				readOutStatus.measurementInProgress = true;
				//MeasurementStart(udp_socket, readOutConfig.sender_ip, readOutConfig.data_mode, readOutConfig.col_trigger_en);
				break;

			case CMD_TPX2_SET_FRQ__STOP_ACQ :	 // TODO !!!
				// CMD_TPX2_SET_FRQ
				uint8_t toaFreq[1] = {0};
				uint8_t totFreq = 0;
				if(readOutStatus.measurementInProgress == true){	// ACQ. STOP
					readOutStatus.numberOfFrames = readOutConfig.numberOfFrames;
					MeasurementStop(udp_socket, readOutConfig.sender_ip);
					readOutStatus.measurementInProgress = false;
					//tpx2_init(TPXA, NULL);

					// TODO read out the data from counter !!

					log_debug("ACQ STOP");
				}
				//} else {										    // CMD SET FREQ.

					// READ value of TOA and TOT
					toaFreq[0] = dataRx[0];
					totFreq = dataRx[1];

					dataTx[0] = 0x00;						// CRD
					dataTx[1] = 0x00;						// .
					dataTx[2] = 0x00;						// .
					dataTx[3] = 0x00;						// .
					dataTx[4] = 0x00;						// .
					dataTx[5] = 0x00;						// CRD
					dataTx[6] = CMD_TPX2_SET_FRQ__STOP_ACQ; // CMD RESPONSE ID
					dataTx[7] = 0x00;
					SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port);
#if defined (TPX_CMD)
					board_start_mclock(totFreq); // TODO set prescaler -> set MCLOCK freq. // need to set MCLOCK prescaler in MCU
					log_debug("totFreq number: %d", totFreq);

					MX_SPI2_Init_user(totFreq);
					status = tpx2_set_reg_8b(TPXA, SET_TOAFREQSEL, toaFreq); // default is 0x1E <-> ToA bypass from ToT (MCLOCK)
#endif
					readOutStatus.set_freq = 0;    // this is signal that freq. was set and after is call CMD CMD_TPX2_SET_FRQ__STOP_ACQ -> it will meas STOP ACQ
					log_debug("CMD_TPX2_SET_FRQ");
				//}
				break;

			case CMD_INTERNAL_TDC_SETTINGS :
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_INTERNAL_TDC_SETTINGS; 		// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_INTERNAL_TDC_SETTINGS");
				break;

			case CMD_SET_ALL_PIXEL_CONFIG :
				log_debug("CMD_SET_ALL_PIXEL_CONFIG");
				chipConfigOffset = 0;
				rxFsm = RX_CHIPCONFIG_STATE;
				break;

			case CMD_SET_PIXEL_CONFIG:	// TODO Not documented
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_SET_PIXEL_CONFIG; 		// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_SET_PIXEL_CONFIG");
				break;

			case CMD_GET_PIXEL_CONFIG:	// TODO Not documented
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_GET_PIXEL_CONFIG; 		// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_GET_PIXEL_CONFIG");
				break;

			case CMD_DIGITAL_TEST:		// TODO
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_DIGITAL_TEST; 			// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_DIGITAL_TEST");
				break;

			case CMD_TPX2_SET_COL_TRIGGER:
				readOutConfig.col_trigger_en = dataRx[2];
				uint16_t col_number[1] = {(dataRx[0]<<0)|(dataRx[1]<<8)};
				uint32_t get_col_number = 0;
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_TPX2_SET_COL_TRIGGER; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );

				log_debug("CMD_TPX2_SET_COL_TRIGGER");
				tpx2_set_reg_16b(TPXA, SET_COLHITTHRESHOLD, col_number);
				tpx2_get_reg_16b(TPXA, GET_COLHITTHRESHOLD, &get_col_number);
				log_debug("Trigger on/off 0x%x. Number of col. trigger: 0x%x", readOutConfig.col_trigger_en, get_col_number);
				break;

			case CMD_CHANGE_PORTS :			// TODO
				readOutConfig.cmd_port = (dataRx[0]<<0)|(dataRx[1]<<8);
				readOutConfig.data_port = (dataRx[2]<<0)|(dataRx[3]<<8);
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_CHANGE_PORTS; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_CHANGE_PORTS");
				log_debug("CMD PORT %d", readOutConfig.cmd_port);
				log_debug("DATA PORT %d", readOutConfig.data_port);
				break;

			case CMD_GET_BIAS_CURRENT:		// TODO
				dataTx[0] = 0x00;						// CRD
				dataTx[1] = 0x00;						// .
				dataTx[2] = 0x00;						// .
				dataTx[3] = 0x00;						// .
				dataTx[4] = 0x00;						// .
				dataTx[5] = 0x00;						// CRD
				dataTx[6] = CMD_GET_BIAS_CURRENT; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_GET_BIAS_CURRENT");
				break;

			case CMD_INTERNAL_TDC_READ_COUNTS:
				dataTx[0] = 0x00;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_INTERNAL_TDC_READ_COUNTS; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, readOutConfig.cmd_port );
				log_debug("CMD_INTERNAL_TDC_READ_COUNTS");
				break;

			case CMD_DBG_UDP_ECHO:
				dataTx[0] = 0x00;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_UDP_ECHO; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
				log_debug("CMD_DBG_UDP_ECHO");
				break;

			case CMD_DBG_UDP_SPEED:
				uint8_t dataTest[1000] = {0xAA};
				for(int i = 0; i < 1000; i++){
					SendUdpPacket( udp_socket, dataTest, sizeof(dataTest), sender_ip_address, UDP_COMMAND_DATA_PORT);
				}

				uint32_t speed_end = 0xEEEEEEEE;
				Convert32bitTo4xByteArray( *(uint32_t *)&speed_end, &dataTx[0]);	// ACK for enter BOOT MODE
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_UDP_SPEED; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);

				log_debug("CMD_DBG_UDP_SPEED");
				break;

			case CMD_DBG_INIT_BOOT_MODE:
				readOutStatus.init_boot_mode = true;
				uint32_t boot_ack = 0xAAAAAAAA;
				Convert32bitTo4xByteArray( *(uint32_t *)&boot_ack, &dataTx[0]);	// ACK for enter BOOT MODE
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_INIT_BOOT_MODE; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				// switch
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
				log_debug("CMD_DBG_INIT_BOOT_MODE");
				break;

			case CMD_DBG_ACT_BOOT_MODE:
				// Entering BOOT MODE
				if(readOutStatus.init_boot_mode){		// BOOT MODE activation only if previous command CMD_DBG_INIT_BOOT_MODE
					dataTx[0] = 0x00;							// CRD
					dataTx[1] = 0x00;							// .
					dataTx[2] = 0x00;							// .
					dataTx[3] = 0x00;							// .
					dataTx[4] = 0x00;							// .
					dataTx[5] = 0x00;							// CRD
					dataTx[6] = CMD_DBG_ACT_BOOT_MODE; 	// CMD RESPONSE ID
					dataTx[7] = 0x00;
					SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
					log_debug("CMD_DBG_ACT_BOOT_MODE");
					Delay(1000);
					JumpToBootloader();

				} else {		// send NACK -> bcs. oot mode is not init
					dataTx[0] = 0x00;							// CRD
					dataTx[1] = 0x00;							// .
					dataTx[2] = 0x00;							// .
					dataTx[3] = 0x00;							// .
					dataTx[4] = 0x00;							// .
					dataTx[5] = 0x00;							// CRD
					dataTx[6] = CMD_DBG_NOT_ACT_BOOT_MODE; 	// CMD RESPONSE ID
					dataTx[7] = 0x00;
					SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
					log_debug("CMD_DBG_NOT_ACT_BOOT_MODE");
				}
				break;
			case CMD_DBG_SPX3_MATRIX:	// TODO rename case to DBG_SPX3_START_ACQ
				dataTx[0] = 0x00;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_SPX3_MATRIX; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				log_debug("CMD_DBG_SPX3_MATRIX");
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);

#if 1
				// Rx Data
				// rx_data are uint8_t but 1 px has 16bit
				const uint32_t rx_data_length = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE*2+1);
				uint8_t *rx_data = (uint8_t *)malloc( rx_data_length );

				memset( rx_data, 0x00, rx_data_length );
				if( rx_data == NULL ){
					log_error("counter_rx_pattern == NULL");

				}
				readOutConfig.acqTime = 10;
				spx3_pixel_matrix_config(spxReadOutConfig.pixel_matrix_config_default, NULL);	// DEFAULT: PX. MATRIX CONFIG
				spx3_global_config(spxReadOutConfig.global_config_default, NULL);				// DEFAULT: GLOBAL CONFIG

				// parametrs to be be set for acqthread
				readOutStatus.measurementInProgress = true;
				readOutStatus.numberOfFrames = 0;
				readOutStatus.first_frame = true;
				readOutConfig.sender_ip = sender_ip_address;
				readOutStatus.measurementInProgress = true;
				readOutConfig.data_port = 1556;

				// Strat of ACQ. - one frame
				//spx3_data_readout(rx_data, readOutConfig.acqTime);
				//send_spx3_pixel_matrix(udp_socket, sender_ip_address, rx_data);

				SAFE_FREE(rx_data);
#endif
				break;

			case CMD_DBG_SPX3_GET_ALL_DAC_SCAN:
				// this command will be after replace by: CMD_GET_ALL_DAC_SCAN
				// scan DACs from Spacepix into feedbackdac config register
				dataTx[0] = 0x00;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_SPX3_GET_ALL_DAC_SCAN; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				log_debug("CMD_DBG_SPX3_GET_ALL_DAC_SCAN");
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);

				// Read dig. value into feadbackTpx2Dacs
				UpdateSpx3Dacs(spxReadOutConfig.feadbackSpx3Dacs, NUM_OF_SPX3_DACS);

				// Send somehow all dacs:
				for(uint8_t i = 0; i < NUM_OF_SPX3_DACS; i++){
					uint16_t tmp = 0;
					tmp |= spxReadOutConfig.feadbackSpx3Dacs[i];
					dataTx[0] = tmp & 0xFF;
					dataTx[1] = (tmp >> 8) & 0xFF;
					dataTx[6] = CMD_DBG_SPX3_GET_ALL_DAC_SCAN;
					dataTx[7] = i;						// means index of DAC
					SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
				}

				break;

			case CMD_DBG_SPX3_GET_VSSA:
#if 0
				spxReadOutConfig.spx3_vssa = 0;
				spx3_get_vssa(&spxReadOutConfig.spx3_vssa);
				Convert32bitTo4xByteArray(  *(uint32_t *)&spxReadOutConfig.spx3_vssa, &dataTx[0] );
				log_debug("vssa: %.3f", spxReadOutConfig.spx3_vssa);						// .
#endif
				dataTx[0] = 0x00;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_SPX3_GET_VSSA; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				log_debug("CMD_DBG_SPX3_GET_VSSA");
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
				break;


			case CMD_DBG_SPX3_GET_ADC_IN:
#if 0
				// TODO set adc_in
				spxReadOutConfig.spx3_adc_in = 0;
				spx3_get_adc_in(&spxReadOutConfig.spx3_adc_in);
				Convert32bitTo4xByteArray(  *(uint32_t *)&spxReadOutConfig.spx3_adc_in, &dataTx[0] );
				log_debug("adc_in: %.3f", spxReadOutConfig.spx3_adc_in);						// .
#endif
				dataTx[0] = 0x00;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_SPX3_GET_ADC_IN; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				log_debug("CMD_DBG_SPX3_GET_ADC_IN");
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
#if 1
				// Rx Data
				// rx_data are uint8_t but 1 px has 16bit
				const uint32_t rx_data_length_adcin = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE*2+1);
				uint8_t *rx_data_adcin = (uint8_t *)malloc( rx_data_length_adcin );

				memset( rx_data_adcin, 0x00, rx_data_length_adcin );
				if( rx_data_adcin == NULL ){
					log_error("counter_rx_pattern == NULL");

				}

				spx3_pixel_matrix_config(spxReadOutConfig.pixel_matrix_config_default, NULL);	// DEFAULT: PX. MATRIX CONFIG
				spx3_global_config(spxReadOutConfig.global_config_default, NULL);

				spx3_data_readout_adcin(rx_data_adcin, 10, DAC_MAX_VALUE/2);
				send_spx3_pixel_matrix(udp_socket, sender_ip_address, rx_data_adcin);


				SAFE_FREE(rx_data_adcin);
#endif
				break;

			case CMD_DBG_SPX3_GET_ADC_IN_SCAN:
#if 0
				// TODO set adc_in
				spxReadOutConfig.spx3_adc_in = 0;
				spx3_get_adc_in(&spxReadOutConfig.spx3_adc_in);
				Convert32bitTo4xByteArray(  *(uint32_t *)&spxReadOutConfig.spx3_adc_in, &dataTx[0] );
				log_debug("adc_in: %.3f", spxReadOutConfig.spx3_adc_in);						// .
#endif
				dataTx[0] = 0x00;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_SPX3_GET_ADC_IN_SCAN; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				log_debug("CMD_DBG_SPX3_GET_ADC_IN_SCAN");
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
#if 1
				// Rx Data
				// rx_data are uint8_t but 1 px has 16bit
				const uint32_t rx_data_length_adcin_scan = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE*2+1);
				uint8_t *rx_data_adcin_scan = (uint8_t *)malloc( rx_data_length_adcin_scan );

				memset( rx_data_adcin_scan, 0x00, rx_data_length_adcin_scan );
				if( rx_data_adcin_scan == NULL ){
					log_error("counter_rx_pattern == NULL");

				}
				spx3_pixel_matrix_config(spxReadOutConfig.pixel_matrix_config_default, NULL);	// DEFAULT: PX. MATRIX CONFIG
				spx3_global_config(spxReadOutConfig.global_config_default, NULL);

				uint32_t adc_value = DAC_MAX_VALUE;
				for(uint32_t i = 0; i < 1700; i += 100){
					spx3_data_readout_adcin(rx_data_adcin_scan, 10, i);
					send_spx3_pixel_matrix(udp_socket, sender_ip_address, rx_data_adcin_scan);
				}

				SAFE_FREE(rx_data_adcin_scan);
#endif
				break;

			case CMD_DBG_SPX3_GET_TEMP:
				spxReadOutConfig.spx3_temp = 0;
				spx3_get_temp(&spxReadOutConfig.spx3_temp);
				Convert32bitTo4xByteArray(  *(uint32_t *)&spxReadOutConfig.spx3_temp, &dataTx[0] );
				log_debug("temp: %.3f", spxReadOutConfig.spx3_temp);
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_SPX3_GET_TEMP; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				log_debug("CMD_DBG_SPX3_GET_TEMP");
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
				break;

			case CMD_DBG_SPX3_INJECT:
				dataTx[0] = 0x00;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_SPX3_INJECT; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				log_debug("CMD_DBG_SPX3_INJECT");
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);

#if 1
				// Rx Data
				// rx_data are uint8_t but 1 px has 16bit
				const uint32_t rx_data_length_inj = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE*2+1);
				uint8_t *rx_data_inj = (uint8_t *)malloc( rx_data_length_inj );

				memset( rx_data_inj, 0x00, rx_data_length_inj );
				if( rx_data_inj == NULL ){
					log_error("counter_rx_pattern == NULL");

				}
				spx3_pixel_matrix_config(spxReadOutConfig.pixel_matrix_config_default, NULL);	// DEFAULT: PX. MATRIX CONFIG
				spx3_global_config(spxReadOutConfig.global_config_default, NULL);

				spx3_data_readout_inj(rx_data_inj, 10, DAC_MAX_VALUE);
				send_spx3_pixel_matrix(udp_socket, sender_ip_address, rx_data_inj);
#if 0
				uint16_t inject = DAC_MAX_VALUE;
				for(uint32_t i = 0; i < 1700; i += 100){
					spx3_data_readout_inj(rx_data_inj, 10, i);
					send_spx3_pixel_matrix(udp_socket, sender_ip_address, rx_data_inj);
				}
#endif
				SAFE_FREE(rx_data_inj);
#endif

				break;
			case CMD_DBG_HV:
				if(dataRx[0] == 0x00){	// turn off HV
					board_spx3_set_en_hv(SPXA, 0);
					board_stop_mclock();
					log_debug("HV turn OFF");
				} else if (dataRx[0] == 0x01) {
				#if 1
					board_spx3_set_en_hv(SPXA, 1);	// rename function to CS or EN_HV !!!a
					board_start_mclock(Frequency_HV_150V);
					Delay(10);
					//board_spx3_set_ncs_hv(SPXA, 1);
					//write_ltc2635(DACB, 0x0);
					//board_scan_pwm();
				#endif
					//board_start_mclock(Frequency_HV_150V);
					//log_debug("HV turn ON");
				}
				dataTx[0] = 0x00;;							// CRD
				dataTx[1] = 0x00;							// .
				dataTx[2] = 0x00;							// .
				dataTx[3] = 0x00;							// .
				dataTx[4] = 0x00;							// .
				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_DBG_HV; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				log_debug("CMD_DBG_HV");
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
				break;
			case CMD_GET_STM32_ID:
				uint32_t uid0 = *(uint32_t*)(UID_BASE + 0x00);  // LOT[31:0]
				uint32_t uid1 = *(uint32_t*)(UID_BASE + 0x04);  // LOT[47:32]
				uint32_t uid2 = *(uint32_t*)(UID_BASE + 0x08);  // WAFER, X, Y
				// Wafer info
				uint8_t wafer  = (uid2 >> 24) & 0xFF;
				uint8_t xcoord = (uid2 >> 16) & 0xFF;
				uint8_t ycoord = (uid2 >> 8)  & 0xFF;
				// Lot number (use only UID0 lower 32 bits)
				uint32_t lot = uid0;  // 32-bit lot
				Convert32bitTo4xByteArray(  lot, &dataTx[0] );

				dataTx[5] = 0x00;							// CRD
				dataTx[6] = CMD_GET_STM32_ID; 	// CMD RESPONSE ID
				dataTx[7] = 0x00;
				log_debug("CMD_GET_STM32_ID");
				log_debug("Lot Number   : 0x%08lX\n", lot);
				SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), sender_ip_address, UDP_COMMAND_DATA_PORT);
				break;

			case CMD_DBG_TEST_MODE:
				log_debug("CMD_DBG_TEST_MODE");
				log_debug("Entering TEST mode, some loop will execute ...");
				uint8_t result = TPX_OK;
				// Note: First readout of ZCS occures to be false: like HDR and FIN -> another readout looks fine after

				uint8_t act_A[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
				uint8_t act_B[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
				uint8_t act_C[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
				uint8_t act_D[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
				uint16_t act_column_A = 0;

#if 0	// USE with all counters
				uint8_t ZCS_B[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
				uint16_t act_B = 0;
				uint8_t ZCS_C[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
				uint16_t act_C = 0;
				uint8_t ZCS_D[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
				uint16_t act_D = 0;


				const uint32_t counterA_length = GetCounterBitSize(GET_COUNTER_A) / 8;
				uint8_t *counterA_raw = (uint8_t *)malloc(counterA_length);
				if(counterA_raw == NULL){
					log_error("Malloc of counter A");
					Error_Handler();
				}

				const uint32_t counterA_length_ZCS = GetCounterBitSize(GET_COUNTER_A) / 8;
				uint8_t *counterA_raw_ZCS = (uint8_t *)malloc(counterA_length_ZCS);
				if(counterA_raw == NULL){
					log_error("Malloc of counter A");
					Error_Handler();
				}

				memset(counterA_raw_ZCS, 0x0000, counterA_length_ZCS);

				while(1){
					// GET info about column which will be read out -> ZCA_X[]



					//result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_C_ZCS, counterA_raw);
					//tpx2_get_reg_256b(TPXA, GET_COUNTER_A_ZCS);

					tpx2_get_reg_256b(TPXA, GET_COUNTER_A_ZCS);
					result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_A_ZCS, counterA_raw, act_A);


					tpx2_get_reg_256b(TPXA, GET_COUNTER_B_ZCS);
					result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_B_ZCS, counterA_raw, act_B);

					tpx2_get_reg_256b(TPXA, GET_COUNTER_C_ZCS);
					result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_C_ZCS, counterA_raw, act_C);

					tpx2_get_reg_256b(TPXA, GET_COUNTER_D_ZCS);
					result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_D_ZCS, counterA_raw, act_D);


					for(uint8_t i = 0; i < 32; i++){
						act_A[i] = 0xAA;
					}
					ZCS_transform(GET_COUNTER_A_ZCS, counterA_raw, act_A, counterA_raw_ZCS);

					// needs to make transformation: 1. fill data from ZCS into buffer -> which is after able to transform
					//								 2. when is transforming recount index
					// if ZCS not using <=> act_X = 256 => transformation is not necessary!




					log_debug("...........");
					Delay(10);

					//result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_D_ZCS, counterA_raw);


					// seems that first need to read out GET_COUNTER_X_ZCS to "clear" the register, after transfer fine
					//result = tpx2_get_reg_256b(TPXA, GET_COUNTER_A_ZCS, ZCS_A);

					//result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_A_ZCS, ZCS_A);

					//tpx2_getcounterA_ZCS(counterA_raw, 17);
					Delay(10);
#endif
#if 0	// USE with all counters
					result = tpx2_get_reg_256b(TPXA, GET_COUNTER_B_ZCS, ZCS_B);
					if(result == 0){
						count_active_column(&act_B, ZCS_B);
						log_debug("act_B: %d", act_B);
					}else {
						log_error("ZCS B");
					}

					result = tpx2_get_reg_256b(TPXA, GET_COUNTER_C_ZCS, ZCS_C);
					if(result == 0){
						count_active_column(&act_C, ZCS_C);
						log_debug("act_C: %d", act_C);
					}else {
						log_error("ZCS C");
					}

					result = tpx2_get_reg_256b(TPXA, GET_COUNTER_D_ZCS, ZCS_D);
					if(result == 0){
						count_active_column(&act_D, ZCS_D);
						log_debug("act_D: %d", act_D);
					} else {
						log_error("ZCS D");
					}

				}
#endif
				break;
			default:
				readout_log_warning("%s UNKNOWN_COMMAND with Id: 0x%x", __FUNCTION__, commandId );
		}
		//
	}
	ret = nx_packet_release(data_packet);
	if (ret != NX_SUCCESS)
	{
	   Error_Handler();
	}

}

void CommandInternal( HW_CMD_ID cmdId )
{
	switch( cmdId ){
		case SENSOR_CONFIG_REGISTERS_UPDATE :
			log_debug("HW_SENSOR_CONFIG_REGISTERS_UPDATE");
			// TODO - aktivovat
			//Tpx3SetOutputBlockConfig( readOutConfig.sensorSetting.OutBlockConfig );
			//Tpx3SetPllConfig( readOutConfig.sensorSetting.PllConfig );
			break;
		case INTERNAL_DAC_UPDATE :
			log_debug("HW_INTERNAL_DAC_UPDATE");
			uint16_t tpx2DacValue;

			// VTPFINE -> VTHR form spaceix
			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VtpFine );
			//Tpx2SetDac( SET_VTPFINE, tpx2DacValue );
#if 1
			spx3_set_dac(spxReadOutConfig.global_config, VTHR, tpx2DacValue);	// update only register value
			spx3_global_config(spxReadOutConfig.global_config, NULL);			// need to update all DACS in SpacePix
#endif
#if defined(TPX_CMD)
			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VbiasPreampOn );
			Tpx2SetDac( SET_VBIAS_PREAMP_ON, tpx2DacValue );
			//Tpx2GetDac( GET_VBIAS_PREAMP_ON, VbiasPreampOn);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VbiasPreampOff );
			Tpx2SetDac( SET_VBIAS_PREAMP_OFF, tpx2DacValue );
			//Tpx2GetDac( GET_VBIAS_PREAMP_OFF, VbiasPreampOff);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VbiasLsOn );
			Tpx2SetDac( SET_VBIAS_LS_ON, tpx2DacValue );
			//Tpx2GetDac( GET_VBIAS_LS_ON, VbiasLsOn);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VbiasLsOff );
			Tpx2SetDac( SET_VBIAS_LS_OFF, tpx2DacValue );
			//Tpx2GetDac( GET_VBIAS_LS_OFF, VbiasLsOff);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VcascPreamp );
			Tpx2SetDac( SET_VCASC_PREAMP, tpx2DacValue );
			//Tpx2GetDac( GET_VCASC_PREAMP, VcascPreamp);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, Vfbk );
			Tpx2SetDac( SET_VFBK, tpx2DacValue );
			//Tpx2GetDac( GET_VFBK, Vfbk);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VthCoarse );
			Tpx2SetDac( SET_VTHCOARSE, tpx2DacValue );
			//Tpx2GetDac( GET_VTHCOARSE, VthCoarse);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VthFine );
			Tpx2SetDac( SET_VTHFINE, tpx2DacValue );
			//Tpx2GetDac( GET_VTHFINE, VthFine);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VbiasIkrum );
			Tpx2SetDac( SET_VBIAS_IKRUM, tpx2DacValue );
			//Tpx2GetDac( GET_VBIAS_IKRUM, VbiasIkrum);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VbiasDiscPmos );
			Tpx2SetDac( SET_VBIAS_DISCPMOS, tpx2DacValue );
			//Tpx2GetDac( GET_VBIAS_DISCPMOS, VbiasDiscPmos);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VbiasDiscNmos );
			Tpx2SetDac( SET_VBIAS_DISCNMOS, tpx2DacValue );
			//Tpx2GetDac( GET_VBIAS_DISCNMOS, VbiasDiscNmos);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VcascDisc );
			Tpx2SetDac( SET_VCASC_DISC, tpx2DacValue );
			//Tpx2GetDac( GET_VCASC_DISC, VcascDisc);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VbiasThs );
			Tpx2SetDac( SET_VBIAS_THS, tpx2DacValue );
			//Tpx2GetDac( GET_VBIAS_THS, VbiasThs);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, Vgnd );
			Tpx2SetDac( SET_VGND, tpx2DacValue );
			//Tpx2GetDac( GET_VGND, Vgnd);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VtpCoarse );
			Tpx2SetDac( SET_VTPCOARSE, tpx2DacValue );
			//Tpx2GetDac( GET_VTPCOARSE, VtpCoarse);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VtpFine );
			Tpx2SetDac( SET_VTPFINE, tpx2DacValue );
			//Tpx2GetDac( GET_VTPFINE, VtpFine);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VbiasSlvs );
			Tpx2SetDac( SET_VBIAS_SLVS, tpx2DacValue );
			//Tpx2GetDac( GET_VBIAS_SLVS, VbiasSlvs);

			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VcmSlvs );
			Tpx2SetDac( SET_VCM_SLVS, tpx2DacValue );
			//Tpx2GetDac( GET_VCM_SLVS, VcmSlvs);

			//UpdateTpx2Dacs(readOutConfig.feadbackTpx2Dacs, 27);	 // read all DACs value into feadbackTpx2Dacs
#endif
#if defined(SPX_CMD)
			// nezajima me VTHCOARSE
			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VthCoarse );
			log_debug("VTH_COARSE: %d", tpx2DacValue);
			//Tpx2SetDac( SET_VTHCOARSE, tpx2DacValue );
			//Tpx2GetDac( GET_VTHCOARSE, VthCoarse);

			// -> set VTHR on SpacePix3 // VTHR coresponds to VTH_FINE
			tpx2DacValue = Tpx2ReadDacFromRegisterArray( (uint16_t *)&readOutConfig.tpx2Cfg.dacs, VthFine );
			log_debug("VTH_FINE: %d", tpx2DacValue);
			//Spx3SetVthr(tpx2DacValue);

			//Tpx2SetDac( SET_VTHFINE, tpx2DacValue );
			//Tpx2GetDac( GET_VTHFINE, VthFine);
#endif
			break;
		case INTERNAL_DAC_BACK_READ :
			log_debug("HW_INTERNAL_DAC_BACK_READ");
			UpdateTpx2Dacs( readOutConfig.feadbackTpx2Dacs, NUM_OF_DACS);
			break;
		case TIMER_READ :
			log_debug("HW_TIMER_READ");
			break;
		case TIMER_SET :
			log_debug("HW_TIMER_SET");
			break;
		case RESET_MATRIX_SEQUENTIAL :
			log_debug("HW_RESET_MATRIX_SEQUENTIAL");
			break;
		case SET_TRIM_CONF_REGISTER :		// TODO rename <-> set TRIM and CONF
			log_debug("HW_SET_TRIM_CONF_REGISTER");

			/* NOTE: Timepix2 pixel registers are laid out as follows:
					      bits 0-4 .. adjustment (5b)			TRIM[4:0]
					      bit 5    .. unknown / unused (1b)		CONF[3:2]
					      bit 6    .. test (1b)					CONF[1]
					      bit 7    .. mask (1b)					CONF[0]
			 */
			// TrackLab default value is 0x1E : 0b0001 1110
			// 	-> TRIM[4:0] = 5'11110, CONF[3:0] = 3'000
#if defined (TPX_CMD)


				board_tpx2_set_shutter_counter(TPXA, 1);
				uint8_t *conf = (uint8_t *)malloc(SIZE_OF_COUNTER_CD_BYTES);
				if(conf == NULL){
					log_error("Malloc of conf");
					Error_Handler();
				}
				uint8_t *counterC_raw = (uint8_t *)malloc(SIZE_OF_COUNTER_CD_BYTES);
				if(counterC_raw == NULL){
					log_error("Malloc of counter C");
					Error_Handler();
				}

				memset(conf, 0x00, SIZE_OF_COUNTER_CD_BYTES);

				transform_set_counterCD_new(readOutConfig.tpx2Cfg.chipConfig, conf);
				tpx2_setconf(conf);	// CONF

#ifdef RELASE		// TODO -> debug vs. relase <-> maybe bug with TL and first empty display frame ??
				tpx2_getcounterC(counterC_raw);
#endif

				SAFE_FREE(conf);
				SAFE_FREE(counterC_raw);
#endif

				// TODO set TDAC for spacepix
				log_debug("TDAC settings: %d", readOutConfig.tpx2Cfg.chipConfig[0]);

#if defined (TPX_CMD)
				board_tpx2_set_shutter_counter(TPXA, 1);
				uint8_t *trim = (uint8_t *)malloc(SIZE_OF_COUNTER_AB_BYTES);
				if(trim == NULL){
					log_error("Malloc of trim");
					Error_Handler();
				}
				uint8_t *counterA_raw = (uint8_t *)malloc(SIZE_OF_COUNTER_AB_BYTES);
				if(counterA_raw == NULL){
					log_error("Malloc of counter A");
					Error_Handler();
				}
				memset(trim, 0x00, SIZE_OF_COUNTER_AB_BYTES);
				transform_set_counterAB_new(readOutConfig.tpx2Cfg.chipConfig, trim);
				tpx2_settrim(trim);	// TRIM
#ifdef RELASE	// TODO -> debug vs. relase <-> maybe bug with TL and first empty display frame ??
				tpx2_getcounterA(counterA_raw);
#endif
				SAFE_FREE(trim);
				SAFE_FREE(counterA_raw);
#endif
			break;
		case LOAD_COLUMN_TEST_PULSE_REGISTER :
			log_debug("HW_LOAD_COLUMN_TEST_PULSE_REGISTER");
			break;
		case READ_COLUMN_TEST_PULSE_REGISTER :
			log_debug("HW_READ_COLUMN_TEST_PULSE_REGISTER");
			break;
		case LOAD_PIXEL_REGISTER_CONFIGURATION :
			log_debug("HW_LOAD_PIXEL_REGISTER_CONFIGURATION");
			break;
		case READ_PIXEL_REGISTER_CONFIGURATION :
			log_debug("HW_READ_PIXEL_REGISTER_CONFIGURATION");
			break;
		case READ_PIXEL_MATRIX_SEQUENTIAL_SETTING :
			log_debug("HW_READ_PIXEL_MATRIX_SEQUENTIAL_SETTING");
			break;
		case READ_PIXEL_MATRIX_DATA_DRIVEN_SETTING :
			log_debug("HW_READ_PIXEL_MATRIX_DATA_DRIVEN_SETTING");
			break;
		case CHIP_ID_READ :
			log_debug("HW_CHIP_ID_READ");
			break;
		case OUTPUT_BLOCK_CONFIG_UPDATE :
			log_debug("HW_OUTPUT_BLOCK_CONFIG_UPDATE");
			break;
		case DIGITAL_TEST :
			log_debug("HW_DIGITAL_TEST");
			break;
		default :
			log_debug("HW_Unknown CmdID");
	}
}


