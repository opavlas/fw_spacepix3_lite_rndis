/*
 * tpx2_comm.c
 *
 *  Created on: Sep 2, 2022
 *      Author: opavl
 */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "board.h"
#include "pinout.h"
#include "tpx2_comm.h"
#include "debug.h"
#include "udp_cmd_handler.h"

//#include "config.h"

#define ARRAY_LENGTH(x)		(sizeof(x)/sizeof(x[0]))

#define DEFAULT_VBIAS_SLVS_SET			0x07
#define DEFAULT_VCM_SLVS_SET			0x08
#define NOMINAL_VBIAS_SLVS_SET			0xc8
#define NOMINAL_VCM_SLVS_SET			0x80
#define MAX_VBIAS_SLVS_SET			0xff
#define MAX_VCM_SLVS_SET			0xff
TIMEPIX2_CONFIG timepix2DefaultCfg = {
		.VBIAS_SLVS = NOMINAL_VBIAS_SLVS_SET,
		.VCM_SLVS = NOMINAL_VCM_SLVS_SET
};

char *DAC_NAME_STR[] = {
    "vbias_preamp_on",
    "vbias_preamp_off",
    "vbias_ls_on",
    "vbias_ls_off",
    "vcasc_preamp",
    "vfbk",
    "vthcoarse",
    "vthfine",
    "vbias_ikrum",
    "vbias_discpmos",
    "vbias_discnmos",
    "vcasc_dis",
    "vbias_ths",
    "vgnd",
    "vtpcoarse",
    "vtpfine",
    "vbias_slvls",
    "vcm_slvs",
    "vbias_res",
    "vdd23",
    "vdd13",
    "vdda23",
	"vdda13",
	"bias_dac",
	"bias_dac_cas",
	"vbg",
	"vbg_temp"
};

bool tx_done = true;
bool rx_done = true;

void tpx2_set_powerstate( TIMEPIX_ID tpxId, uint8_t state )
{
	board_tpx2_set_pwr_en1v2( tpxId, state );
}

int tpx2_init( TIMEPIX_ID tpxId, TIMEPIX2_CONFIG *timepix2Cfg )
{
	if( timepix2Cfg == NULL ){
		timepix2Cfg = &timepix2DefaultCfg;
	}
	board_stop_mclock();		// recommended by P. Burian
	tpx2_basic_configuration(tpxId);
	int num_of_init = 300;
	int retryCounter = num_of_init;
	while( retryCounter-- )
	{
		uint8_t arr_vbias_slvs_set[1] = {timepix2Cfg->VBIAS_SLVS};
		uint8_t arr_vcm_slvs_set[1] = {timepix2Cfg->VCM_SLVS};
		uint32_t vbias_slvs_get = 0;
		uint32_t vcm_slvs_get = 0;

		tpx2_set_reg_8b( tpxId, SET_VBIAS_SLVS, arr_vbias_slvs_set);		// nastaveni proudu SLVS
		tpx2_set_reg_8b( tpxId, SET_VCM_SLVS, arr_vcm_slvs_set);			// nastaveni offestu proudu SLVS
		tpx2_get_reg_8b( tpxId, GET_VBIAS_SLVS, &vbias_slvs_get);
		tpx2_get_reg_8b( tpxId, GET_VCM_SLVS, &vcm_slvs_get);

		log_debug("GET_VBIAS_SLVS: 0x%x | 0x%x", timepix2Cfg->VBIAS_SLVS, vbias_slvs_get);
		if( (vbias_slvs_get == timepix2Cfg->VBIAS_SLVS) && (vcm_slvs_get == timepix2Cfg->VCM_SLVS) ){
			uint32_t OMR = 0;
			tpx2_get_reg_16b(TPXA, GET_OMR, &OMR);
			if(OMR == DEFAULT_OMR){
				log_debug("OMR: 0x%x. Num of init: %d", OMR, num_of_init - retryCounter);
				board_start_mclock(Frequency10);
				return TPX_OK;
			}
		}
	}
	return TPX_FAILED;
}

uint8_t tpx2_init_dig(void){
	uint8_t ret = TPX_OK;
#ifdef TPX2_INIT
	bool init_pass = false;
	while(init_pass == false){
	//for(int i = 0; i < 10; i++){
		if(tpx2_init(TPXA, NULL) != TPX_OK){					// INIT TPX2
			log_error("Timpepix 2 - Init fail.");
			tpx2_set_powerstate(TPXA, 0);
			Delay(500);
			continue;
		}
		log_debug("Timpepix 2 - Init - passed.");
		init_pass = true;
	}
#endif
#ifdef DIGITAl_TEST
	uint32_t tpx2_valid_iter = 0;
	uint32_t num_of_test = 1;		// SET number of DIGITAL TEST
	uint8_t init = 0;
	while(init == 0){
#if 1
		if(tpx2_init(TPXA, NULL) != TPX_OK){					// INIT TPX2
			log_error("Timpepix 2 - Init - fail");
			tpx2_set_powerstate(TPXA, 0);
			Delay(500);
			num_of_test++;
			continue;
		}
		log_debug("Timpepix 2 - Init - passed");
#endif
	#if 1
		board_tpx2_set_shutter_counter(TPXA, 1);
		if(tpx2_digital_test(TPXA) == TPX_OK){					// DIGITAL TEST
			log_debug("Timpepix 2 - Digital test - PASS");
			tpx2_valid_iter += 1;
			init = 1;
		}else {
			log_error("Timpepix 2 - Digital test - failed");
			tpx2_set_powerstate(TPXA, 0);
			Delay(100);
		}
	#endif
	}
	log_debug("---------------------\n");
	log_debug("Timpepix 2 - Digital test:\n");
	log_debug("Num. of test: %d\n", num_of_test);
	log_debug("Digital test: PASS/FAIL: %d/%d.\n", tpx2_valid_iter, num_of_test-tpx2_valid_iter);
#endif
	return ret;
}


void tpx2_basic_configuration( TIMEPIX_ID tpxId )
{
	uint8_t spi_txbuf[TPX2_PKT_CMD_SET_32_LENGTH]= {0};
	board_tpx2_set_chip_select( tpxId, 0 );
	board_tpx2_set_global_reset(tpxId, 0);												// GR do '1'
	Delay(10);
	board_tpx2_set_pwr_en1v2( tpxId, 1 );		// TURN ON 1V2 PWR for tpx2
	board_set_1v2_pwr_en(TPXA, 1);				// TURN ON 1V2 PWR for MachXO2 Bank ONLY ! -> Timepix 2 has own 1V2 on Chipboard PCB
	board_tpx2_set_pwr_en2v5(tpxId, 1);			// TURN ON 2V5 PWR for tpx2
	board_tpx2_set_chip_select( tpxId, 0 );
	Delay(10);
	board_tpx2_set_shutter_counter( tpxId, 1);		// SHUTTER do 1 na RESET
	Delay(1);
	board_tpx2_set_global_reset( tpxId, 1 );		// min. 487 cyklu MCLOCK must be GLOBAL_RESET in log '0'
																// ~5MHz measurement clock run
	for(int i = 0; i < 1000; i++){	//1000
		board_spi_transmit( tpxId, spi_txbuf, sizeof(spi_txbuf) );
	}
	board_tpx2_set_global_reset( tpxId, 0 );

	//
	Delay(5);
	//board_tpx2_set_chip_select( tpxId, 1);
	board_tpx2_set_global_reset( tpxId, 1 );
	for(int i = 0; i < 2048; i++){	//1000
		board_spi_transmit( tpxId, spi_txbuf, sizeof(spi_txbuf) );
	}
	board_tpx2_set_global_reset( tpxId, 0 );

	Delay(10);

}

uint8_t tpx2_set_reg_8b(TIMEPIX_ID tpxId, TPX2_SET_CMD cmd, uint8_t *data)
{
	uint8_t hal_result = transmit_receive_set( tpxId, cmd, data, 8);
    return hal_result;
}

uint8_t tpx2_set_reg_16b(TIMEPIX_ID tpxId, TPX2_SET_CMD cmd, uint16_t *data)
{
	uint8_t hal_result = transmit_receive_set( tpxId, cmd, data, 16);
    return hal_result;
}
uint8_t tpx2_set_reg_32b(TIMEPIX_ID tpxId, TPX2_SET_CMD cmd, uint32_t *data)
{
	uint8_t hal_result = transmit_receive_set( tpxId, cmd, data, 32);
    return hal_result;
}

uint8_t tpx2_set_reg_256b(TIMEPIX_ID tpxId, TPX2_SET_CMD cmd, uint8_t *data)
{
	uint8_t hal_result = transmit_receive_set( tpxId, cmd, data, 256);
    return hal_result;
}


uint8_t transmit_receive_set(TIMEPIX_ID tpxId, uint8_t cmd, uint8_t *data, int sizeof_payload )
{
	uint8_t tpx_result;	// TODO should control result of spi transaction!
	uint8_t spi_txbuf[TPX2_PKT_CMD_SET_256_LENGTH] = {0};
	uint8_t spi_rxbuf[TPX2_PKT_CMD_SET_256_LENGTH] = {0};
	uint8_t cmd_length = 0;

	board_tpx2_set_chip_select( tpxId, 0);

	spi_txbuf[TPX2_PKT_COM_OFFSET] = cmd;
	spi_txbuf[TPX2_PKT_COM_OFFSET+1] = cmd;



	if(sizeof_payload == 8){
		// 8 bits data
		//{cmd, cmd, wait, data, 0, 0, 0, 0, 0, 0, 0};
		spi_txbuf[TPX2_PKT_CMD_SET_PAYLOAD_OFFSET] = data[0];
		cmd_length = TPX2_PKT_CMD_SET_8_LENGTH;
	} else if (sizeof_payload == 16){
		// 16 bits data
		//{cmd, cmd, 0, partA, partB, 0, 0, 0, 0, 0, 0};
		spi_txbuf[TPX2_PKT_CMD_SET_PAYLOAD_OFFSET] = data[1];
		spi_txbuf[TPX2_PKT_CMD_SET_PAYLOAD_OFFSET+1] = data[0];
		cmd_length = TPX2_PKT_CMD_SET_16_LENGTH;
	} else if (sizeof_payload == 32){
		for(int i = 0; i < sizeof_payload/8; i++){
			spi_txbuf[TPX2_PKT_CMD_SET_PAYLOAD_OFFSET+i] = data[i];
		}
		cmd_length = TPX2_PKT_CMD_SET_32_LENGTH;
	} else if (sizeof_payload == 256){
		for(int i = 0; i < sizeof_payload/8; i++){
			spi_txbuf[TPX2_PKT_CMD_SET_PAYLOAD_OFFSET+i] = data[i];
		}
			cmd_length = TPX2_PKT_CMD_SET_256_LENGTH;
	} else{
		tpx_log_error("[%s]:[%d] - sizeof_payload == 0", __FILE__, __LINE__ );
	}

	board_tpx2_set_chip_select( tpxId, 1 );
#ifdef TPX2_DEBUG
	memset(spi_rxbuf, 0, sizeof(spi_rxbuf));
#endif

	tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_txbuf, sizeof(spi_txbuf) );
	// TODO read answer
	//tpx_result = board_spi_transmit_receive( tpxId, (uint8_t *)spi_txbuf_empty, (uint8_t *)spi_rxbuf, cmd_length );
	//tpx_result = board_tpx_spi_transfer( tpxId, (uint8_t *)spi_txbuf, (uint8_t *)spi_rxbuf, cmd_length );
	board_tpx2_set_chip_select( tpxId, 0 );

	//Delay(1);
	//DUMMY
	tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_dummy_txbuf, sizeof(spi_dummy_txbuf) ); 	// DUMMY

	return tpx_result;
}

uint8_t tpx2_get_reg_8b(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint32_t *reg_value)
{
	uint8_t hal_result = transmit_receive_get( tpxId, cmd, reg_value, 8);
    return hal_result;
}

uint8_t tpx2_get_reg_16b(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint32_t *reg_value)
{
    uint8_t hal_result = transmit_receive_get( tpxId, cmd, reg_value, 16);
    return hal_result;
}

uint8_t tpx2_get_reg_24b(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint32_t *reg_value)
{
	uint8_t hal_result = transmit_receive_get( tpxId, cmd, reg_value, 24);
    return hal_result;
}

uint8_t tpx2_get_reg_32b(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint32_t *reg_value)
{
	uint8_t hal_result = transmit_receive_get( tpxId, cmd, reg_value, 32);
	return hal_result;
}

uint8_t tpx2_get_reg_256b(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd)
{
    uint8_t tpx_result = TPX_OK;
#if 1
    uint16_t size_of_buf = 39;
    uint8_t rx_buffer[39] = {0};
    uint8_t spi_tx_buf[39] = {0};			// 50 is for test, need to be set TODO
#endif
    board_tpx2_set_chip_select( tpxId, 0);


    // Fill HDR
    spi_tx_buf[TPX2_PKT_COM_OFFSET] = cmd;
    spi_tx_buf[TPX2_PKT_COM_OFFSET+1] = cmd;

    // TODO
	board_tpx2_set_chip_select( tpxId, 1 );

	tpx_result = board_spi_receive( tpxId, (uint8_t *)(&rx_buffer[0]), size_of_buf);	// prepare buffer for receive data
	tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_tx_buf, size_of_buf);	// transmit empty spi_tx_buf into tpx2

	//tpx_result = board_spi_transmit_receive( tpxId, (uint8_t *)spi_txbuf_empty, (uint8_t *)spi_rxbuf, cmd_length);
	board_tpx2_set_chip_select( tpxId, 0 );
	//Delay(1);
	//DUMMY
	tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_dummy_txbuf, sizeof(spi_dummy_txbuf) );	// Dummy transfer

    board_tpx2_set_chip_select( tpxId, 0);

    // check HDR and FIN bytes:
    // HDR
#if 1
    if( (rx_buffer[TPX2_PKT_ECHO_COM_OFFSET] != cmd)||\
		(rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+1] != cmd)||\
		(rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+2] != cmd) ){
    	log_error("GET 256 HDR: 0x%x 0x%x 0x%x READ: 0x%x 0x%x 0x%x",cmd,cmd,cmd, rx_buffer[TPX2_PKT_ECHO_COM_OFFSET], rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+1], rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+2]);
		tpx_result = TPX_HDR_FAIL;
    } else {
    	// HDR is fine check FIN
		if(rx_buffer[TPX2_PKT_DTAT_OFFSET+TPX2_PKT_PAYLOAD256_LENGTH] != cmd){
			log_error("GET 256 TAIL: 0x%x READ: 0x%x",cmd, rx_buffer[TPX2_PKT_DTAT_OFFSET+TPX2_PKT_PAYLOAD256_LENGTH]);
			tpx_result = TPX_FIN_FAIL;
		} else {
			// Everythings works fine
			//log_debug("CMD: 0x%x HDR: 0x%x 0x%x 0x%x",cmd, rx_buffer[TPX2_PKT_ECHO_COM_OFFSET], rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+1], rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+2]);
			tpx_result = TPX_OK;

		}

    }
#endif


    return tpx_result;
}

const char* strGetCmdList_ZCS[4] = { "ZCS_A", "ZCS_B", "ZCS_C", "ZCS_D" };
uint8_t tpx2_get_reg_256b_ZCS(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint8_t *buffer, uint8_t *act_zcs)
{
    uint8_t tpx_result = TPX_OK;
    bool header_valid = true;
#if 0
    uint16_t size_of_buf = 100;
    uint8_t rx_buffer[5200] = {0};
    uint8_t spi_tx_buf[5200] = {0};			// 50 is for test, need to be set TODO
#endif
#if 0
    uint16_t size_of_buf = 50;
    uint8_t rx_buffer[50] = {0};
    uint8_t spi_tx_buf[50] = {0};			// 50 is for test, need to be set TODO
#endif
    board_tpx2_set_chip_select( tpxId, 0);


    // Fill HDR
    uint32_t size_of_header = 39;
    uint8_t spi_rx_column[39] = {0};
    uint8_t spi_tx_column[39] = {0};
    spi_tx_column[TPX2_PKT_COM_OFFSET] = cmd;
    spi_tx_column[TPX2_PKT_COM_OFFSET+1] = cmd;

	board_tpx2_set_chip_select( tpxId, 1 );

	tpx_result = board_spi_receive( tpxId, (uint8_t *)spi_rx_column, size_of_header);	// prepare spi_rx_header for receive data
	tpx_result = board_spi_transmit(tpxId, (uint8_t *)spi_tx_column, size_of_header);	// transmit  into tpx2, load data into spi_rx_header

	if(spi_rx_column[3] != cmd || spi_rx_column[4] != cmd || spi_rx_column[5] != cmd ){
		log_error("%s FAIL", strGetCmdList_ZCS[zcs_number(cmd)]);
		log_error("GET HDR: 0x%x READ: 0x%x 0x%x 0x%x 0x%x",cmd, spi_rx_column[3], spi_rx_column[4], spi_rx_column[5], spi_rx_column[6]);
		header_valid = false;
		//goto finish;
	} else {
		log_debug("%s OK", strGetCmdList_ZCS[zcs_number(cmd)]);
	}
	// COUNT ACT column
	uint16_t act_column_A = 0;
	count_active_column(&act_column_A, spi_rx_column, act_zcs); // -> from this will be calculate lenght of payload
	log_debug("COL: %d",act_column_A);

	uint8_t spi_tx_buf[320] = {0};					// COUNTER_A/B - 320 & COUNTER_C/D - 128, can be 320 -> after size limit set by size of transmit buffer
	uint16_t num_of_row = act_column_A+1;;

#if 0
	if(header_valid){
		num_of_row = act_column_A+1;			// +1 bcs of future for loop range
	} else {
		num_of_row = act_column_A-1;
	}
#endif

	uint16_t size_of_buf = 0;
	size_of_buf = (get_size_of_buf(cmd));				// number of bytes need for read out one row: A,B (10bits) : 320B (320*8 = 2560b). C,D (4bits) : 128B (128*8 = 1024b)

	for( uint16_t row = 0; row < num_of_row; row++ )
	{
		if( buffer == NULL ){
			tpx_result = board_spi_receive( tpxId, (uint8_t *)spi_dummy_rxbuf, size_of_buf);
		} else {
			// TODO : test if cannot by transformation done here
			tpx_result = board_spi_receive( tpxId, (uint8_t *)(&buffer[row * size_of_buf]), size_of_buf);	// prepare buffer for receive data
			tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_tx_buf, size_of_buf);					// transmit empty spi_tx_buf into tpx2, load data into buffer
		}
	}
	log_debug("FIN: 0x%x 0x%x 0x%x", buffer[(act_column_A*size_of_buf)-1], buffer[act_column_A*size_of_buf], buffer[(act_column_A*size_of_buf)+1]);

#if 0 // test
	for(int i = 0; i < 52; i++){
		tpx_result = board_spi_receive( tpxId, (uint8_t *)(&rx_buffer[i*100]), size_of_buf);	// prepare buffer for receive data
		tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_tx_buf, size_of_buf);	// transmit empty spi_tx_buf into tpx2
	}
#endif

	//Delay(1);
	//DUMMY
    board_tpx2_set_chip_select( tpxId, 0);
    tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_dummy_txbuf, sizeof(spi_dummy_txbuf) );	// Dummy transfer
    if(!header_valid){
    	return TPX_FAILED;
    } else {
    	return TPX_OK;
    }

    // check HDR and FIN bytes:
    // HDR
#if 0
    if( (rx_buffer[TPX2_PKT_ECHO_COM_OFFSET] != cmd)||\
		(rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+1] != cmd)||\
		(rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+2] != cmd) ){
    	log_error("GET 256 HDR: 0x%x 0x%x 0x%x READ: 0x%x 0x%x 0x%x",cmd,cmd,cmd, rx_buffer[TPX2_PKT_ECHO_COM_OFFSET], rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+1], rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+2]);
		tpx_result = TPX_HDR_FAIL;
    } else {
    	// HDR is fine check FIN
		if(rx_buffer[TPX2_PKT_DTAT_OFFSET+TPX2_PKT_PAYLOAD256_LENGTH] != cmd){
			log_error("GET 256 TAIL: 0x%x READ: 0x%x",cmd, rx_buffer[TPX2_PKT_DTAT_OFFSET+TPX2_PKT_PAYLOAD256_LENGTH]);
			tpx_result = TPX_FIN_FAIL;
		} else {
			// Everythings works fine
			//log_debug("CMD: 0x%x HDR: 0x%x 0x%x 0x%x",cmd, rx_buffer[TPX2_PKT_ECHO_COM_OFFSET], rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+1], rx_buffer[TPX2_PKT_ECHO_COM_OFFSET+2]);
			tpx_result = TPX_OK;

		}

    }
#endif
    // take only data from the packet:
#if 0
    for(uint8_t i = 0; i < TPX2_PKT_PAYLOAD256_LENGTH; i++){
    	uint8_t tmp = 0;
    	tmp = rx_buffer[i+TPX2_PKT_DTAT_OFFSET];		// offset is set bcs of header lenght and zeros data at start
    	buffer[i] = tmp;				// copy data into output buffer
    }
#endif
    return tpx_result;
}

// act_zcs -> info about activation of column
// buffer -> input, data from zcs mode which is necessary to transform
// buffer_ZCS -> output data
uint8_t ZCS_transform(TPX2_GET_CMD cmd, uint8_t *buffer, uint8_t *act_zcs, uint8_t *buffer_ZCS){
	uint8_t col = 0;						// how many active column is in the act_zcs
	uint16_t index_act_col[256] = {0x0};	//
	switch (cmd) {

		case GET_COUNTER_A_ZCS:
		case GET_COUNTER_B_ZCS:
			// testing purpouse
			// find index of ones in act_zcs

#if 1		// find on which position in act_zcs is 1 and save the index
			col = 0;
			for(uint8_t i = 0; i < TPX2_PKT_PAYLOAD256_LENGTH; i++){
				for(uint8_t bit = 0; bit < 8; bit++){
					uint8_t var = ((act_zcs[i] >> bit) & 0x1);
					if(var){
						index_act_col[col] = (i*8 + bit)-1;	// -1 bcs of meaning in index word
						col++;		// also counting active_col numer
					}
				}
			}

			uint8_t px = 9;
			uint8_t radek = 0;
			uint32_t zcs_bits = col*10*256;		// Total number of bits need to read out. Counters A, B are 10bits
			uint8_t tmp = 0;
			for(uint32_t bits = 0; bits < zcs_bits; bits++){
				//uint8_t tmp = buffer[i+(3-byteCounter)];	// Reverse order bytes


				// TODO naskladani bitu do vystupniho pole
				if(bits == col*10){	// n-th bit read out from buffer
					px--;
					if(px == -1){	// all bits from radek was read out
						radek++;
						px = 9;		// in new radek strart shifting from start
					}
				}
			}

#endif
#if 0		// TEST pattern
			uint8_t pattter = 0xaa;
			for(uint16_t row = 0; row < 256; row++){
				if(row%2 == 0){
					pattter = 0xaa;
				} else{
					pattter = 0x55;
				}
				for(uint32_t col = 0; col < 320; col++){	// jeden radek
					buffer[row*320 + col] = pattter;
				}
			}
			for(uint16_t i = 0; i < 256; i++){
				if(i%2 == 0){
					act_zcs[i] = 0xaa;
				} else {
					act_zcs[i] = 0x55;
				}

			}
#endif


#if 0


			for(uint16_t j = 0; j < 256; j++){
				for(uint8_t px = 0; px < 10; px++){
					for(uint16_t i = 0; i < col; i++){	// for 1 bit in pixel
						buffer_ZCS[index_act_col[i] + j*256 + px*256] = buffer[i + px*col + j*col*10];		// bleju !!!!
					}
				}

			}
#endif

			break;
		case GET_COUNTER_C_ZCS:
		case GET_COUNTER_D_ZCS:
			// testing purpouse
			// find index of ones in act_zcs
			col = 0;
			for(uint8_t i = 0; i < TPX2_PKT_PAYLOAD256_LENGTH; i++){
				for(uint8_t bit = 0; bit < 8; bit++){
					uint8_t var = ((act_zcs[i] >> bit) & 0x1);
					if(var){
						index_act_col[col] = (i*8 + bit)-1;	// -1 bcs of meaning in index word
						col++;		// also counting active_col numer
					}
				}
			}

			for(uint16_t j = 0; j < 256; j++){
				for(uint8_t px = 0; px < 4; px++){
					for(uint16_t i = 0; i < col; i++){	// for 1 bit in pixel
						buffer_ZCS[index_act_col[i] + j*256 + px*256] = buffer[i + px*col + j*col*10];		// bleju !!!!
					}
				}

			}


			break;
		default:
			break;
	}
}

// act_zcs -> store info about which column is actve
uint8_t count_active_column(uint16_t *active_column, uint8_t *zcs, uint8_t *act_zcs){
	uint8_t result = BOARD_OK;
	uint16_t temp = 0;
	uint8_t hdr_offset = 6;
	for(uint8_t i = 0+hdr_offset; i < TPX2_PKT_PAYLOAD256_LENGTH+hdr_offset; i++){
		act_zcs[i-hdr_offset] = zcs[i];
		for(uint8_t bit = 0; bit < 8; bit++){
			uint8_t var = ((zcs[i] >> bit) & 0x1);
			if(var){
				temp++;
			}

		}
	}
	*active_column = temp;
	return result;
}

uint8_t transmit_receive_get( TIMEPIX_ID tpxId, uint8_t cmd, uint32_t *reg_value, int sizeof_payload)
{
	uint8_t tpx_result;	// TODO should control result of spi transaction!
	uint8_t spi_rxbuf[TPX2_PKT_CMD_GET_32_LENGTH] = {0};
	uint8_t spi_txbuf_get[TPX2_PKT_CMD_GET_32_LENGTH] = {0};
	spi_txbuf_get[TPX2_PKT_COM_OFFSET] = cmd;
	spi_txbuf_get[TPX2_PKT_COM_OFFSET+1] = cmd;
	uint8_t cmd_length = 0;

	board_tpx2_set_chip_select( tpxId, 0);

	if(sizeof_payload == 8){
		cmd_length = TPX2_PKT_CMD_GET_8_LENGTH;
	} else if(sizeof_payload == 16){
		cmd_length = TPX2_PKT_CMD_GET_16_LENGTH;
	} else if(sizeof_payload == 24){
		cmd_length = TPX2_PKT_CMD_GET_24_LENGTH;
	} else if(sizeof_payload == 32){
		cmd_length = TPX2_PKT_CMD_GET_32_LENGTH;
	} else if(sizeof_payload == 256){
			cmd_length = TPX2_PKT_CMD_GET_256_LENGTH;
	} else{
		tpx_log_error("[%s]:[%d] - invalid sizeof_payload == %d", __FILE__, __LINE__, sizeof_payload );
	}

	//Delay(5);
	board_tpx2_set_chip_select( tpxId, 1 );
#ifdef TPX2_DEBUG
	memset(spi_rxbuf, 0, sizeof(spi_rxbuf));
#endif
	tpx_result = board_spi_receive( tpxId, (uint8_t *)spi_rxbuf, cmd_length);
	tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_txbuf_get, sizeof(spi_txbuf_get) );	// SEND 2 CMD
	//tpx_result = board_spi_transmit_receive( tpxId, (uint8_t *)spi_txbuf_empty, (uint8_t *)spi_rxbuf, cmd_length);
	board_tpx2_set_chip_select( tpxId, 0 );
	//Delay(1);
	//DUMMY
	tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_dummy_txbuf, sizeof(spi_dummy_txbuf) );	// Dummy transfer
	// extract data from spi_rxbuf
	*reg_value = tpx2_extract_value_from_get_cmd( &spi_rxbuf[6], sizeof_payload );

	return tpx_result;
}

uint8_t tpx2_setcounter( TIMEPIX_ID tpxId, uint8_t cmd, uint8_t *buffer, uint32_t length)
{
	bool header_valid = true;
	bool header_recieve = false;
	bool firstColumn = true;
	uint8_t tpx_result;				// TODO should control result of spi transaction!
	board_tpx2_set_chip_select( tpxId, 0);
	uint8_t spi_rx_header[3] = {0};
	uint8_t spi_tx_header[3] = {0};					// spi_tx_header = [COM COM WAIT]
	spi_tx_header[TPX2_PKT_COM_OFFSET]   = cmd;
	spi_tx_header[TPX2_PKT_COM_OFFSET+1] = cmd;

	board_tpx2_set_chip_select( tpxId, 1 );
	// Send SET HEADER
	tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_tx_header, sizeof(spi_tx_header));

	uint16_t num_of_row = 256;
	uint16_t size_of_buf = 0;
	size_of_buf = get_size_of_buf(cmd);
	for( uint16_t row = 0; row < num_of_row ; row++ )
	{
		if(firstColumn){
			tpx_result = board_spi_receive( tpxId, (uint8_t *)spi_rx_header, sizeof(spi_rx_header));
			firstColumn = false;
		}
		if(header_recieve){
			if( (spi_rx_header[0] != cmd)||\
				(spi_rx_header[1] != cmd)||\
				(spi_rx_header[2] != cmd) )
			{
				header_valid = false;
				//goto finish;
			}
		}
		tpx_result = board_spi_transmit( tpxId, (uint8_t *)(&buffer[row * size_of_buf]), size_of_buf);
		header_recieve = true;
	}
	// Read FIN
	uint8_t spi_rx_footer[4] = {0};
	uint8_t spi_tx_footer[4] = {0};
	tpx_result = board_spi_receive(  tpxId, (uint8_t *)spi_rx_footer, sizeof(spi_rx_footer) );
	tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_tx_footer, sizeof(spi_tx_footer) );
finish:
	board_tpx2_set_chip_select( tpxId, 0 );
	// Dummy transfer		?NECCESSARY? -> YES!!
	//tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_dummy_txbuf, sizeof(spi_dummy_txbuf) );	// Dummy transfer
	for( uint16_t col = 0; col < 1 ; col++ )
	{
		tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_dummy_txbuf, sizeof(spi_dummy_txbuf) );	// Dummy transfer
	}

	if( (spi_rx_footer[3] == cmd) && header_valid )		// control if everything was fine
		return TPX_OK;

	return TPX_FAILED;
}


uint32_t tpx2_extract_value_from_get_cmd( uint8_t *data, uint8_t sizeof_payload)
{
	uint32_t tmp = 0;
	uint8_t sizeof_payload_in_bytes = sizeof_payload / 8;
	for( uint8_t i = 0; i < sizeof_payload_in_bytes; i++ )
	{
		uint8_t shift = ((sizeof_payload_in_bytes-1) - i);	// Reverse order
		tmp |= (data[i] << (shift*8));
	}
	return tmp;
}


uint8_t tpx2_getcounter( TIMEPIX_ID tpxId, uint8_t cmd, uint8_t *buffer, uint16_t column)
{
	bool header_valid = true;
	uint8_t tpx_result;				// TODO should control result of spi transaction!
	board_tpx2_set_chip_select( tpxId, 0);
	uint8_t spi_tx_header[6] = {0};
	uint8_t spi_rx_header[6] = {0};
	spi_tx_header[TPX2_PKT_COM_OFFSET]   = cmd;
	spi_tx_header[TPX2_PKT_COM_OFFSET+1] = cmd;		// spi_tx_header = [COM COM WAIT 0 0 0]

	board_tpx2_set_chip_select( tpxId, 1 );
	// Send Cmd-Header
	tpx_result = board_spi_receive( tpxId, (uint8_t *)spi_rx_header, sizeof(spi_rx_header));	// prepare spi_rx_header for receive data
	tpx_result = board_spi_transmit(tpxId, (uint8_t *)spi_tx_header, sizeof(spi_tx_header));	// transmit  into tpx2, load data into spi_rx_header

	if( (spi_rx_header[3] != cmd)||\
		(spi_rx_header[4] != cmd)||\
		(spi_rx_header[5] != cmd) )
	{
		header_valid = false;
		log_error("GET COUNTER HDR: 0x%x 0x%x 0x%x READ:0x%x 0x%x 0x%x", cmd, cmd, cmd, spi_rx_header[3], spi_rx_header[4], spi_rx_header[5]);
		goto finish;
	}
	//uint8_t spi_tx_buf[320] = {0};

	uint8_t spi_tx_buf[320] = {0};					// COUNTER_A/B - 320 & COUNTER_C/D - 128, can be 320 -> after size limit set by size of transmit buffer
	uint16_t num_of_row = column;
	uint16_t size_of_buf = 0;
	size_of_buf = (get_size_of_buf(cmd));				// number of bytes need for read out one row: A,B (10bits) : 320B (320*8 = 2560b). C,D (4bits) : 128B (128*8 = 1024b)
	for( uint16_t row = 0; row < num_of_row; row++ )
	{
		if( buffer == NULL ){
			tpx_result = board_spi_receive( tpxId, (uint8_t *)spi_dummy_rxbuf, size_of_buf);
		} else {
			// TODO : test if cannot by transformation done here
			tpx_result = board_spi_receive( tpxId, (uint8_t *)(&buffer[row * size_of_buf]), size_of_buf);	// prepare buffer for receive data
			tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_tx_buf, size_of_buf);					// transmit empty spi_tx_buf into tpx2, load data into buffer
		}
	}
	// Read Fin
	uint8_t spi_rx_footer[1] = {0};
	uint8_t spi_tx_footer[1] = {0};
	tpx_result = board_spi_receive( tpxId, (uint8_t *)spi_rx_footer, sizeof(spi_rx_footer));
	tpx_result = board_spi_transmit(tpxId, (uint8_t *)spi_tx_footer, sizeof(spi_tx_footer));
finish:
	board_tpx2_set_chip_select( tpxId, 0 );
	for( uint16_t col = 0; col < 1 ; col++ )
	{
		tpx_result = board_spi_transmit( tpxId, (uint8_t *)spi_dummy_txbuf, sizeof(spi_dummy_txbuf) );	// Dummy transfer
	}

	if(header_valid)// && (spi_rx_footer[0] == cmd))
		return TPX_OK;

	if((spi_rx_footer[0] != cmd)){
		log_error("GET COUNTER TAIL: 0x%x READ: 0x%x", cmd, spi_rx_footer[0]);
	}
	return TPX_FAILED;
}


uint32_t GetCounterBitSize( uint8_t cmd )
{
	switch( cmd ){
		case GET_COUNTER_A :
		case SET_COUNTER_A :
		case GET_COUNTER_B :
		case SET_COUNTER_B :
		case SET_CONF	   :
			return 655360;
		case GET_COUNTER_C :
		case SET_COUNTER_C :
		case GET_COUNTER_D :
		case SET_COUNTER_D :
		case GET_CONF	   :
		case SET_TRIM	   :
		case GET_TRIM      :
			return 262144;
		case GET_MASKZCS :
		case SET_MASKZCS :
			return 256;

		default :
			tpx_log_error("[%s]:[%d] - invalid CMD id: = 0x%x", __FILE__, __LINE__, cmd );
			return 0;
	}

}

uint16_t get_size_of_buf(uint8_t cmd){
	switch(cmd){
		case GET_COUNTER_A :
		case GET_COUNTER_B :
		case SET_COUNTER_A :
		case SET_COUNTER_B :
		case GET_COUNTER_A_ZCS :
		case GET_COUNTER_B_ZCS :
		case SET_TRIM:
			return 320;
		case GET_COUNTER_C :
		case GET_COUNTER_D :
		case SET_COUNTER_C :
		case SET_COUNTER_D :
		case GET_COUNTER_C_ZCS :
		case GET_COUNTER_D_ZCS :
		case SET_CONF:
		case GET_CONF:
		case GET_TRIM:
		//case SET_TRIM:
			return 128;
		default :
			tpx_log_error("Cannot set size of BUF");
			return 0;
	}
}

uint8_t pattern_inc( uint32_t index )
{
	//return (uint8_t)(~index & 0xFF);
	return (uint8_t)(~index);
}



uint8_t pattern_gen( TEST_PATTERN_ID testId, uint32_t index )
{
	uint8_t pattern = 0;
	switch( testId ){
		case PATTERN_00 :
			pattern = 0x00;
			break;
		case PATTERN_FF :
			pattern = 0xFF;
			break;
		case PATTERN_8 :
			pattern = 0x08;
			break;
		case PATTERN_DEC :
			pattern = (uint8_t)(~index & 0xFF);
			break;
		case PATTERN_INC :
			pattern = (uint8_t)(index & 0xFF);
			break;
		case PATTERN_AA:
			pattern = 0xAA;
			break;
		default:
			while(1);
	}
	return pattern;
}

uint8_t zcs_number( TPX2_GET_CMD get_cmd)
{
	uint8_t number = 0;
	switch( get_cmd ){
		case GET_COUNTER_A_ZCS :
			number = 0;
			break;
		case GET_COUNTER_B_ZCS:
			number = 1;
			break;
		case GET_COUNTER_C_ZCS:
			number = 2;
			break;
		case GET_COUNTER_D_ZCS:
			number = 3;
			break;
		default:
			while(1);
	}
	return number;
}

const char* strGetCmdList[4] = { "GET_COUNTER_A", "GET_COUNTER_B", "GET_COUNTER_C", "GET_COUNTER_D" };
uint8_t tpx2_digital_test( TIMEPIX_ID tpxId )
{
	bool set_done = false;
	bool get_done = true;
	bool err = false;
	uint8_t reset;
	uint8_t num_tests = 2;
	uint8_t num_of_test_counter = 4;
	const TPX2_SET_CMD setCmdList[4] = { SET_COUNTER_A, SET_COUNTER_B, SET_COUNTER_C, SET_COUNTER_D};
	const TPX2_GET_CMD getCmdList[4] = { GET_COUNTER_A, GET_COUNTER_B, GET_COUNTER_C, GET_COUNTER_D};

	for( uint8_t index_of_test = 0; index_of_test < num_tests ; index_of_test++ )
	{
		for( uint8_t counter_index = 0; counter_index < num_of_test_counter ; counter_index++ )
		{
			//tpx2_get_reg_8b(TPXA, RESET_COUNTER_A, &reset);
			// SET
			TPX2_SET_CMD cmdSet = setCmdList[counter_index];
			const uint32_t counter_tx_pattern_length = GetCounterBitSize( cmdSet ) / 8;
			uint8_t *counter_tx_pattern = (uint8_t *)malloc( counter_tx_pattern_length );
			if( counter_tx_pattern == NULL ){
				log_error("counter_tx_pattern == NULL");
				return TPX_FAILED;
			}
			// GET
			TPX2_GET_CMD cmdGet = getCmdList[counter_index];
			const uint32_t counter_rx_pattern_length = GetCounterBitSize( cmdGet ) / 8;
			uint8_t *counter_rx_pattern = (uint8_t *)malloc( counter_rx_pattern_length );
			if( counter_rx_pattern == NULL ){
				return TPX_FAILED;
			}

			// Generating SET pattern -> make function
			for( int i = 0; i < counter_tx_pattern_length ; i++ )
			{
				counter_tx_pattern[i] = pattern_gen(index_of_test, i);
			}


			if( tpx2_setcounter( TPXA, cmdSet, counter_tx_pattern, counter_tx_pattern_length ) != TPX_OK ){
				log_error("Digital test: SET_COUNTER");
				while(1);
				set_done = false;
			} else {
				set_done = true;
			}


			SAFE_FREE(counter_tx_pattern);

#if 0
			if(set_done == false){
				SAFE_FREE(counter_rx_pattern);
				return TPX_FAILED;
			}
#endif
			//
			Delay( 5 );


			memset( counter_rx_pattern, 0xA5, counter_rx_pattern_length );
			if( tpx2_getcounter( TPXA, cmdGet, counter_rx_pattern, 256 ) != TPX_OK ){
				log_error("Digital test: GET_COUNTER");
				while(1);						// TODO, while(1) not ideal
				get_done = false;
			} else {
				get_done = true;
			}
#if 0
			if(get_done == false){
				SAFE_FREE(counter_rx_pattern);
				return TPX_FAILED;
			}

#endif
			// Control if SET COUNTER = GET COUNTER
			bool pattern_err = false;
			for( int i = 0; i < counter_rx_pattern_length; i++ )
			{
				if( counter_rx_pattern[i] != pattern_gen( index_of_test, i) ){
					log_error("Failed counter: %s, test ID: %d, index: %d, write: 0x%x, read: 0x%x.", strGetCmdList[counter_index], index_of_test, i, pattern_gen( index_of_test, i ), counter_rx_pattern[i] );
					err = true;
					pattern_err = true;
					SAFE_FREE( counter_rx_pattern );
					return TPX_FAILED;
				}
			}
			SAFE_FREE(counter_rx_pattern);
			//return TPX_OK;
		}
	}
	return TPX_OK;
}

uint8_t transform_set_counterAB_new(const uint8_t *bmc, uint8_t *trimStream)
{
	uint32_t i,x,y,xx = 0;

	for (i = 0; i < 81920; i++)
		trimStream[i] = 0;

	for (y = 0; y < 256; y++)
	{
		for (x = 0; x < 256; x++)
		{
			for (i = 5; i < 10; i++)
			{
				xx = 255 - x;

				unsigned int pos = (y * 10 * 256) + (i * 256) + x;
				unsigned int pos_byte = (pos / 8);
				unsigned int pos_bit = 7-(pos % 8);

				trimStream[pos_byte] |= ((bmc[xx+256*y] >> (9-i))&0x1)<<pos_bit;
			}
		}
	}
}

uint8_t transform_set_counterCD_new(const uint8_t *bmc, uint8_t *confStream )
{
	uint32_t i,x,y,xx = 0;

	for (i = 0; i < 32768; i++)
		confStream[i] = 0;

	for (y = 0; y < 256; y++)
	{
		for (x = 0; x < 256; x++)
		{
			for (i = 2; i < 4; i++)
			{
				xx = 255 - x;
				unsigned int pos = (y * 4 * 256) + (i * 256) + x;
				unsigned int pos_byte = (pos / 8);
				unsigned int pos_bit = 7 - (pos % 8);

				confStream[pos_byte] |= ((bmc[xx + 256 * y] >> (9-i)) & 0x1) << pos_bit;
			}
		}
	}
}

uint8_t transform_set_counterAB(uint8_t *buffer, uint8_t *set_buf){

	// FILL data into buffer
#if 0
	for(int i = 0; i < SIZE_OF_MATRIX; i++){

		buffer[i] = 0x1F;		// TRIM[4:0] -> 5'11111
	}
#endif

#if 1
	int j = 0;
	int count = 0;
	for(int px = 0; px < 10; px++){
		uint64_t tmp = 0;
		int idx = 0;
		for(int i = 0; i < SIZE_OF_MATRIX; i++){
			if(px > 4){ // 5-bits padded with 0's	-> using for set TRIM bits
				tmp = 0;
				count++;
				if(count == 32){
					set_buf[j] =   (tmp>>0)  & 0xFF;
					set_buf[j+1] = (tmp>>8)  & 0xFF;
					set_buf[j+2] = (tmp>>16) & 0xFF;
					set_buf[j+3] = (tmp>>24) & 0xFF;
					j+=4;
					count = 0;
				}

			} else {
				tmp |= ((buffer[i]>>px) & (0x01)) << idx;
				idx++;
				count++;
				if(count == 32){
					set_buf[j] =   (tmp>>0)  & 0xFF;
					set_buf[j+1] = (tmp>>8)  & 0xFF;
					set_buf[j+2] = (tmp>>16) & 0xFF;
					set_buf[j+3] = (tmp>>24) & 0xFF;
					j+=4;
					tmp = 0;
					idx = 0;
					count = 0;
				}
			}
		}
	}
#endif
	return TPX_OK;
}

uint8_t transform_set_counterCD(uint8_t *buffer, uint8_t *set_buf){
// need to set up for SET SONF matrix
	/* NOTE: buffer:	// PM TrackLab
		  bits 0-4 .. adjustment (5b)			TRIM[4:0]
		  bit 5    .. unknown / unused (1b)		CONF[3:2]
		  bit 6    .. test (1b)					CONF[1]
		  bit 7    .. mask (1b)					CONF[0]
	 */

	/*
	 NOTE: CONF:	// tpx2 manual pg. 18
	 	  bit 0	 	.. test (1b)	CONF[0]
		  bit 1  	.. mask (1b)	CONF[1]
		  bit 2-3	.. unused		CONF[2:3]
	 */
	for(int i =0; i < SIZE_OF_COUNTER_CD_BYTES/2; i++){
		set_buf[i] = 0x0;
	}
	for(int i = SIZE_OF_COUNTER_CD_BYTES/2; i < SIZE_OF_COUNTER_CD_BYTES; i++){
		set_buf[i] = 0x00;
	}
	return TPX_OK;
}


// nerozumim co se deje pokud je vstup miniLut[px][2 nebo 3]??
const uint32_t miniLut[10][4] = {
	// 0
	{
			0x00000000,
			0x00000001,
			0x00010000,
			0x00010001,
	},
	// 1
	{
			0x00000000,
			0x00000002,
			0x00020000,
			0x00020002,
	},
	// 2
	{
			0x00000000,
			0x00000004,
			0x00040000,
			0x00040004,
	},
	// 3
	{
			0x00000000,
			0x00000008,
			0x00080000,
			0x00080008,
	},
	// 4
	{
			0x00000000,
			0x00000010,
			0x00100000,
			0x00100010,
	},
	// 5
	{
			0x00000000,
			0x00000020,
			0x00200000,
			0x00200020,
	},
	// 6
	{
			0x00000000,
			0x00000040,
			0x00400000,
			0x00400040,
	},
	// 7
	{
			0x00000000,
			0x00000080,
			0x00800000,
			0x00800080,
	},
	// 8
	{
			0x00000000,
			0x00000100,
			0x01000000,
			0x01000100,
	},
	// 9
	{
			0x00000000,
			0x00000200,
			0x02000000,
			0x02000200,
	},
};

const uint16_t miniLut4[4][4] = {
	// 0
	{
			0x0000,
			0x0001,
			0x1000,
			0x1001,
	},
	// 1
	{
			0x0000,
			0x0002,
			0x2000,
			0x2002,
	},
	// 2
	{
			0x0000,
			0x0004,
			0x4000,
			0x4004,
	},
	// 3
	{
			0x0000,
			0x0008,
			0x8000,
			0x8008,
	},
};

uint8_t transform_10bit_matrix_get_optim(uint8_t *buffer, uint16_t *out_buf){
// loop throught buffer
	int px = 9;
	int j = 7;
	int radek = 0;
	bool initLut = true;

	for(int i = 0; i < SIZE_OF_COUNTER_AB_BYTES; i+=4){
#ifdef ORIGINAL_CODE
		for(int n = 31; n > -1; n--){
			out_buf[radek*256 + j*32+n] |= (((tmp >> n) & 0x01) << px);
		}
#else
		uint8_t byteCounter = 3;
		uint8_t pxShift = px;
		uint16_t pixMask = (1<<px);
		do {
			uint8_t tmp = buffer[i+(3-byteCounter)];	// Reverse order bytes
			// Super performance boost :)

			if( !tmp )
				continue;

#if 0
			uint8_t bitCounter = 7;
			do {
				uint32_t offset = radek*256 + j*32 + byteCounter*8;
				if( (1<<bitCounter) & tmp )
					out_buf[offset+bitCounter] |= pixMask;
				//out_buf[offset+bitCounter] |= (((tmp >> bitCounter) & 0x01) << px);
			} while( bitCounter-- );

#else
#if 0
			// Unroll version
			uint32_t offset = radek*256 + j*32 + byteCounter*8;
			out_buf[offset+7] |= (((tmp >> 7) & 0x01) << pxShift);
			out_buf[offset+6] |= (((tmp >> 6) & 0x01) << pxShift);
			out_buf[offset+5] |= (((tmp >> 5) & 0x01) << pxShift);
			out_buf[offset+4] |= (((tmp >> 4) & 0x01) << pxShift);
			out_buf[offset+3] |= (((tmp >> 3) & 0x01) << pxShift);
			out_buf[offset+2] |= (((tmp >> 2) & 0x01) << pxShift);
			out_buf[offset+1] |= (((tmp >> 1) & 0x01) << pxShift);
			out_buf[offset+0] |= (((tmp >> 0) & 0x01) << pxShift);
			// Unroll version
#else
			// New Lut version
			uint32_t offset = radek*256 + j*32 + byteCounter*8;
			uint32_t *dst32 = (uint32_t)&out_buf[offset];
			uint8_t miniLutIndex;

			// Bit[1..0]
			miniLutIndex = (tmp >> 0) & 0x3;
			dst32[0] |= miniLut[px][miniLutIndex];
			// Bit[3..2]
			miniLutIndex = (tmp >> 2) & 0x3;
			dst32[1] |= miniLut[px][miniLutIndex];
			// Bit[5..4]
			miniLutIndex = (tmp >> 4) & 0x3;
			dst32[2] |= miniLut[px][miniLutIndex];
			// Bit[7..6]
			miniLutIndex = (tmp >> 6) & 0x3;
			dst32[3] |= miniLut[px][miniLutIndex];



#endif
#endif
		} while( byteCounter-- );
#endif
		j-=1;
		if(j == -1){		// was shifted out 256b -> 1 bit from one row
			j = 7;
			px-=1;			// next bit will be shifted in px position
			initLut = true;
			if(px == -1){	// all bits from radek was read out
				radek++;
				px = 9;		// in new radek strart shifting from start
			}
		}
	}
#if 0		// This conversion can be done when data are sending into TL ...
	for(uint32_t i = 0; i < SIZE_OF_MATRIX; i++){
		uint16_t var = out_buf[i];
		uint16_t real_value;
		real_value = LUT_10_bit[var];
		out_buf[i] = real_value;
	}
#endif
	return 0;
}

uint8_t transform_4bit_matrix_get_optim(uint8_t *buffer, uint8_t *out_buf){
// loop throught buffer
	int px = 3;
	int j = 7;
	int radek = 0;
	bool initLut = true;

	for(int i = 0; i < SIZE_OF_COUNTER_CD_BYTES; i+=4){
#ifdef ORIGINAL_CODE
		for(int n = 31; n > -1; n--){
			out_buf[radek*256 + j*32+n] |= (((tmp >> n) & 0x01) << px);
		}
#else
		uint8_t byteCounter = 3;
		uint8_t pxShift = px;
		uint16_t pixMask = (1<<px);
		do {
			uint8_t tmp = buffer[i+(3-byteCounter)];	// Reverse order bytes
			// Super performance boost :)

			if( !tmp )
				continue;

#if 0
			uint8_t bitCounter = 7;
			do {
				uint32_t offset = radek*256 + j*32 + byteCounter*8;
				if( (1<<bitCounter) & tmp )
					out_buf[offset+bitCounter] |= pixMask;
				//out_buf[offset+bitCounter] |= (((tmp >> bitCounter) & 0x01) << px);
			} while( bitCounter-- );

#else
#if 1
			// Unroll version
			uint32_t offset = radek*256 + j*32 + byteCounter*8;
			out_buf[offset+7] |= (((tmp >> 7) & 0x01) << pxShift);
			out_buf[offset+6] |= (((tmp >> 6) & 0x01) << pxShift);
			out_buf[offset+5] |= (((tmp >> 5) & 0x01) << pxShift);
			out_buf[offset+4] |= (((tmp >> 4) & 0x01) << pxShift);
			out_buf[offset+3] |= (((tmp >> 3) & 0x01) << pxShift);
			out_buf[offset+2] |= (((tmp >> 2) & 0x01) << pxShift);
			out_buf[offset+1] |= (((tmp >> 1) & 0x01) << pxShift);
			out_buf[offset+0] |= (((tmp >> 0) & 0x01) << pxShift);
			// Unroll version
#else
			// New Lut version
			uint32_t offset = radek*256 + j*32 + byteCounter*8;
			uint32_t *dst32 = (uint32_t)&out_buf[offset];
			uint8_t miniLutIndex;

			// Bit[1..0]
			miniLutIndex = (tmp >> 0) & 0x3;
			dst32[0] |= miniLut[px][miniLutIndex];
			// Bit[3..2]
			miniLutIndex = (tmp >> 2) & 0x3;
			dst32[1] |= miniLut[px][miniLutIndex];
			// Bit[5..4]
			miniLutIndex = (tmp >> 4) & 0x3;
			dst32[2] |= miniLut[px][miniLutIndex];
			// Bit[7..6]
			miniLutIndex = (tmp >> 6) & 0x3;
			dst32[3] |= miniLut[px][miniLutIndex];

#endif
#endif
		} while( byteCounter-- );
#endif
		j-=1;
		if(j == -1){		// was shifted out 256b -> 1 bit from one row
			j = 7;
			px-=1;			// next bit will be shifted in px position
			initLut = true;
			if(px == -1){	// all bits from radek was read out
				radek++;
				px = 3;		// in new radek strart shifting from start
			}
		}
	}
#if 0		// This conversion can be done when data are sending into TL ...
	for(uint32_t i = 0; i < SIZE_OF_MATRIX; i++){
		uint16_t var = out_buf[i];
		uint16_t real_value;
		real_value = LUT_10_bit[var];
		out_buf[i] = real_value;
	}
#endif
	return 0;
}


// Function to set the kth bit of n
uint8_t setBit(uint32_t n, uint32_t k)
{
    return (n | (1 << (k - 1)));
}


uint8_t tpx2_get_chip_id(TIMEPIX_ID tpxId, TPX2_GET_CMD cmd, uint32_t *reg_value)
{
	board_tpx2_set_burn_en( tpxId, 0 );
	Delay(10);
	board_tpx2_set_burn_en( tpxId, 1 );					// +2.5V on VDD33 pin of TPX2
	Delay(100);
	uint8_t hal_result = tpx2_get_reg_32b(tpxId, cmd, reg_value);
	board_tpx2_set_burn_en( tpxId, 0 );					// +1.2V on VDD33 pin of TPX2
	return hal_result;
}

void DecodeChipId( uint32_t eFuseValue, char *stringId )
{
    // Sample: H09-W0164
    uint8_t x = eFuseValue & 0x0F;
    stringId[0] = 'A' + (x-1);
    uint8_t y = (eFuseValue >> 4) & 0x0F;
    stringId[1] = '0' + (y / 10);
    stringId[2] = '0' + (y % 10);
    stringId[3] = '-';
    stringId[4] = 'W';
    uint32_t waferId = (eFuseValue >> 8) & 0xFFF;
    stringId[5] = '0' + waferId / 1000;
    stringId[6] = '0' + waferId / 100;
    stringId[7] = '0' + waferId / 10;
    stringId[8] = '0' + waferId % 10;
    stringId[9] = 0;
}

uint8_t Tpx2GetChipId(uint32_t *chipId)
{
	uint8_t status = TPX_OK;
#if defined(TPX_CMD)
	status = tpx2_get_chip_id(TPXA, GET_CHIPID, chipId);
#endif
#if defined(SPX_CMD)
	*chipId = 0xaaaaaaaa;
#endif

	return status;
}



uint8_t tpx2_get_temp(TIMEPIX_ID tpxId, float *data)
{
	uint8_t status = TPX_OK;
	// SET DACOUTSEL
	float vbg = 0;
	float vtemp = 0;
	float tpx2_temp;
	uint8_t set_dacoutsel[1] = {0};

	// GET DAC : VBG
	set_dacoutsel[0] = VBG;
	tpx2_set_reg_8b(TPXA, SET_DACOUTSEL, set_dacoutsel);		// set what type of DAC will be on DACOUT output
	board_tpx2_get_dacout(TPXA, &vbg);


	set_dacoutsel[0] = VBG_TEMP; 								   // GET DAC : VBG_TEMP
	if(tpx2_set_reg_8b(TPXA, SET_DACOUTSEL, set_dacoutsel) != TPX_OK) {    // set what type of DAC will be on DACOUT output
		Error_Handler();
	}
	if(board_tpx2_get_dacout(TPXA, &vtemp) != BOARD_OK) {
		Error_Handler();
	}
	tpx2_temp = 471.99*(vtemp - vbg) - 179.14;					// Sim. -> TT. // Temp. into real value in ^C.

	*data = tpx2_temp;
	return status;
}

uint8_t Tpx2ScanDacs(float *tpx2Dacs)
{
	uint8_t status = TPX_OK;
	float dac_voltage = 0;
	uint8_t set_dacoutsel[1] = {0};

	for(uint8_t i = 0; i < NUM_OF_DACS; i++){
		set_dacoutsel[0] = dacsScanList[i];
		tpx2_set_reg_8b(TPXA, SET_DACOUTSEL, set_dacoutsel);		// set what type of DAC will be on DACOUT output
		board_tpx2_get_dacout(TPXA, &dac_voltage);
		tpx2Dacs[i] = dac_voltage;
	}
	return status;
}

uint8_t Tpx2WriteDacToRegisterArray(uint16_t *dacArrayReg, uint8_t regIndex, uint16_t regData)
{
	uint8_t status = TPX_OK;
	dacArrayReg[regIndex] = regData;
	return status;
}

uint16_t Tpx2ReadDacFromRegisterArray( uint16_t *dacArrayReg, uint8_t regIndex )
{
	uint16_t regData;

	regData = dacArrayReg[regIndex];
	return regData;

}

uint8_t Tpx2SetDac( TPX2_SET_CMD dacChannel, uint16_t dacValue )
{
	uint8_t status = TPX_OK;

	if(dacChannel == SET_VTHFINE || dacChannel == SET_VTPFINE){	// 16 bit registers
		uint16_t vthcoarse_arr[1] = {dacValue};
		status = tpx2_set_reg_16b(TPXA, dacChannel, vthcoarse_arr);
	} else {
		uint8_t vthcoarse_arr[1] = {dacValue};
		status = tpx2_set_reg_8b(TPXA, dacChannel, vthcoarse_arr);
	}

	// SET DAC of TPX 2
	return status;
	// return Tpx3SendInputPeriphery( SetDAC_Code, ((dacValue & 0x1FF)<<5)|(dacChannel & 0x1F) );
}

uint8_t Tpx2GetDac( TPX2_SET_CMD dacChannel, uint8_t index)
{
	uint8_t status = TPX_OK;
	uint32_t dac_value = 0;

	if(dacChannel == GET_VTHFINE || dacChannel == GET_VTPFINE){	// 16 bit registers
		status = tpx2_get_reg_16b(TPXA, dacChannel, &dac_value);
	} else {
		status = tpx2_get_reg_8b(TPXA, dacChannel, &dac_value);
	}

	//log_debug("CMD_INTERNAL_DAC_SETTINGS - Dac: %s, value: 0x%x", DAC_NAME_STR[index], dac_value);
	// SET DAC of TPX 2
	return status;
	// return Tpx3SendInputPeriphery( SetDAC_Code, ((dacValue & 0x1FF)<<5)|(dacChannel & 0x1F) );
}



void UpdateTpx2Dacs( float *tpx2Dacs, uint8_t tpx2DacsLength)
{
	for( uint8_t i = 0; i < tpx2DacsLength; i++)
	{
		uint8_t dacIndex = dacsScanList[i];

		// SET DACOUTSEL // choose what will be on DACOUT
		Tpx2SenseDacSel(dacIndex);
		// Read value from DACOUT pin
		float tpxDac;
		board_tpx2_get_dacout(TPXA, &tpxDac);					// get of DAC value on DACOUT output
		//log_debug("CMD_GET_ALL_DACS_SCAN - Dac: %s, value: %.3f", DAC_NAME_STR[i], tpxDac);
		tpx2Dacs[i] = tpxDac;
	}
}
uint8_t Tpx2SenseDacSel(uint8_t dacIndex)
{
	uint8_t status = TPX_OK;
	uint8_t set_dacoutsel[1] = {dacIndex};
	status = tpx2_set_reg_8b(TPXA, SET_DACOUTSEL, set_dacoutsel);		// set what type of DAC will be on DACOUT output
	return status;
}

void set_data_mode(uint8_t datamode)	// TODO TEST
{
	uint8_t status = TPX_OK;
	// WRITE INTO OMR registr
	uint32_t get_omr = 0x0;
	uint16_t set_omr[1] = {0x0};

	status = tpx2_get_reg_16b(TPXA, GET_OMR, &get_omr);		// GET OMR REG value
	set_omr[0] = (get_omr & 0x0FFF) | (datamode << 12);			// SET 4 MSB bits of OMR <-> Pixel Operation Mode
	board_tpx2_set_shutter_counter(TPXA, 1);					// TPX2 manual pg. 33:  is best to set the IO pad SHUTTERn = 1 while changing the OMR
	status = tpx2_set_reg_16b(TPXA, SET_OMR, set_omr);

}
uint8_t pixel_matrix_loadA(uint32_t *pixel_matrix, uint16_t *counterA)
{
	// counterA -> 10 bits need to load into pixel_matrix
	uint8_t status = TPX_OK;
	for(int i = 0; i < SIZE_OF_MATRIX; i++){
		pixel_matrix[i] = (counterA[i] << 18);
	}
	return status;
}

uint8_t pixel_matrix_loadB(uint32_t *pixel_matrix, uint16_t *counterB)
{
	// counterB -> 10 bits need to load into pixel_matrix
	uint8_t status = TPX_OK;
	for(int i = 0; i < SIZE_OF_MATRIX; i++){
		pixel_matrix[i] = (counterB[i] << 8);
	}
	return status;
}

uint8_t pixel_matrix_load_counter10(uint32_t *pixel_matrix, uint16_t *buffer, TPX2_GET_CMD cmd)
{
	// counterB -> 10 bits need to load into pixel_matrix
	uint8_t status = TPX_OK;
	if(cmd == GET_COUNTER_A){
		for(int i = 0; i < SIZE_OF_MATRIX; i++){
			pixel_matrix[i] = (buffer[i] << 18);
		}
	}
	else if(cmd == GET_COUNTER_B){
		for(int i = 0; i < SIZE_OF_MATRIX; i++){
			pixel_matrix[i] = (buffer[i] << 8);
		}
	} else {
		log_error("pixel load not implemented");
	}

	return status;
}

uint8_t pixel_matrix_load_counter4(uint32_t *pixel_matrix, uint8_t *buffer, TPX2_GET_CMD cmd)
{
	// counterB -> 10 bits need to load into pixel_matrix
	uint8_t status = TPX_OK;
	if(cmd == GET_COUNTER_C){
		for(int i = 0; i < SIZE_OF_MATRIX; i++){
			pixel_matrix[i] = (buffer[i] << 4);
		}
	}
	else if(cmd == GET_COUNTER_D){
		for(int i = 0; i < SIZE_OF_MATRIX; i++){
			pixel_matrix[i] = (buffer[i]);
		}
	} else {
		log_error("pixel load not implemented");
	}

	return status;
}

uint8_t pixel_matrix_load_counterAB(uint32_t *pixel_matrix, uint16_t *buffer, COUNTER cmd)
{
	// counterB -> 10 bits need to load into pixel_matrix
	uint8_t status = TPX_OK;
	if(cmd == COUNTER_A){
		for(int i = 0; i < SIZE_OF_MATRIX; i++){
			pixel_matrix[i] = (buffer[i] << 18);
		}
	}
	else if(cmd == COUNTER_B){
		for(int i = 0; i < SIZE_OF_MATRIX; i++){
			pixel_matrix[i] = (buffer[i] << 8);
		}
	} else {
		log_error("pixel load not implemented");
	}

	return status;
}

uint8_t pixel_matrix_load_counterCD(uint32_t *pixel_matrix, uint8_t *buffer, COUNTER cmd)
{

	uint8_t status = TPX_OK;
	if(cmd == COUNTER_C){
		for(int i = 0; i < SIZE_OF_MATRIX; i++){
			pixel_matrix[i] = (buffer[i] << 4);
		}
	}
	else if(cmd == COUNTER_D){
		for(int i = 0; i < SIZE_OF_MATRIX; i++){
			pixel_matrix[i] = (buffer[i]);
		}
	} else {
		log_error("pixel load not implemented");
	}

	return status;
}

// RESET COUNTER help function
void tpx2_reset_counterA(uint8_t *counterA){
	if( tpx2_getcounter( TPXA, RESET_COUNTER_A, counterA, 256) != TPX_OK ){
		log_error("GET COUNTER A");
		while(1);
	}
}


// GET COUNTER help function
void tpx2_getcounterA(uint8_t *counterA){
	if( tpx2_getcounter( TPXA, GET_COUNTER_A, counterA, 256) != TPX_OK ){
		log_error("GET COUNTER A");
		while(1);
	}
}
void tpx2_getcounterB(uint8_t *counterB){
	if( tpx2_getcounter( TPXA, GET_COUNTER_B, counterB, 256) != TPX_OK ){
		log_error("GET COUNTER B");
		while(1);
	}
}
void tpx2_getcounterC(uint8_t *counterC){
	if( tpx2_getcounter( TPXA, GET_COUNTER_C, counterC, 256) != TPX_OK ){
		log_error("GET COUNTER C");
		while(1);
	}
}
void tpx2_getcounterD(uint8_t *counterD){
	if( tpx2_getcounter( TPXA, GET_COUNTER_D, counterD, 256) != TPX_OK ){
		log_error("GET COUNTER D");
		while(1);
	}
}

void tpx2_getcounterA_ZCS(uint8_t *counterA, uint16_t column){
	if( tpx2_getcounter( TPXA, GET_COUNTER_A_ZCS, counterA, column) != TPX_OK ){
		log_error("GET COUNTER A ZCS");
		//while(1);
	}
}

void tpx2_getconf(uint8_t *conf){
	if( tpx2_getcounter( TPXA, GET_CONF, conf, SIZE_OF_COUNTER_CD_BYTES) != TPX_OK ){
		log_error("GET COUNTER C");
		while(1);
	}
}
void tpx2_gettrim(uint8_t *trim){
	if( tpx2_getcounter( TPXA, GET_TRIM, trim, SIZE_OF_COUNTER_CD_BYTES) != TPX_OK ){
		log_error("GET COUNTER D");
		while(1);
	}
}


void tpx2_settrim(uint8_t *trim){	// TRIM
	if( tpx2_setcounter( TPXA, SET_TRIM, trim, SIZE_OF_COUNTER_AB_BYTES ) != TPX_OK ){
		log_error("SET TRIM");
		while(1);
	}
}

void tpx2_setconf(uint8_t *conf){	// CONF
	if( tpx2_setcounter( TPXA, SET_CONF, conf, SIZE_OF_COUNTER_CD_BYTES ) != TPX_OK ){
		log_error("SET CONF");
		while(1);
	}
}

// SET COUNTER help function, CounterX -> counter that will be write into Timepix
void tpx2_setcounterA(uint8_t *counterA){	// TRIM
	if( tpx2_setcounter( TPXA, SET_COUNTER_A, counterA, SIZE_OF_COUNTER_AB_BYTES ) != TPX_OK ){
		log_error("SET COUNTER A");
		while(1);
	}
}
void tpx2_setcounterB(uint8_t *counterB){
	if( tpx2_setcounter( TPXA, SET_COUNTER_B, counterB, SIZE_OF_COUNTER_AB_BYTES) != TPX_OK ){
		log_error("SET COUNTER B");
		while(1);
	}
}
void tpx2_setcounterC(uint8_t *counterC){
	if( tpx2_setcounter( TPXA, SET_COUNTER_C, counterC, SIZE_OF_COUNTER_CD_BYTES) != TPX_OK ){
		log_error("SET COUNTER C");
		while(1);
	}
}
void tpx2_setcounterD(uint8_t *counterD){
	if( tpx2_setcounter( TPXA, SET_COUNTER_D, counterD, SIZE_OF_COUNTER_CD_BYTES) != TPX_OK ){
		log_error("SET COUNTER D");
		while(1);
	}
}

uint8_t set_trim(uint8_t *buffer)
{
	uint8_t status = TPX_OK;

	/* NOTE: buffer:
				  bits 0-4 .. adjustment (5b)			TRIM[4:0]
				  bit 5    .. unknown / unused (1b)		CONF[3:2]
				  bit 6    .. test (1b)					CONF[1]
				  bit 7    .. mask (1b)					CONF[0]
	 */
	// mam jen matici
	uint8_t set_buf[SIZE_OF_COUNTER_AB_BYTES] = {0};
	int j = 0;
	int count = 0;
	for(int px = 0; px < 10; px++){
		uint64_t tmp = 0;
		int idx = 0;
		for(int i = 0; i < SIZE_OF_MATRIX; i++){
			if(px > 4){ // 5-bits padded with 0's
				uint64_t tmp = 0;
				count++;
				if(count == 32){
					set_buf[j] =   (tmp>>0)  & 0xFF;
					set_buf[j+1] = (tmp>>8)  & 0xFF;
					set_buf[j+2] = (tmp>>16) & 0xFF;
					set_buf[j+3] = (tmp>>24) & 0xFF;
					j+=4;
					count = 0;
				}

			} else {
				tmp |= ((buffer[i]>>px) & (0x01)) << idx;
				idx++;
				count++;
				if(count == 32){
					set_buf[j] =   (tmp>>0)  & 0xFF;
					set_buf[j+1] = (tmp>>8)  & 0xFF;
					set_buf[j+2] = (tmp>>16) & 0xFF;
					set_buf[j+3] = (tmp>>24) & 0xFF;
					j+=4;
					tmp = 0;
					idx = 0;
					count = 0;
				}
			}
		}
	}

	if(tpx2_setcounter(TPXA, SET_TRIM, set_buf, SIZE_OF_COUNTER_AB_BYTES) != TPX_OK){
		log_error("SET TRIM");
		return TPX_FAILED;
	}
	return status;
}


uint8_t tpx2_get_readready(){
	uint8_t out = 1;
	out = HAL_GPIO_ReadPin(TPX2_READREADY_port, TPX2_READREADY_pin);
	return out;
}
uint8_t tpx2_get_matrix_occ(){
	uint8_t out = 1;
	out = HAL_GPIO_ReadPin(TPX2_MATRIX_OCC_port, TPX2_MATRIX_OCC_pin);
	return out;
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi){
	tx_done = true;
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi){
	rx_done = true;
}













