#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <user.h>

#include "app_netxduo.h"
#include "pinout.h"
#include "debug.h"
#include "board.h"
#include "tpx2_comm.h"
#include "meas.h"
#include "udp_cmd_handler.h"


//#define OPTIM_TEST

#define SAFE_FREE( x )		{ free( x ); x = NULL; }
#define     PACKET_SIZE     7000
#define     POOL_SIZE       ((sizeof(NX_PACKET) + PACKET_SIZE) * 60)
uint8_t txUdpPoolBuffer[POOL_SIZE];
NX_PACKET_POOL          pool_0;

char *DATA_MODE_STR[] = {
    "ToT10_ToA18",
    "ToT14_ToA14",
    "ContToT10_Event4",
    "ContToT14",
    "ContToA10",
    "ContToA14",
    "ContEvent10",
    "ContEvent14",
    "iToT10_ToA18",
    "iToT14_ToA14",
    "ContiToT10_Event4",
    "ContiToT14"
};

READOUT_STATUS readOutStatus;
READOUT_CONFIG readOutConfig;
extern SPX3_CONFIG spxReadOutConfig;

UINT UDP_packet_pool()
{
    /* Create a packet pool.  */
    UINT ret =  nx_packet_pool_create(&pool_0, "UDP Main Packet Pool", PACKET_SIZE, txUdpPoolBuffer, POOL_SIZE);
    return ret;
}

UINT SendUdpPacket( NX_UDP_SOCKET *udp_socket, uint8_t *dataTx, uint32_t dataTxLength, ULONG dst_ip_address, UINT port )
{
	UINT ret;
	NX_PACKET *tx_udp_packet;
    /* Allocate a packet.  */
    ret =  nx_packet_allocate(&pool_0, &tx_udp_packet, NX_UDP_PACKET, TX_WAIT_FOREVER);
	if (ret != NX_SUCCESS)
	{
		log_error("TX UDP allocation");
	   Error_Handler();
	}
	ret = nx_packet_data_append( tx_udp_packet, dataTx, dataTxLength, &pool_0, TX_WAIT_FOREVER);
	if (ret != NX_SUCCESS)
	{
		log_error("TX UDP append data");
	   Error_Handler();
	}
	ret = nx_udp_socket_send(udp_socket, tx_udp_packet, dst_ip_address, port);
	if (ret != NX_SUCCESS)
	{
		log_error("TX UDP send data");
	   Error_Handler();
	}
	return ret;
}



void send_spx3_pixel_matrix(NX_UDP_SOCKET *udp_socket, ULONG dst_ip_address, uint8_t *rx_data){

	uint8_t dataTx[6] = {0};
	uint8_t udp_buf[UDP_DBG_BUF_SIZE] = {0};
	// HEADER
	dataTx[5] = (NEW_FRAME_ESTABLISHED);  // [47 .. 44] -> HEADER
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, UDP_COMMAND_DATA_PORT );

	dataTx[5] = 0x0; dataTx[4] = 0x0; dataTx[3] = 0x0; dataTx[2] = 0x0; dataTx[1] = 0x0; dataTx[0] = 0x0;
	dataTx[5] = (PIXEL_MEASUREMENT_DATA);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, UDP_COMMAND_DATA_PORT );

	// DATA
	int y = 0;
	int x = 0;
	int j = 0;
	dataTx[5] = 0;
	uint64_t send_pixels = 0;
	// *2 bcs rx_data are uint8_t bud one pixel has 16bit
	for(int i = 0; i < NUM_OF_SEND_PIXELS_SPX3*2; i+=2){	// SEND measurment data // one loop means send one pixel, see doc.
		// Mereni1 - start
		dataTx[5] = 0x0; dataTx[4] = 0x0; dataTx[3] = 0x0; dataTx[2] = 0x0; dataTx[1] = 0x0; dataTx[0] = 0x0;
		uint16_t tmp_px = ((rx_data[i]) << 8) | rx_data[i+1];
		// x
		dataTx[5] = x;
		//y
		dataTx[4] = y;
		//log_debug("x: %d y: %d", x, y);

		uint32_t tmp = 0;
		tmp |= ((tmp_px) & 0x3FF)<< 0;  	//10A
		dataTx[1] |= ((tmp>>8) & 0x03);				// 2A
		dataTx[0] |= (tmp) &  0xFF;					// 8A

		// copy data to udp_buf
		udp_buf[j] =   x;	// x
		udp_buf[j+1] = y;	// y
		udp_buf[j+2] = dataTx[0];
		udp_buf[j+3] = dataTx[1];
		j+=4;
		send_pixels++;
	// Mereni1 - konec
	// Mereni2 - start
		if(j == UDP_DBG_BUF_SIZE){
			j = 0;
			SendUdpPacket( udp_socket, udp_buf, sizeof(udp_buf), dst_ip_address, UDP_COMMAND_DATA_PORT );
		}
		//}
		if(i == NUM_OF_SEND_PIXELS_SPX3-1){	// send whatever's left, if nothing left send nothing
			if(j != 0){
				SendUdpPacket( udp_socket, udp_buf, j, dst_ip_address, UDP_COMMAND_DATA_PORT );
			}
		}

		y++;
		if(y == 64){
			y = 0;
			x++;
			//log_debug("y: %d", y);
		}

	}

	// TAIL
	dataTx[5] = 0x0; dataTx[4] = 0x0; dataTx[3] = 0x0; dataTx[2] = 0x0; dataTx[1] = 0x0; dataTx[0] = 0x0;
	dataTx[5] = (END_OF_FRAME_TIMESTAMP_LSB);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, UDP_COMMAND_DATA_PORT );
}

static void send_pixel_matrix(NX_UDP_SOCKET *udp_socket, ULONG dst_ip_address, uint8_t datamode,
							  uint16_t *counterA_decode, uint16_t *counterB_decode, uint8_t *counterC_decode, uint8_t *counterD_decode)
{
	int j = 0;
	uint8_t dataTx[6] = {0};
	uint8_t udp_buf[UDP_BUF_SIZE] = {0};

	readOutStatus.start_time_udp_send = HAL_GetTick();

	// HEADER
	dataTx[5] = (NEW_FRAME_ESTABLISHED << 4);  // [47 .. 44] -> HEADER
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );

	dataTx[5] = 0x0; dataTx[4] = 0x0; dataTx[3] = 0x0; dataTx[2] = 0x0; dataTx[1] = 0x0; dataTx[0] = 0x0;
	dataTx[5] = (PIXEL_MEASUREMENT_DATA << 4);

	// DATA
	int y = 0;
	int x = 0;
	dataTx[5] = 0;
	uint64_t send_pixels = 0;
	for(int i = 0; i < NUM_OF_SEND_PIXELS_SPX3*2; i+=2){	// SEND measurment data // one loop means send one pixel, see doc.
// Mereni1 - start
		uint16_t tmpA = 0;
		uint16_t tmpB = 0;
		uint16_t tmpC = (counterC_decode[i] << 8 | counterC_decode[i+1]);
		uint8_t tmpD = 0;

		if((!tmpC)){
			x++;
			if(x == 64){
				x = 0;
				y++;
			}
			if(i == (NUM_OF_SEND_PIXELS_SPX3*2)-1){	// send whatever's left, if nothing left send nothing
				if(j != 0){
					SendUdpPacket( udp_socket, udp_buf, j, dst_ip_address, readOutConfig.data_port );
				}
			}
			continue;
		}
#ifdef OPTIM_TEST
/* From TrackLab
 * namespace tot10_toa18 {
	using ToT = Bitfield<18, 10, quint16>;
	using ToA = Bitfield<0, 18, quint32>;
	using CoordX = Bitfield<36, 8, quint16>;
	using CoordY = Bitfield<28, 8, quint16>;
	}
 */

		uint16_t *p_tmp47_32 = (uint16_t *)&dataTx[4];
		uint32_t *p_tmp31_0 = (uint32_t *)&dataTx[0];

		uint16_t tmp47_32 = (PIXEL_MEASUREMENT_DATA << 12); 	// [15..12] -> [47..44]
		tmp47_32 |= (x & 0xFF) << 4;							// [11 .. 4] -> [43..36]
		tmp47_32 |= ((y >> 4) & 0x0F);							// [0 .. 3] -> [35..32]
		uint32_t tmp31_0 = (y & 0x0F) << 28;					// [31 .. 28] -> [31..28]
#else
		dataTx[5] = 0x0; dataTx[4] = 0x0; dataTx[3] = 0x0; dataTx[2] = 0x0; dataTx[1] = 0x0; dataTx[0] = 0x0;
		dataTx[5] = (PIXEL_MEASUREMENT_DATA << 4);


		dataTx[5] = (dataTx[5] & 0xF0) |  (x >> 4);
		dataTx[4] = (dataTx[4] & 0x0F) | ((x & 0x0F) << 4);
		// y
		dataTx[4] = (dataTx[4] & 0xF0) |  (y >> 4);
		dataTx[3] = (dataTx[3] & 0x0F) | ((y & 0x0F) << 4);
#endif
		uint32_t tmp = 0;

		// convert data depends on datamode
		switch (datamode) {
			case iToT10_ToA18:
			case ToT10_ToA18:
#ifdef OPTIM_TEST
				tmp31_0 = ((tmpA)  & 0x3FF)<<  0; //10A
				tmp31_0 |= ((tmpD)  & 0x0F) << 10; //4D
				tmp31_0 |= ((tmpC)  & 0x0F) << 14; //4C
				tmp31_0 |= ((tmpB)  & 0x3FF)<< 18; //10B

				*p_tmp47_32 = tmp47_32;
				*p_tmp31_0 = tmp31_0;
#else
				tmp |= ((tmpC) & 0x3FF)<< 0;  	//10A
				dataTx[3] |= (tmp>>6) & 0x0F;			// 4A
				dataTx[2] |= (tmp & 0x3F) << 2;			// 6A

#endif
#if 0	// For display circle into TL
				int t = (x-128)*(x-128) + (y-90)*(y-90);
				if(t <= 2500 && t >= 1600){
					dataTx[3] = (dataTx[3] | 0xf);
					dataTx[2] = 0xff;
					dataTx[1] = 0xff;
					dataTx[0] = 0xff;
				}else {
					dataTx[3] = ((dataTx[3] & 0xF0));
					dataTx[2] = 0x0;
					dataTx[1] = 0x0;
					dataTx[0] = 0x0;
				}
#endif
				break;
			case iToT14_ToA14:
			case ToT14_ToA14:
				tmp = 0;
				tmp |= ((tmpA) & 0x3FF)<< 0;  //10A
				tmp |= ((tmpD) & 0x0F) << 10; //4D
				tmp |= ((tmpC) & 0x0F) << 14; //4C
				tmp |= ((tmpB) & 0x3FF)<< 18; //10B

				dataTx[3] |= ((tmp>>10) & 0x0F);		// 4D
				dataTx[2] |= ((tmp>> 2) & 0xFF);		// 8A
				dataTx[1] |= ((tmp>> 0) & 0x03) << 6;	// 2C
				dataTx[1] |= ((tmp>>14) & 0x0F) << 2;	// 2C
				dataTx[1] |= ((tmp>>26) & 0x03);		// 2B
				dataTx[0] |= ((tmp>>18) & 0xFF);		// 8B
				break;
			case ContiToT14:
			case ContiToT10_Event4:
			case ContToT14:
			case ContToA14:
			case ContEvent14:
				tmp = 0;
				tmp |= ((tmpA) & 0x3FF)<< 0;  	//10A
				tmp |= ((tmpD) & 0x0F) << 10; 		//4D

				dataTx[1] |= (tmp >> 10) << 2;				//4D
				dataTx[1] |= (tmp >> 8) & 0x03;				//2A
				dataTx[0] |= (tmp >> 0) & 0xFF;				//8A
				break;
			case ContToT10_Event4:
				tmp = 0;
				tmp |= ((tmpA) & 0x3FF)<< 0;  	//10A
				tmp |= ((tmpD) & 0x0F) << 10; 	//4D

				dataTx[1] |= (tmp >> 10) << 2;				//4D
				dataTx[1] |= (tmp >> 8) & 0x03;				//2A
				dataTx[0] |= (tmp >> 0) & 0xFF;				//8A
				break;
			case ContToA10:
			case ContEvent10:
				tmp = 0;
				tmp |= ((tmpA) & 0x3FF)<< 0;  	//10A

				dataTx[1] |= ((tmp>>8) & 0x03);				// 2A
				dataTx[0] |= (tmp) &  0xFF;					// 8A
				break;
			// TODO, dont know what iToT means

			default:
				break;
		}
		// Zeros decode counters
		counterA_decode[i] = 0;
		counterB_decode[i] = 0;
		counterC_decode[i] = 0;
		counterD_decode[i] = 0;

		x++;
		if(x == 64){
			x = 0;
			y++;
		}

#if 1	// Some compresion -> Dont send data with zero's and send data in chunk of UDP_BUF_SIZE
		//if(tmp != 0){
		udp_buf[j] =   dataTx[0];
		udp_buf[j+1] = dataTx[1];
		udp_buf[j+2] = dataTx[2];
		udp_buf[j+3] = dataTx[3];
		udp_buf[j+4] = dataTx[4];
		udp_buf[j+5] = dataTx[5];
		j+=6;
		send_pixels++;
// Mereni1 - konec
// Mereni2 - start
		if(j == UDP_BUF_SIZE){
			j = 0;
			SendUdpPacket( udp_socket, udp_buf, sizeof(udp_buf), dst_ip_address, readOutConfig.data_port );
		}
		//}
		if(i == (NUM_OF_SEND_PIXELS_SPX3*2)-1){	// send whatever's left, if nothing left send nothing
			if(j != 0){
				SendUdpPacket( udp_socket, udp_buf, j, dst_ip_address, readOutConfig.data_port );
			}
		}
// Mereni2 - konec
#endif
	}

	// TAIL
	dataTx[5] = 0x0; dataTx[4] = 0x0; dataTx[3] = 0x0; dataTx[2] = 0x0; dataTx[1] = 0x0; dataTx[0] = 0x0;

	// END OF AQ.
	dataTx[0x0] = (uint8_t)((readOutStatus.acq_start_time >> 0) & 0xFF);
	dataTx[0x1] = (uint8_t)((readOutStatus.acq_start_time >> 8) & 0xFF);
	dataTx[0x2] = (uint8_t)((readOutStatus.acq_start_time >> 16) & 0xFF);
	dataTx[0x3] = (uint8_t)((readOutStatus.acq_start_time >> 24) & 0xFF);
	dataTx[0x4] = (uint8_t)((readOutStatus.acq_start_time >> 32) & 0xFF);
	dataTx[0x5] = (START_OF_FRAME_TIMESTAMP_LSB<<4) | (uint8_t)((readOutStatus.acq_start_time >> 40) & 0xF);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );

	dataTx[5] = 0x0; dataTx[4] = 0x0; dataTx[3] = 0x0; dataTx[2] = 0x0; dataTx[1] = 0x0; dataTx[0] = 0x0;

	dataTx[5] = (START_OF_FRAME_TIMESTAMP_MSB << 4);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );

	readOutStatus.acq_end_time = HAL_GetTick();
	dataTx[0x0] = (uint8_t)((readOutStatus.acq_end_time >> 0) & 0xFF);
	dataTx[0x1] = (uint8_t)((readOutStatus.acq_end_time >> 8) & 0xFF);
	dataTx[0x2] = (uint8_t)((readOutStatus.acq_end_time >> 16) & 0xFF);
	dataTx[0x3] = (uint8_t)((readOutStatus.acq_end_time >> 24) & 0xFF);
	dataTx[0x4] = (uint8_t)((readOutStatus.acq_end_time >> 32) & 0xFF);
	dataTx[0x5] = (END_OF_FRAME_TIMESTAMP_LSB<<4) | (uint8_t)((readOutStatus.acq_end_time >> 40) & 0xF);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );

	dataTx[5] = 0x0; dataTx[4] = 0x0; dataTx[3] = 0x0; dataTx[2] = 0x0; dataTx[1] = 0x0; dataTx[0] = 0x0;

	dataTx[5] = (END_OF_FRAME_TIMESTAMP_MSB << 4);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );

	// SEND number of lost pixels -> 0. Same as Katherine protocol for Tmepix 2
	dataTx[5] = (NUMBER_OF_LOST_PIXELS << 4);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );

	// TAIL
	uint64_t totalPixelCounter = send_pixels;
	dataTx[0x0] = (uint8_t)((totalPixelCounter >> 0) & 0xFF);
	dataTx[0x1] = (uint8_t)((totalPixelCounter >> 8) & 0xFF);
	dataTx[0x2] = (uint8_t)((totalPixelCounter >> 16) & 0xFF);
	dataTx[0x3] = (uint8_t)((totalPixelCounter >> 24) & 0xFF);
	dataTx[0x4] = (uint8_t)((totalPixelCounter >> 32) & 0xFF);
	dataTx[0x5] = (CURRENT_FRAME_FINISHED<<4) | (uint8_t)((totalPixelCounter >> 40) & 0xF);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );

	readOutStatus.end_time_udp_send = HAL_GetTick();
	readOutStatus.diff_time_udp_send = readOutStatus.end_time_udp_send - readOutStatus.start_time_udp_send;
	log_debug("UDP send time [ms] = %d. Size of data [B] = %d", readOutStatus.diff_time_udp_send, send_pixels*6);
}

void read_out_counters(counterA_raw, counterB_raw, counterC_raw, counterD_raw){
	tpx2_getcounterA(counterA_raw);
	tpx2_getcounterB(counterB_raw);
	tpx2_getcounterC(counterC_raw);
	tpx2_getcounterD(counterD_raw);
}
void MeasurementStop( NX_UDP_SOCKET *udp_socket, ULONG dst_ip_address){
	uint8_t dataTx[6] = {0};
	// HEADER
	dataTx[5] = (NEW_FRAME_ESTABLISHED << 4);  // [47 .. 44] -> HEADER
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );

	// END OF AQ.
	dataTx[5] = (START_OF_FRAME_TIMESTAMP_LSB << 4);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );
	dataTx[5] = (START_OF_FRAME_TIMESTAMP_MSB << 4);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );
	dataTx[5] = (END_OF_FRAME_TIMESTAMP_LSB << 4);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );
	dataTx[5] = (END_OF_FRAME_TIMESTAMP_MSB << 4);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );

	// SEND number of lost pixels -> 0. Same as Katherine protocol for Tmepix 2
	dataTx[5] = (NUMBER_OF_LOST_PIXELS << 4);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );

	// TAIL: ABORT
	dataTx[5] = (MEASUREMENT_ABORTED_NOTICE << 4);
	SendUdpPacket( udp_socket, dataTx, sizeof(dataTx), dst_ip_address, readOutConfig.data_port );
}

uint8_t  MeasurementProcess( NX_UDP_SOCKET *udp_socket, ULONG dst_ip_address, uint8_t datamode, uint8_t col_trigger_en, uint8_t *counterA_raw, uint8_t *counterB_raw, uint8_t *counterC_raw, uint8_t *counterD_raw,
		uint16_t *counterA_decode_get, uint16_t *counterB_decode_get, uint8_t *counterC_decode_get, uint8_t *counterD_decode_get)
{
	uint8_t result = TPX_OK;
	const uint32_t counterA_length_ZCS = GetCounterBitSize(GET_COUNTER_A) / 8;
	const uint32_t counterC_length_ZCS = GetCounterBitSize(GET_COUNTER_C) / 8;

	uint8_t act_A[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
	uint8_t act_B[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
	uint8_t act_C[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
	uint8_t act_D[TPX2_PKT_PAYLOAD256_LENGTH] = {0};
#ifdef ZCS
	uint8_t *counterA_raw_ZCS = (uint8_t *)malloc(counterA_length_ZCS);
	if(counterA_raw_ZCS == NULL){
		log_error("Malloc of counter A");
		Error_Handler();
	}
	memset(counterA_raw_ZCS, 0x00, counterA_length_ZCS);

	uint8_t *counterC_raw_ZCS = (uint8_t *)malloc(counterC_length_ZCS);
	if(counterC_raw_ZCS == NULL){
		log_error("Malloc of counter A");
		Error_Handler();
	}
	memset(counterC_raw_ZCS, 0x00, counterC_length_ZCS);
#endif
// reset counters before fisrt frame -> Done when TRIM and CONF is setting
#if 0
	if(readOutStatus.first_frame){ 			// first frame reset counter using -> read out corretly all counters
		tpx2_getcounterA(counterA_raw);
		tpx2_getcounterB(counterB_raw);
		tpx2_getcounterC(counterC_raw);
		tpx2_getcounterD(counterD_raw);
		readOutStatus.first_frame = false;
		return TPX_OK;
	}
#endif

	if(readOutStatus.numberOfFrames < readOutConfig.numberOfFrames){

		readOutStatus.measurementInProgress = true;
		readOutStatus.numberOfFrames++;
		log_debug("Num. of frames: %d", (readOutStatus.numberOfFrames));
		// INIT COUNTERs on zero's
#if 0
		memset(counterA_decode_get, 0x0000, SIZE_OF_MATRIX);
		memset(counterB_decode_get, 0x0000, SIZE_OF_MATRIX);
		memset(counterC_decode_get, 0x00, SIZE_OF_MATRIX);
		memset(counterD_decode_get, 0x00, SIZE_OF_MATRIX);
#endif
		// select DATA MODE
		readOutStatus.acq_start_time = HAL_GetTick();
		log_debug("Data Mode: %s", DATA_MODE_STR[datamode]);
		switch (datamode) {
			case iToT14_ToA14:
			case iToT10_ToA18:
			case ToT14_ToA14:
			case ToT10_ToA18:
				//log_debug("Mode: Simultaneous 18bits. Col. trigger: 0x%x", readOutConfig.col_trigger_en);
				// GET RAW COUNTER
#ifdef ZCS
				tpx2_get_reg_256b(TPXA, GET_COUNTER_A_ZCS);
				result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_A_ZCS, counterA_raw, act_A);
				ZCS_transform(GET_COUNTER_A_ZCS, counterA_raw, act_A, counterA_raw_ZCS);
				transform_10bit_matrix_get_optim(counterA_raw_ZCS, counterA_decode_get);
				memset(counterA_raw_ZCS, 0x00, counterA_length_ZCS);

				tpx2_get_reg_256b(TPXA, GET_COUNTER_B_ZCS);
				result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_B_ZCS, counterB_raw, act_B);
				ZCS_transform(GET_COUNTER_B_ZCS, counterB_raw, act_B, counterA_raw_ZCS);
				transform_10bit_matrix_get_optim(counterA_raw_ZCS, counterB_decode_get);
				SAFE_FREE(counterA_raw_ZCS);


				tpx2_get_reg_256b(TPXA, GET_COUNTER_C_ZCS);
				result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_C_ZCS, counterC_raw, act_C);
				ZCS_transform(GET_COUNTER_C_ZCS, counterC_raw, act_C, counterC_raw_ZCS);
				transform_4bit_matrix_get_optim(counterC_raw_ZCS, counterC_decode_get);
				memset(counterC_raw_ZCS, 0x00, counterC_length_ZCS);

				tpx2_get_reg_256b(TPXA, GET_COUNTER_D_ZCS);
				result = tpx2_get_reg_256b_ZCS(TPXA, GET_COUNTER_D_ZCS, counterD_raw, act_D);
				ZCS_transform(GET_COUNTER_D_ZCS, counterD_raw, act_D, counterC_raw_ZCS);
				transform_4bit_matrix_get_optim(counterC_raw_ZCS, counterD_decode_get);
				SAFE_FREE(counterC_raw_ZCS);



#endif

				readOutStatus.start_time_readout_counters = HAL_GetTick();
#if 0
				tpx2_getcounterA(counterA_raw);
				tpx2_getcounterB(counterB_raw);
				tpx2_getcounterC(counterC_raw);
				tpx2_getcounterD(counterD_raw);

#endif
				//spx3_global_config(spxReadOutConfig.global_config, NULL);
				spx3_data_readout(counterC_decode_get, readOutConfig.acqTime);
				readOutStatus.end_time_readout_counters = HAL_GetTick();

				// TRANSFORM GET
#if 0
				readOutStatus.start_time_transform = HAL_GetTick();
				transform_10bit_matrix_get_optim(counterA_raw, counterA_decode_get);
				transform_10bit_matrix_get_optim(counterB_raw, counterB_decode_get);
				transform_4bit_matrix_get_optim(counterC_raw, counterC_decode_get);
				transform_4bit_matrix_get_optim(counterD_raw, counterD_decode_get);
				readOutStatus.end_time_transform = HAL_GetTick();

#endif

				//send_spx3_pixel_matrix(udp_socket, dst_ip_address, counterC_decode_get);
				send_pixel_matrix(udp_socket, dst_ip_address, datamode, counterA_decode_get, counterB_decode_get, counterC_decode_get, counterD_decode_get);
				break;

			case ContiToT10_Event4:
			case ContiToT14:
			case ContToT10_Event4:
			case ContToT14:
			case ContToA14:
				//log_debug("Mode: Continuous 14 bits");
				//log_debug("Mode: ContToT14");
#if 1
				// choose 1th counters: A,D
				board_tpx2_set_shutter_counter(TPXA, 0);
				Delay(1);
				Delay(readOutConfig.acqTime);
				board_tpx2_set_shutter_counter(TPXA, 1);		// choose 1th counters: A,D
				Delay(1);
				//while(tpx2_get_readready() != 0){} 	// if 0, selected ToT counters have completed counting, after can read out value
				board_tpx2_set_shutter_counter(TPXA, 0);
				readOutStatus.start_time_readout_counters = HAL_GetTick();
				tpx2_getcounterA(counterA_raw);
				tpx2_getcounterD(counterD_raw);
				readOutStatus.end_time_readout_counters = HAL_GetTick();

				//board_tpx2_set_shutter_counter(TPXA, 0);
				transform_10bit_matrix_get_optim(counterA_raw, counterA_decode_get);
				transform_4bit_matrix_get_optim(counterD_raw, counterD_decode_get);

				send_pixel_matrix(udp_socket, dst_ip_address, datamode, counterA_decode_get, counterB_decode_get, counterC_decode_get, counterD_decode_get);

				readOutStatus.numberOfFrames++;
				if(readOutStatus.numberOfFrames > readOutConfig.numberOfFrames){
					readOutStatus.measurementInProgress = false;
					break;
				}

				board_tpx2_set_shutter_counter(TPXA, 1);		// choose 1th counters: A,D
				Delay(readOutConfig.acqTime);
				board_tpx2_set_shutter_counter(TPXA, 0);
				//Active low boolean flag to indicate that the ToT counters have completed counting and can be changed to readout mode after a transition in the SHUTTERn/COUNTERSEL pin.
				Delay(1);
				board_tpx2_set_shutter_counter(TPXA, 1);
				//while(tpx2_get_readready() != 0){} 	// if 0, selected ToT counters have completed counting, after can read out value
				tpx2_getcounterB(counterB_raw);
				tpx2_getcounterC(counterC_raw);
				readOutStatus.start_time_transform = HAL_GetTick();
				transform_10bit_matrix_get_optim(counterB_raw, counterB_decode_get);
				transform_4bit_matrix_get_optim(counterC_raw, counterC_decode_get);
				readOutStatus.end_time_transform = HAL_GetTick();

				// TODO tricky part how to send decode counters, need to make more clear
				send_pixel_matrix(udp_socket, dst_ip_address, datamode, counterB_decode_get, counterA_decode_get, counterD_decode_get, counterC_decode_get);
#endif
				break;
			case ContEvent10:
			case ContToA10:
				//log_debug("Mode: Continuous. 10 bits");
#if 1
				// choose 1th counters: A,D
				board_tpx2_set_shutter_counter(TPXA, 0);
				Delay(1);
				Delay(readOutConfig.acqTime);
				board_tpx2_set_shutter_counter(TPXA, 1);		// choose 1th counters: A,D
				Delay(1);
				//while(tpx2_get_readready() != 0){} 	// if 0, selected ToT counters have completed counting, after can read out value
				board_tpx2_set_shutter_counter(TPXA, 0);
				readOutStatus.start_time_readout_counters = HAL_GetTick();
				tpx2_getcounterA(counterA_raw);
				readOutStatus.end_time_readout_counters = HAL_GetTick();

				//board_tpx2_set_shutter_counter(TPXA, 0);
				transform_10bit_matrix_get_optim(counterA_raw, counterA_decode_get);
				send_pixel_matrix(udp_socket, dst_ip_address, datamode, counterA_decode_get, counterB_decode_get, counterC_decode_get, counterD_decode_get);

				readOutStatus.numberOfFrames++;
				if(readOutStatus.numberOfFrames > readOutConfig.numberOfFrames){
					readOutStatus.measurementInProgress = false;
					break;
				}

				board_tpx2_set_shutter_counter(TPXA, 1);		// choose 1th counters: A,D
				Delay(readOutConfig.acqTime);
				board_tpx2_set_shutter_counter(TPXA, 0);
				//Active low boolean flag to indicate that the ToT counters have completed counting and can be changed to readout mode after a transition in the SHUTTERn/COUNTERSEL pin.
				Delay(10);
				board_tpx2_set_shutter_counter(TPXA, 1);
				//while(tpx2_get_readready() != 0){} 	// if 0, selected ToT counters have completed counting, after can read out value
				tpx2_getcounterB(counterB_raw);
				//board_tpx2_set_shutter_counter(TPXA, 1);
				readOutStatus.start_time_transform = HAL_GetTick();
				transform_10bit_matrix_get_optim(counterB_raw, counterB_decode_get);
				readOutStatus.end_time_transform = HAL_GetTick();

				// TODO tricky part how to send decode counters, need to make more clear
				send_pixel_matrix(udp_socket, dst_ip_address, datamode, counterB_decode_get, counterA_decode_get, counterC_decode_get, counterD_decode_get);

#endif
				break;
			case ContEvent14:		// how to read from continous read write mode? -> this is not correct
				log_debug("Mode: ContEvent14");
				readOutStatus.eq_frames = 1;

				if(readOutStatus.eq_frames%2 == 0){
					log_debug("KURVA");
#if 0
					board_tpx2_set_shutter_counter(TPXA, 1);		// choose 1th counters: A,D
					Delay(readOutConfig.acqTime*10);
					board_tpx2_set_shutter_counter(TPXA, 0);
#endif
					process_shutter(readOutConfig.acqTime);

					//Active low boolean flag to indicate that the ToT counters have completed counting and can be changed to readout mode after a transition in the SHUTTERn/COUNTERSEL pin.
					while(tpx2_get_readready() != 0){} 	// if 0, selected ToT counters have completed counting, after can read out value
					tpx2_getcounterB(counterB_raw);
					tpx2_getcounterC(counterC_raw);

					transform_10bit_matrix_get_optim(counterB_raw, counterA_decode_get);
					transform_4bit_matrix_get_optim(counterC_raw, counterD_decode_get);

				} else {
#if 0
					board_tpx2_set_shutter_counter(TPXA, 0);		// choose 2th counters: B,C
					Delay(readOutConfig.acqTime);
					board_tpx2_set_shutter_counter(TPXA, 1);		// choose 1th counters: A,D
#endif
#if defined(TPX_CMD)
					process_shutter(readOutConfig.acqTime);

					while(tpx2_get_readready() != 0){} 	// if 0, selected ToT counters have completed counting, after can read out value
					readOutStatus.start_time_readout_counters = HAL_GetTick();
					tpx2_getcounterA(counterA_raw);
					tpx2_getcounterD(counterD_raw);
					readOutStatus.end_time_readout_counters = HAL_GetTick();
					readOutStatus.start_time_transform = HAL_GetTick();
					transform_10bit_matrix_get_optim(counterA_raw, counterA_decode_get);
					transform_4bit_matrix_get_optim(counterD_raw, counterD_decode_get);
					readOutStatus.end_time_transform = HAL_GetTick();
#endif
#if defined (SPX_CMD)
					// MEAS !!!
					// TODO readout the data
					//
					for(uint32_t i = 0 ; i < MATRIX_SIZE; i++){
						counterA_decode_get[i] = 0;
						counterB_decode_get[i] = 0;
						counterC_decode_get[i] = 0;
						counterD_decode_get[i] = 0;
					}

					// Meas !
					// RX Pixel Matrix Config
#if 0
					const uint32_t rx_pattern_length = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE+1);
					uint16_t *rx_pattern = (uint16_t *)malloc( rx_pattern_length );
					memset( rx_pattern, 0x00, rx_pattern_length );
					if( rx_pattern == NULL ){
						log_error("counter_rx_pattern == NULL");
						return SPX_FAILED;
					}
#endif

					//SAFE_FREE(rx_pattern);

#endif

				}
				// TODO
				send_pixel_matrix(udp_socket, dst_ip_address, datamode, counterA_decode_get, counterB_decode_get, counterC_decode_get, counterD_decode_get);
				break;

			default:
				log_error("Measprocess in acq. thread wrong unknow format of data mode");
				break;
		}
		readOutStatus.diff_time_transform = readOutStatus.end_time_transform - readOutStatus.start_time_transform;
		readOutStatus.diff_time_readout_counters = readOutStatus.end_time_readout_counters - readOutStatus.start_time_readout_counters;
		log_debug("Transform time [ms] = %d", readOutStatus.diff_time_transform);
		log_debug("Readout time [ms] = %d", readOutStatus.diff_time_readout_counters);
#if 1
		SAFE_FREE(counterA_raw);
		SAFE_FREE(counterB_raw);
		SAFE_FREE(counterC_raw);
		SAFE_FREE(counterD_raw);
#endif
#if 0
		SAFE_FREE(counterA_raw_ZCS);
		SAFE_FREE(counterC_raw_ZCS);
#endif
	}
	if(readOutStatus.numberOfFrames == readOutConfig.numberOfFrames){
		log_debug("End of ACQ");
		//tpx2_init(TPXA, NULL);
		readOutStatus.measurementInProgress = false;
	}
	return TPX_OK;
}


#if 0
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
#if 1
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
#if 1 //LOAD CONF
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
#if 1 // LOAD TRIM
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
#endif

#if 0
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
				readOutConfig.temperature = 33;
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
				float temp = 0;
				Tpx2GetTemp(&temp);
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

				board_tpx2_set_shutter_counter(TPXA, 1);					// TPX2 manual pg. 33:  is best to set the IO pad SHUTTERn = 1 while changing the OMR
				//omrValue[0] = 0x734c;		// change to some own value
				status = tpx2_set_reg_16b(TPXA, SET_OMR, omrValue);			// SET OMR VALUE
				uint32_t get_omr = 0;
				status = tpx2_get_reg_16b(TPXA, GET_OMR, &get_omr);			// GET OMR VALUE
				log_debug("CMD_TPX2_SET_OMR");
				log_debug("GET OMR, datamode not set: 0x%x", get_omr);
				break;

			case CMD_ACQ_START :	// TODO	start measurment -> ONLY FOR TPX3??
				//uint8_t data_mode = dataRx[0];				// data mode is SET in CMD_TPX2_SET_OMR
				readOutConfig.data_mode = dataRx[0];
				uint8_t zero_suppresion = dataRx[1];		// if 1 -> ZCS enable // TODO
				set_data_mode(readOutConfig.data_mode);
				if(readOutConfig.data_mode > 0x1 && readOutConfig.data_mode < 0x8){
					readOutConfig.conti_mode = true;
				} else {
					readOutConfig.conti_mode = false;
				}

				uint32_t get_datamode = 0;
				status = tpx2_get_reg_16b(TPXA, GET_OMR, &get_datamode);			// GET OMR VALUE
				log_debug("ACQ START: GET OMR: 0x%x", get_datamode);
				// NOTE: does not produce ack	--> Measurement data…

				log_debug("CMD_ACQ_START");
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

					board_start_MCLOCK(TPXA, totFreq); // TODO set prescaler -> set MCLOCK freq. // need to set MCLOCK prescaler in MCU
					log_debug("totFreq number: %d", totFreq);
					status = tpx2_set_reg_8b(TPXA, SET_TOAFREQSEL, toaFreq); // default is 0x1E <-> ToA bypass from ToT (MCLOCK)

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
				log_debug("ECHO from TPX2 Lite");
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

				log_debug("UDP TEST SPEED");
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
				log_debug("DBG MODE");
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

				}
				break;

			case CMD_DBG_TEST_MODE:
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
#endif
