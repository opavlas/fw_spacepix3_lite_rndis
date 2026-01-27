/*
 * spx3_comm.c
 *
 *  Created on: Mar 17, 2025
 *      Author: opavl
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <user.h>


#include "board.h"
#include "pinout.h"
#include "spx3_comm.h"
#include "debug.h"



//#define TEST_GPIO

SPX3_CONFIG spxReadOutConfig;



void print_uid(void) {
	#define UID_BASE   0x0BFA0700UL

	uint32_t uid0 = *(uint32_t *)(UID_BASE + 0x00);  // least significant
	uint32_t uid1 = *(uint32_t *)(UID_BASE + 0x04);
	uint32_t uid2 = *(uint32_t *)(UID_BASE + 0x08);  // most significant

	log_debug("UID: %08lX %08lX %08lX\n", uid2, uid1, uid0);
}
void print_uid_info(void)
{
    uint32_t uid0 = *(uint32_t*)(UID_BASE + 0x00);  // LOT[31:0]
    uint32_t uid1 = *(uint32_t*)(UID_BASE + 0x04);  // LOT[47:32]
    uint32_t uid2 = *(uint32_t*)(UID_BASE + 0x08);  // WAFER, X, Y


    // Wafer info
    uint8_t wafer  = (uid2 >> 24) & 0xFF;
    uint8_t xcoord = (uid2 >> 16) & 0xFF;
    uint8_t ycoord = (uid2 >> 8)  & 0xFF;

    // Lot number (use only UID0 lower 32 bits)
    uint32_t lot = uid0;  // 32-bit lot

    log_debug("STM32U5 Unique Device ID (96-bit)\n");
    log_debug("RAW UID: %08lX %08lX %08lX\n", uid2, uid1, uid0);
    log_debug("-------------------------------------\n");
    log_debug("Wafer Number : %u\n", wafer);
    log_debug("Wafer X Coord: %u\n", xcoord);
    log_debug("Wafer Y Coord: %u\n", ycoord);
    log_debug("Lot Number   : 0x%08lX\n", lot);
}

uint8_t spx3_init_dig(void)
{
	uint8_t ret = SPX_OK;
	bool init_pass = false;
	board_spx3_set_pwr_en1v8(SPXA, 0);
	log_debug("SpacePix3 1V8 PWR OFF");
	Delay(1);

	board_spx3_set_en_hv(SPXA, 0);
	board_stop_mclock();
	log_debug("HV OFF");
	Delay(10);
	board_spx3_set_pwr_en1v8(SPXA, 1);
	log_debug("SpacePix3 1V8 PWR ON");
	Delay(100);

	// HV START OF TESTING HV
#if 1
	board_spx3_set_en_hv(SPXA, 1);
	board_start_mclock(Frequency_HV_150V);
	Delay(10);
	//board_spx3_set_ncs_hv(SPXA, 1);
	//write_ltc2635(DACB, 0x0);
	//board_scan_pwm();
#endif
#if 0
	// STM32U5 device ID
	uint32_t dev_id = DBGMCU->IDCODE & 0xFFF;
	uint32_t rev_id = (DBGMCU->IDCODE >> 16) & 0xFFFF;
	uint32_t uid0 = *(uint32_t*)0x0BFA0000;
	uint32_t uid1 = *(uint32_t*)0x0BFA0004;
	uint32_t uid2 = *(uint32_t*)0x0BFA0008;
	log_debug("Device ID: 0x%03lX\n", dev_id);
	log_debug("Revision ID: 0x%04lX\n", rev_id);
	log_debug("UID: %08lX %08lX %08lX\n", uid2, uid1, uid0);
#endif


	print_uid();
	print_uid_info();
#if 1
	// TX Pixel Matrix Config - Default // 0x0008
	const uint16_t tx_pixel_matrix_config_len = SPX3_PX_MATRIX_BLOCK_SIZE*64;			// sending 1024b -> 128B per ROW
	uint8_t *tx_pixel_matrix_config = (uint8_t *)malloc( tx_pixel_matrix_config_len );
	memset( tx_pixel_matrix_config, 0x0, tx_pixel_matrix_config_len );
	if( tx_pixel_matrix_config == NULL ){
		log_error("tx_pixel_matrix_config == NULL");
		return SPX_FAILED;
	}
	// RX Pixel Matrix Config
	const uint32_t rx_pixel_matrix_config_len = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE+1);
	uint8_t *rx_pixel_matrix_config = (uint8_t *)malloc( rx_pixel_matrix_config_len );
	memset( rx_pixel_matrix_config, 0x00, rx_pixel_matrix_config_len );
	if( rx_pixel_matrix_config == NULL ){
		log_error("counter_rx_pattern == NULL");
		return SPX_FAILED;
	}

	// Rx Data
	// rx_data are uint8_t but 1 px has 16bit
	const uint32_t rx_data_length = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE*2+1);
	uint8_t *rx_data = (uint8_t *)malloc( rx_data_length );

	memset( rx_data, 0x00, rx_data_length );
	if( rx_data == NULL ){
		log_error("counter_rx_pattern == NULL");
		return SPX_FAILED;
	}


	while(init_pass == false){
		/// TESTING

		// -> Set init levels od gpio and choose comm. mode
		spx3_basic_config();
		log_debug("SPX3 basic config done");

// PX MATRIX CONFIG
		spx3_set_tdac(tx_pixel_matrix_config, TDAC_DEFAULT);
		// copy default settings into spxReadOutConfig struct for future usage
		memcpy( &spxReadOutConfig.pixel_matrix_config_default[0], tx_pixel_matrix_config, SPX3_PX_MATRIX_BLOCK_SIZE*64 );
		// set default PX MATRIX CONFIG
		spx3_pixel_matrix_config(spxReadOutConfig.pixel_matrix_config_default, rx_pixel_matrix_config);
		log_debug("SPX3 pixel matrix config done");

// GLOBAL CONFIG
		memset( tx_pixel_matrix_config, 0x0, tx_pixel_matrix_config_len );
		memset( rx_pixel_matrix_config, 0x0, rx_pixel_matrix_config_len );
		spx3_default_global_config(tx_pixel_matrix_config);
		// copy default settings into spxReadOutConfig struct for future usage
		memcpy( &spxReadOutConfig.global_config_default[0], tx_pixel_matrix_config, SPX3_PX_MATRIX_BLOCK_SIZE );
		// set default GLOBAL CONFIG
		spx3_global_config(spxReadOutConfig.global_config_default, rx_pixel_matrix_config);

		// Global Config array for modification
		memcpy( &spxReadOutConfig.global_config[0], tx_pixel_matrix_config, SPX3_PX_MATRIX_BLOCK_SIZE );


// TEST ANALOG_OUT
#if 0
		for(uint8_t i = 0; i < 10; i++){
			uint16_t raw_adc = 0;
			float adc_voltage = 0;
			read_ad799x_raw(AD7991_1, CH0, &raw_adc);
			log_debug("ADC CH0: 0x%X ", raw_adc);
			convert_adc_raw(raw_adc, &adc_voltage);
			log_debug("ADC CH0: %.2f V", adc_voltage);

			read_ad799x_raw(AD7991_1, CH1, &raw_adc);
			log_debug("ADC CH1: 0x%X ", raw_adc);
			convert_adc_raw(raw_adc, &adc_voltage);
			log_debug("ADC CH1: %.2f V", adc_voltage);
		}
#endif


#if 0 // TEST DACOUT
		// TODO -> need to set SPX3 TEST DAC !!!
	uint8_t num_meas = 5;
	float data_dac[5] = {0.0};

	for(uint8_t i = 0; i < num_meas; i++){
		float tpxDac = 0;
		board_tpx2_get_dacout(TPXA, &tpxDac);					// get of DAC value on DACOUT output
		log_debug("DAC value: %.3f", tpxDac);
		data_dac[i] += tpxDac;
	}

#endif

# if 1	// SCAN ALL DACS test
		UpdateSpx3Dacs(spxReadOutConfig.feadbackSpx3Dacs, NUM_OF_SPX3_DACS);

#endif
// READOUT DATA
		//spx3_data_readout(rx_data, 100);

		// TODO chcek it


#if 0 // TEST DACOUT
	uint8_t num_meas = 5;
	float data_dac[5] = {0.0};

	for(uint8_t i = 0; i < num_meas; i++){
		for(uint16_t i = 0; i < 5; i+=1){
			uint8_t test_patern[7] = {0,64,128,192,255};
			memset( rx_pixel_matrix_config, 0x0, rx_pixel_matrix_config_len );
			spx3_set_dac(spxReadOutConfig.global_config, TEST, test_patern[i]);
			spx3_global_config(spxReadOutConfig.global_config, rx_pixel_matrix_config);

			Delay(20);
			float tpxDac = 0;
			board_tpx2_get_dacout(TPXA, &tpxDac);					// get of DAC value on DACOUT output
			log_debug("DAC value: %.3f", tpxDac);
			data_dac[i] += tpxDac;
		}
	}
	// Average samples
	for(uint8_t i = 0; i < 5; i++){
		data_dac[i] = data_dac[i]/num_meas;
	}

#endif
	//spx3_global_config(spxReadOutConfig.global_config_default, rx_pixel_matrix_config);
	SAFE_FREE(tx_pixel_matrix_config);
	SAFE_FREE(rx_pixel_matrix_config);
	SAFE_FREE(rx_data);
	Delay(10);
	init_pass = true;

	}
#endif
	return ret;
}

#if 0
uint8_t Spx3SetVthr(uint16_t dac_value)
{
	uint8_t result = SPX_OK;

	// TODO read TDAC settings
	// For test purpose:
	// TX Pixel Matrix Config
	const uint16_t tx_pixel_matrix_config = SPX3_PX_MATRIX_BLOCK_SIZE;			// sending 1024b -> 128B per ROW
	uint8_t *counter_tx_pattern = (uint8_t *)malloc( tx_pixel_matrix_config );
	// CHANGE IT !!! to current TDAC settings
	///
	// TODO
	memset( counter_tx_pattern, 0x000f, tx_pixel_matrix_config );
	if( counter_tx_pattern == NULL ){
		log_error("counter_tx_pattern == NULL");
		return SPX_FAILED;
	}

	// RX Pixel Matrix Config
	const uint32_t counter_rx_pattern_length = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE+1);
	uint8_t *counter_rx_pattern = (uint8_t *)malloc( counter_rx_pattern_length );
	memset( counter_rx_pattern, 0x00, counter_rx_pattern_length );
	if( counter_rx_pattern == NULL ){
		log_error("counter_rx_pattern == NULL");
		return SPX_FAILED;
	}

// Global config setup
	const uint16_t global_config_tx_len = SPX3_PX_MATRIX_BLOCK_SIZE;			// sending 1024b
	uint8_t *global_config_tx = (uint8_t *)malloc( global_config_tx_len );
	if( global_config_tx == NULL ){
		log_error("global_config_tx == NULL");
		return SPX_FAILED;
	}
	memset( global_config_tx, 0x00, global_config_tx_len );

	const uint16_t global_config_rx_len = SPX3_PX_MATRIX_BLOCK_SIZE*2;			// sending 1024b
	uint8_t *global_config_rx = (uint8_t *)malloc( global_config_rx_len );
	if( global_config_rx == NULL ){
		log_error("global_config_tx == NULL");
		return SPX_FAILED;
	}

	uint16_t vthr = dac_value;
	spx3_default_global_config(global_config_tx)
	memcpy( &spxReadOutConfig.global_config[0], global_config_tx, SPX3_PX_MATRIX_BLOCK_SIZE );

	spx3_pixel_matrix_config(counter_tx_pattern, counter_rx_pattern);
	spx3_global_config(global_config_tx, global_config_rx);


	SAFE_FREE(counter_tx_pattern);
	SAFE_FREE(counter_rx_pattern);
	SAFE_FREE(global_config_tx);
	SAFE_FREE(global_config_rx);

	return result;
}
#endif
#if 0
// Meas !
		// RX Pixel Matrix Config
		const uint32_t rx_pattern_length = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE+1);
		uint8_t *rx_pattern = (uint8_t *)malloc( rx_pattern_length );
		memset( rx_pattern, 0x00, rx_pattern_length );
		if( rx_pattern == NULL ){
			log_error("counter_rx_pattern == NULL");
			return SPX_FAILED;
		}

		spx3_data_readout(rx_pattern);


		for(uint32_t i = 0; i < rx_pattern_length; i++){
			if(rx_pattern[i] != 0){
				//log_debug("Data!!!!!!");
			}
		}
		SAFE_FREE(rx_pattern);
#endif

uint8_t spx3_test(void)
{

	uint8_t result = SPX_OK;

	uint8_t spi_txbuf[3] = {0xAA, 0xAA, 0xAA};
	result = board_spi_transmit(SPXA, (uint8_t *)spi_txbuf, sizeof(spi_txbuf) );

	//board_start_mclock(Frequency5);

	board_spx3_set_en_hv(SPXA, 1);
	uint16_t dac_value = 0xffff;
	write_ltc2635(BIAS_ADJ, dac_value);

	return result;
}

uint8_t spx3_pixel_matrix_config(uint8_t *tx_buf, uint8_t *rx_buf)
{
	uint8_t result = SPX_OK;
#if 1
	//board_spx3_set_chip_select(SPXA, 1);
	board_spx3_set_clr_n(SPXA, 1);
	board_spx3_set_cnf_en(SPXA, 0);
	board_spx3_set_chip_select(SPXA, 0);
	Delay(1);
	board_spx3_set_clr_n(SPXA, 0);
	Delay(1);
	board_spx3_set_clr_n(SPXA, 1);
	Delay(1);


#endif

	for(uint8_t row = 0; row < SPX3_ROW_SIZE; row++){

		// TODO check row status signal
		board_spx3_set_row_shift(SPXA, 1);
		Delay(1);

		result = board_spi_receive(TPXA, (uint8_t *)&rx_buf[row*128], SPX3_PX_MATRIX_BLOCK_SIZE);
		result = board_spi_transmit(TPXA,(uint8_t *)&tx_buf[row*128], SPX3_PX_MATRIX_BLOCK_SIZE);

		Delay(1);
		board_spx3_set_row_shift(SPXA, 0);
		board_spx3_set_cnf_en(SPXA, 1);
		Delay(1);
		board_spx3_set_cnf_en(SPXA, 0);
		Delay(1);
	}


	return result;
}

uint8_t spx3_global_config(uint8_t *tx_buf, uint8_t *rx_buf)
{
	uint8_t result = SPX_OK;
	uint8_t tx_buf_zeros[128] = {0};

	board_spx3_set_row_shift(SPXA, 0);
	board_spx3_set_chip_select(SPXA, 0);
	board_spx3_set_clr_n(SPXA, 1);
	board_spx3_set_cnf_en(SPXA, 0);
	board_spx3_set_cnt_en(SPXA, 0);

#if 1 // master reset
	Delay(1);
	board_spx3_set_clr_n(SPXA, 0);
	Delay(1);
	board_spx3_set_clr_n(SPXA, 1);
	Delay(1);
#endif

	// Proccess to make global config without setting befor pixel matrix config
#if 1 // 64x ROW SHIFT
	for(uint8_t row = 0; row < 64; row++){
		board_spx3_set_row_shift(SPXA, 1);
		Delay(1);
		board_spx3_set_row_shift(SPXA, 0);
		Delay(1);
	}
#endif

	board_spx3_set_cnf_en(SPXA, 1);
	Delay(1);
	board_spx3_set_cnf_en(SPXA, 0);
	Delay(1);


	for(uint8_t row = 0; row < 2; row++){
		// TODO check row status signal
		board_spx3_set_row_shift(SPXA, 1);
		Delay(1);

		if(row == 1){ // last packet, send zeros and readout global config register // rx_buf[0:127] will be value of dac out
			result = board_spi_receive(TPXA, (uint8_t *)&rx_buf[0], SPX3_PX_MATRIX_BLOCK_SIZE);
			result = board_spi_transmit(TPXA, tx_buf_zeros, SPX3_PX_MATRIX_BLOCK_SIZE);
		} else {
			// send GLobal config from end
#if 1
			for(uint8_t i = 0; i < 128; i++){
				result = board_spi_receive(TPXA, (uint8_t *)&rx_buf[127+i], 1);
				result = board_spi_transmit(TPXA,(uint8_t *)&tx_buf[127-i], 1);
			}
#endif

#if 0
			for(uint8_t i = 0; i < 128; i++){
				result = board_spi_receive(TPXA, (uint8_t *)&rx_buf[i], 1);
				result = board_spi_transmit(TPXA,(uint8_t *)&tx_buf[i], 1);
			}
			//result = board_spi_receive(TPXA, (uint8_t *)&rx_buf[127], SPX3_PX_MATRIX_BLOCK_SIZE);
			//result = board_spi_transmit(TPXA,(uint8_t *)&tx_buf[0], SPX3_PX_MATRIX_BLOCK_SIZE);
#endif

		}

		Delay(1);
		board_spx3_set_row_shift(SPXA, 0);
		board_spx3_set_cnf_en(SPXA, 1);
		Delay(1);
		board_spx3_set_cnf_en(SPXA, 0);
		Delay(1);
	}

	return result;
}




uint8_t spx3_basic_config(void)
{
	uint8_t result = SPX_OK;

	// SET init value for GPIO signals:
	board_spx3_set_chip_select(SPXA, 1);
	// only for version 1
	//board_spx3_set_mode_select(SPXA, SPI_MODE);
	board_spx3_set_clr_n(SPXA, 1);
	board_spx3_set_cnf_en(SPXA, 0);
	board_spx3_set_cnt_en(SPXA, 0);
	board_spx3_set_row_shift(SPXA, 0);
	Delay(1);

	// Choose comm. mode -> on version V2 is default to SPI mode only
#if 0
	board_spx3_set_mode_select(SPXA, 0);
	Delay(1);
	board_spx3_set_mode_select(SPXA, SPI_MODE);
	Delay(1);
#endif


	//write_ltc2635(INJ_EN, DAC_MAX_VALUE); -> V1
	board_spx3_set_inj_en(SPXA, 1);

	write_ltc2635(ADC_IN, 0);
	Delay(1);

	return result;
}

uint8_t spx3_data_readout(uint8_t *rx_buf, uint32_t acq_time)
{
	uint8_t result = SPX_OK;

	uint8_t tx_buf_n[N] = {0x00};
	for(uint8_t i = 0; i < N; i++){
		tx_buf_n[i] = 0xff;
	}

	uint8_t tx_readout[128] = {0x0};

	// idle state definition os signal
#if 0
	board_spx3_set_row_shift(SPXA, 0);
	board_spx3_set_cnt_en(SPXA, 0);
	board_spx3_set_chip_select(SPXA, 0);
	board_spx3_set_cnf_en(SPXA, 0);
#endif

#if 0
	board_spx3_set_cnf_en(SPXA, 0);
	board_spx3_set_row_shift(SPXA, 0);

	Delay(1);
	board_spx3_set_clr_n(SPXA, 1);
#endif
	Delay(1);
	board_spx3_set_clr_n(SPXA, 0);
	Delay(1);
	board_spx3_set_clr_n(SPXA, 1);
	Delay(1);

	// Shutter - exposition
#if 0
	board_spx3_set_cnt_en(SPXA, 1);
	//Delay(1000);
	board_spx3_set_cnt_en(SPXA, 0);
#endif

	// Shutter - exposition
    process_shutter_spx3(acq_time);

	// Bus charging
	Delay(1);
	board_spx3_set_row_shift(SPXA, 1);
	Delay(1);
	board_spx3_set_row_shift(SPXA, 0);
	Delay(1);

	//Delay(20);
	// Id - Nx
	result = board_spi_transmit(TPXA, tx_buf_n, sizeof(tx_buf_n));
	Delay(1);

	for(uint8_t row = 0; row < SPX3_ROW_SIZE; row++){

		// TODO check row status signal
		board_spx3_set_row_shift(SPXA, 1);
		//Delay(1);

		result = board_spi_receive(TPXA, (uint8_t *)&rx_buf[row*128], SPX3_PX_MATRIX_BLOCK_SIZE);
		result = board_spi_transmit(TPXA, tx_readout, SPX3_PX_MATRIX_BLOCK_SIZE);

		//Delay(1);
		board_spx3_set_row_shift(SPXA, 0);

		// Id - Nx
		result = board_spi_transmit(TPXA, tx_buf_n, sizeof(tx_buf_n));
		//Delay(1);
	}

	return result;
}

uint8_t spx3_data_readout_inj(uint8_t *rx_buf, uint32_t acq_time, uint16_t inject)
{
	uint8_t result = SPX_OK;

	// EN INJ in pixels
	uint8_t a = 0x08;
	uint8_t b = 0x18;

#if 1
	uint8_t tx_buf[SPX3_PX_MATRIX_BLOCK_SIZE*64] = {0};
	for(uint32_t i = 0; i < SPX3_PX_MATRIX_BLOCK_SIZE*64; i+=2){
		// INJ . into 1st row

		if(i < SPX3_PX_MATRIX_BLOCK_SIZE*64/2){
			tx_buf[i] = a;
			tx_buf[i+1] = b;

		} else {
			tx_buf[i] = a;
			tx_buf[i+1] = a;

		}


	}
	spx3_pixel_matrix_config(tx_buf, rx_buf);

	spx3_set_dac(spxReadOutConfig.global_config, ADC_PIN_EN_DATA, 0);
	spx3_set_dac(spxReadOutConfig.global_config, TEMP_SENS_EN_DATA, 0);

	spx3_global_config(spxReadOutConfig.global_config, NULL);
#endif
	uint8_t tx_buf_n[N] = {0x00};
	for(uint8_t i = 0; i < N; i++){
		tx_buf_n[i] = 0xff;
	}

	uint8_t tx_readout[128] = {0x0};

	// idle state definition os signal
#if 0
	board_spx3_set_row_shift(SPXA, 0);
	board_spx3_set_cnt_en(SPXA, 0);
	board_spx3_set_chip_select(SPXA, 0);
	board_spx3_set_cnf_en(SPXA, 0);
#endif

#if 0
	board_spx3_set_cnf_en(SPXA, 0);
	board_spx3_set_row_shift(SPXA, 0);

	Delay(1);
	board_spx3_set_clr_n(SPXA, 1);
#endif
	Delay(1);
	board_spx3_set_clr_n(SPXA, 0);
	Delay(1);
	board_spx3_set_clr_n(SPXA, 1);
	Delay(1);

	// Shutter - exposition
#if 0
	board_spx3_set_cnt_en(SPXA, 1);
	//Delay(1000);
	board_spx3_set_cnt_en(SPXA, 0);
#endif

	// Shutter - exposition
	if(inject > DAC_MAX_VALUE){
		inject = DAC_MAX_VALUE;
	}
    process_shutter_spx3_inj(acq_time, inject);

	// Bus charging
	Delay(1);
	board_spx3_set_row_shift(SPXA, 1);
	Delay(1);
	board_spx3_set_row_shift(SPXA, 0);
	Delay(1);

	//Delay(20);
	// Id - Nx
	result = board_spi_transmit(TPXA, tx_buf_n, sizeof(tx_buf_n));
	Delay(1);

	for(uint8_t row = 0; row < SPX3_ROW_SIZE; row++){

		// TODO check row status signal
		board_spx3_set_row_shift(SPXA, 1);
		Delay(1);

		result = board_spi_receive(TPXA, (uint8_t *)&rx_buf[row*128], SPX3_PX_MATRIX_BLOCK_SIZE);
		result = board_spi_transmit(TPXA, tx_readout, SPX3_PX_MATRIX_BLOCK_SIZE);

		Delay(1);
		board_spx3_set_row_shift(SPXA, 0);

		// Id - Nx
		result = board_spi_transmit(TPXA, tx_buf_n, sizeof(tx_buf_n));
		Delay(1);
	}

	return result;
}

// data -> global_config matrix // size 1024B
void spx3_set_dac(uint8_t *data, SPX3_DACS dac, uint16_t value)
{
	uint8_t data_3_0 = 0;
	uint8_t data_7_4 = 0;
	uint8_t data_4_0 = 0;
	uint8_t data_9_5 = 0;
	// 10 bits dac
	if(dac == VTHR || dac == BAL || dac == VIN_N){
		data_4_0 = 0;
		data_9_5 = 0;
		data_4_0 = (uint8_t)((value) & (0x1F));
		data_9_5 = (uint8_t)(value >> 5);
	} else {
		data_3_0 = 0;
		data_7_4 = 0;
		data_3_0 = value & 0xF;
		data_7_4 = value >> 4;
	}

	switch (dac) {
	case VBP_CSA:
			data[0] = (data[0] & 0xF0) | (data_3_0 & 0x0F);
			data[2] = (data[2] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case VBN_CSA:
			data[4] = (data[4] & 0xF0) | (data_3_0 & 0x0F);
			data[6] = (data[6] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case VFB_CSA:
			data[8] = (data[8] & 0xF0) | (data_3_0 & 0x0F);
			data[10] = (data[10] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case VBN_PDH:
			data[12] = (data[12] & 0xF0) | (data_3_0 & 0x0F);
			data[14] = (data[14] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case VBP_HYST:
			data[16] = (data[16] & 0xF0) | (data_3_0 & 0x0F);
			data[18] = (data[18] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case VBP_COMP:
			data[20] = (data[20] & 0xF0) | (data_3_0 & 0x0F);
			data[22] = (data[22] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case VBN_TDAC:
			data[24] = (data[24] & 0xF0) | (data_3_0 & 0x0F);
			data[26] = (data[26] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case VBP_LCC:
			data[28] = (data[28] & 0xF0) | (data_3_0 & 0x0F);
			data[30] = (data[30] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case VTHR:
			data[32] = (data[32] & 0xE0) | (data_4_0 & 0x1F);
			data[34] = (data[34] & 0xE0) | (data_9_5 & 0x1F);
			break;
		case VIN_N:
			data[36] = (data[36] & 0xE0) | (data_4_0 & 0x1F);
			data[37] = (data[37] & 0xE0) | (data_9_5 & 0x1F);
			break;
		case BAL:
			data[40] = (data[40] & 0xE0) | (data_4_0 & 0x1F);
			data[42] = (data[42] & 0xE0) | (data_9_5 & 0x1F);
			break;
		case SAMPLE_LENGTH:
			data[44] = (data[44] & 0xF0) | (data_3_0 & 0x0F);
			data[46] = (data[46] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case DRIVER_BIAS:
			data[48] = (data[48] & 0xF0) | (data_3_0 & 0x0F);
			data[50] = (data[50] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case BUFFER_BIAS:
			data[52] = (data[52] & 0xF0) | (data_3_0 & 0x0F);
			data[54] = (data[54] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case LVDS_CM:
			data[56] = (data[56] & 0xF0) | (data_3_0 & 0x0F);
			data[58] = (data[58] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case LVDS_STRENGTH:
			data[60] = (data[60] & 0xF0) | (data_3_0 & 0x0F);
			data[62] = (data[62] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case VBP_AMP:
			data[64] = (data[64] & 0xF0) | (data_3_0 & 0x0F);
			data[66] = (data[66] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case VBN_AMP:
			data[68] = (data[68] & 0xF0) | (data_3_0 & 0x0F);
			data[70] = (data[70] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case SF:
			data[72] = (data[72] & 0xF0) | (data_3_0 & 0x0F);
			data[74] = (data[74] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case TAIL:
			data[76] = (data[76] & 0xF0) | (data_3_0 & 0x0F);
			data[78] = (data[78] & 0xF0) | (data_7_4 & 0x0F);
			break;
		case TEST:
			data[80] = (data[80] & 0xF0) | (data_3_0 & 0x0F);
			data[82] = (data[82] & 0xF0) | (data_7_4 & 0x0F);
			break;
	    case SPI_CLK_FE_SEL_DATA:
	    	//data[124] |= (data_3_0 & 0x1) << 2;
	    	if (data_3_0)
				data[124] |= (1 << 2);   // Set bit
			else
				data[124] &= ~(1 << 2);  // Clear bit
	        break;
	    case BACKSIDE_DEBUG_EN_DATA:
	    	//data[124] |= (data_3_0 & 0x1) << 3;
	    	if (data_3_0)
				data[124] |= (1 << 3);   // Set bit
			else
				data[124] &= ~(1 << 3);  // Clear bit
	        break;
	    case VREF_EN_DATA:
	    	//data[124] |= (data_3_0 & 0x1) << 4;
	    	if (data_3_0)
				data[124] |= (1 << 4);   // Set bit
			else
				data[124] &= ~(1 << 4);  // Clear bit
	        break;
	    case BACKSIDE_LOW_LEAK_EN_DATA:
	    	//data[124] |= (data_3_0 & 0x1) << 5;
	    	if (data_3_0)
				data[124] |= (1 << 5);   // Set bit
			else
				data[124] &= ~(1 << 5);  // Clear bit
	        break;
	    case BACKSIDE_INJECT_EN_DATA:
	    	//data[124] |= (data_3_0 & 0x1) << 6;
	    	if (data_3_0)
				data[124] |= (1 << 6);   // Set bit
			else
				data[124] &= ~(1 << 6);  // Clear bit
	        break;
	    case ANALOG_OUT_0_EN_DATA:
	    	//data[126] |= (data_3_0 & 0x1) << 0;
	    	if (data_3_0)
				data[126] |= (1 << 0);   // Set bit
			else
				data[126] &= ~(1 << 0);  // Clear bit
	        break;
	    case ANALOG_OUT_1_EN_DATA:
	    	//data[126] |= (data_3_0 & 0x1) << 1;
	    	if (data_3_0)
				data[126] |= (1 << 1);   // Set bit
			else
				data[126] &= ~(1 << 1);  // Clear bit
	        break;
	    case ANALOG_OUT_2_EN_DATA:
	    	//data[126] |= (data_3_0 & 0x1) << 2;
	    	if (data_3_0)
				data[126] |= (1 << 2);   // Set bit
			else
				data[126] &= ~(1 << 2);  // Clear bit
	        break;
	    case ANALOG_OUT_3_EN_DATA:
	    	//data[126] |= (data_3_0 & 0x1) << 3;
	    	if (data_3_0)
				data[126] |= (1 << 3);   // Set bit
			else
				data[126] &= ~(1 << 3);  // Clear bit
	        break;
	    case BACKSIDE_EN_DATA:
	    	//data[126] |= (data_3_0 & 0x1) << 4;
	    	if (data_3_0)
				data[126] |= (1 << 4);   // Set bit
			else
				data[126] &= ~(1 << 4);  // Clear bit
	    	break;
	    case TEMP_SENS_EN_DATA:
	        //data[126] |= (data_3_0 & 0x1) << 5;
	        if (data_3_0)
				data[126] |= (1 << 5);   // Set bit
			else
				data[126] &= ~(1 << 5);  // Clear bit
	        break;
	    case ADC_PIN_EN_DATA:
	    	if (data_3_0)
	    		data[126] |= (1 << 6);   // Set bit
	    	else
	    		data[126] &= ~(1 << 6);  // Clear bit
	        break;
	    default:
	        // Handle unknown case
	    	log_error("Wrong DAC");
	        break;
	}

}

void spx3_default_global_config(uint8_t *data) {
    // Initialize data values based on the constants

	uint16_t vthr = 0x200;
	uint8_t vthr_4_0 = 0;
	uint8_t vthr_9_5 = 0;
	vthr_4_0 = (uint8_t)((vthr) & (0x1F));
	vthr_9_5 = (uint8_t)(vthr >> 5);

	uint16_t vin_n = 0x2db;
	uint8_t vin_n_4_0 = 0;
	uint8_t vin_n_9_5 = 0;
	vin_n_4_0 = (uint8_t)((vin_n) & (0x1F));
	vin_n_9_5 = (uint8_t)(vin_n >> 5);

	uint16_t bal = 0x271;
	uint8_t bal_4_0 = 0;
	uint8_t bal_9_5 = 0;
	bal_4_0 = (uint8_t)((bal) & (0x1F));
	bal_9_5 = (uint8_t)(bal >> 5);



	// VBP_CSA
	data[0] |= 0;    // Bits 3:0
	data[2] |= DEF_VAL;    // Bits 19:16

	// VBN_CSA
	data[4] |= 0;    // Bits 35:32
	data[6] |= DEF_VAL;    // Bits 51:48

	//VFB_CSA
	data[8] |= 0;    // Bits 67:64
	data[10] |= DEF_VAL;   // Bits 83:80

	//VBN_PDH
	data[12] |= 0;   // Bits 99:96
	data[14] |= DEF_VAL;   // Bits 115:112

	//VBP_HYST
	data[16] |= 0;   // Bits 131:128
	data[18] |= DEF_VAL;   // Bits 147:144

	//VBP_COMP
	data[20] |= 0;   // Bits 179:176
	data[22] |= DEF_VAL;   // Bits 163:160

	//VBN_TDAC
	data[24] |= 0;   // Bits 195:192
	data[26] |= DEF_VAL;   // Bits 211:208

	//VBP_LCC
	data[28] |= 0;   // Bits 227:224
	data[30] |= DEF_VAL;   // Bits 243:240

	//VTHR
	data[32] |= vthr_4_0;   	// Bits 260:256		// VTHR[4:0] - 5 bits
	data[34] |= vthr_9_5;		// Bits 276:272		// VTHR[9:5] - 5 bits

	//VIN_N
	data[36] |= vin_n_4_0;   	// Bits 292:288
	data[38] |= vin_n_9_5;		// Bits 308:304

	//BAL
	data[40] |= bal_4_0;   		// Bits 324:320
	data[42] |= bal_9_5;   		// Bits 340:336

	//SAMPLE_LENGHT
	data[44] |= 0;   		// Bits 355:352
	data[46] |= DEF_VAL;    // Bits 371:368

	//DRIVER_BIAS
	data[48] |= 0;   		// Bits 387:384
	data[50] |= DEF_VAL;    // Bits 403:400

	//BUFFER_BIAS
	data[52] |= 0;   		// Bits 419:416
	data[54] |= DEF_VAL;    // Bits 435:432

	//LVDS_CM
	data[56] |= 0;   		// Bits 451:448
	data[58] |= DEF_VAL;    // Bits 467:464

	//LVDS_STRENGTH
	data[60] |= 0;   		// Bits 483:480
	data[62] |= DEF_VAL;    // Bits 499:496

	//VBP_AMP
	data[64] |= 0;   		// Bits 515:512
	data[66] |= DEF_VAL;    // Bits 531:528

	//VBN_AMP
	data[68] |= 0;   		// Bits 547:544
	data[70] |= DEF_VAL;    // Bits 563:560

	//SF
	data[72] |= 0;   		// Bits 579:576
	data[74] |= DEF_VAL;    // Bits 595:592

	//TAIL
	data[76] |= 0;   		// Bits 611:608
	data[78] |= DEF_VAL;    // Bits 627:624

	//TEST
	data[80] |= 0;   		// Bits 643:640
	data[82] |= DEF_VAL;    // Bits 659:656


	data[124] |= SPI_CLK_FE_SEL << 2;       // Bits 994			// SPI_CLK_FE_SEL
	data[124] |= BACKSIDE_DEBUG_EN << 3;    // Bits 995			// BACKSIDE_DEBUG_EN
	data[124] |= VREF_EN << 4;      		// Bits 996			// VREF_EN
	data[124] |= BACKSIDE_LOW_LEAK_EN << 5; // Bits 997			// BACKSIDE_LOW_LEAK_EN
	data[124] |= BACKSIDE_INJECT_EN << 6;   // Bits 998			// BACKSIDE_INJECT_EN
	data[126] |= ANALOG_OUT_0_EN << 0;      // Bits 1008		// ANALOG_OUT_0_EN
	data[126] |= ANALOG_OUT_1_EN << 1;      // Bits 1009		// ANALOG_OUT_1_EN
	data[126] |= ANALOG_OUT_2_EN << 2;      // Bits 1010		// ANALOG_OUT_2_EN
	data[126] |= ANALOG_OUT_3_EN << 3;      // Bits 1011		// ANALOG_OUT_3_EN
	data[126] |= BACKSIDE_EN 	 << 4;      // Bits 1012		// BACKSIDE_EN
	data[126] |= TEMP_SENS_EN 	 << 5;      // Bits 1013		// TEMP_SENS_EN
	data[126] |= ADC_PIN_EN 	 << 6;      // Bits 1014		// ADC_PIN_EN
}

#if 0

uint8_t spx3_digital_test(void)
{
	uint8_t result = SPX_OK;

	uint8_t pattern = PATTERN_8;
# if 1 // DIGITAL TEST
	// TX Pixel Matrix Config
	const uint16_t counter_tx_pattern_length = SPX3_PX_MATRIX_BLOCK_SIZE;			// sending 1024b -> 128B per ROW
	uint8_t *counter_tx_pattern = (uint8_t *)malloc( counter_tx_pattern_length );
	if( counter_tx_pattern == NULL ){
		log_error("counter_tx_pattern == NULL");
		return SPX_FAILED;
	}
	// TODO : neccessary to set with equalization
	// Generating TEST pattern for Pixel Matrix Config -> make function
	for( int i = 0; i < counter_tx_pattern_length ; i++ )
	{
		counter_tx_pattern[i] = pattern_gen(pattern, i);
	}

	// RX Pixel Matrix Config
	const uint32_t counter_rx_pattern_length = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE+1);
	uint8_t *counter_rx_pattern = (uint8_t *)malloc( counter_rx_pattern_length );
	memset( counter_rx_pattern, 0x00, counter_rx_pattern_length );
	if( counter_rx_pattern == NULL ){
		log_error("counter_rx_pattern == NULL");
		return SPX_FAILED;
	}

	const uint16_t global_config_tx_len = SPX3_PX_MATRIX_BLOCK_SIZE;			// sending 1024b
	uint8_t *global_config_tx = (uint8_t *)malloc( global_config_tx_len );
	if( global_config_tx == NULL ){
		log_error("global_config_tx == NULL");
		return SPX_FAILED;
	}
	memset( global_config_tx, 0x00, global_config_tx_len );

	const uint16_t global_config_rx_len = SPX3_PX_MATRIX_BLOCK_SIZE*2;			// sending 1024b
	uint8_t *global_config_rx = (uint8_t *)malloc( global_config_rx_len );
	if( global_config_rx == NULL ){
		log_error("global_config_tx == NULL");
		return SPX_FAILED;
	}
	uint16_t vthr = VTHR_DEFAULT;
	init_global_config(global_config_tx, vthr, TEST_DEFAULT);
	memcpy( &spxReadOutConfig.global_config[0], global_config_tx, SPX3_PX_MATRIX_BLOCK_SIZE );

	spx3_pixel_matrix_config(counter_tx_pattern, counter_rx_pattern); // immedietly after should be set global config
	spx3_global_config(global_config_tx, global_config_rx);

	// Control if SET COUNTER = GET COUNTER
	bool pattern_err = false;
	uint8_t tmp = 0;
	for( int i = 128; i < counter_rx_pattern_length; i++ )
	{
		if(tmp > 127){	// bcs we are generating only 128 pattern not 255
			tmp = 0;
		}
		uint16_t tmp_gen =  pattern_gen( pattern, tmp);
		uint16_t tmp_rx = counter_rx_pattern[i];
		if( tmp_rx != tmp_gen){					// there is offset bcs of DS of SPX
			log_error("Pixel matrix fail on index %d", i);
			pattern_err = true;
			SAFE_FREE( counter_rx_pattern );
			return SPX_FAILED;
		}
		tmp++;
	}
	log_debug("spx3_pixel_matrix_config OK");
	SAFE_FREE(counter_tx_pattern);
	SAFE_FREE(counter_rx_pattern);
	SAFE_FREE(global_config_tx);
	SAFE_FREE(global_config_rx);
#endif


	return result;
}

#endif

// spx3Dacs -> FeedbackDacs
void UpdateSpx3Dacs( uint16_t *spx3Dacs, uint8_t tpx2DacsLength)
{

	// Readout Global Config from SpacePix3 -> after extract data of DACs // only able to extract digital value

	uint8_t rx_buf[256] = {0};
	// TODO think about if to load again config
	spx3_global_config(spxReadOutConfig.global_config, rx_buf);

	// in spxReadOutConfig.global_config is current settings of DACs
	// edit where data will be start in rx_buf !!
	for(uint8_t i = 0; i < NUM_OF_SPX3_DACS; i++){
		// TODO -> need to for exact dacs cut exact part of rx_buf -> response what is inside of dacs
		spx3Dacs[i] = rx_buf[i];
		uint16_t tmp = 0;
		switch (i) {
			case VBP_CSA:
				// Handle VBP_CSA
				tmp |= (rx_buf[125] & (0xF)) << 4;
				tmp |=  rx_buf[127] & (0xF);
				spx3Dacs[i] = tmp;

				break;
			case VBN_CSA:
				tmp = 0;
				tmp |= (rx_buf[121] & (0xF)) << 4;
				tmp |=  rx_buf[123] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case VFB_CSA:
				tmp = 0;
				tmp |= (rx_buf[117] & (0xF)) << 4;
				tmp |=  rx_buf[119] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case VBN_PDH:
				tmp = 0;
				tmp |= (rx_buf[113] & (0xF)) << 4;
				tmp |=  rx_buf[115] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case VBP_HYST:
				tmp = 0;
				tmp |= (rx_buf[109] & (0xF)) << 4;
				tmp |=  rx_buf[111] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case VBP_COMP:
				tmp = 0;
				tmp |= (rx_buf[105] & (0xF)) << 4;
				tmp |=  rx_buf[107] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case VBN_TDAC:
				tmp = 0;
				tmp |= (rx_buf[101] & (0xF)) << 4;
				tmp |=  rx_buf[103] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case VBP_LCC:
				tmp = 0;
				tmp |= (rx_buf[97] & (0xF)) << 4;
				tmp |=  rx_buf[99] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case VTHR:
				tmp = 0;
				tmp |= (rx_buf[93] & (0x1F)) << 5;
				tmp |=  rx_buf[95] & (0x1F);
				spx3Dacs[i] = tmp;
				break;
			case VIN_N:
				tmp = 0;
				tmp |= (rx_buf[89] & (0x1F)) << 5;
				tmp |=  rx_buf[91] & (0x1F);
				spx3Dacs[i] = tmp;
				break;
			case BAL:
				tmp = 0;
				tmp |= (rx_buf[85] & (0x1F)) << 5;
				tmp |=  rx_buf[87] & (0x1F);
				spx3Dacs[i] = tmp;
				break;
			case SAMPLE_LENGTH:
				tmp = 0;
				tmp |= (rx_buf[81] & (0xF)) << 4;
				tmp |=  rx_buf[83] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case DRIVER_BIAS:
				tmp = 0;
				tmp |= (rx_buf[77] & (0xF)) << 4;
				tmp |=  rx_buf[79] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case BUFFER_BIAS:
				tmp = 0;
				tmp |= (rx_buf[73] & (0xF)) << 4;
				tmp |=  rx_buf[75] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case LVDS_CM:
				tmp = 0;
				tmp |= (rx_buf[69] & (0xF)) << 4;
				tmp |=  rx_buf[71] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case LVDS_STRENGTH:
				tmp = 0;
				tmp |= (rx_buf[65] & (0xF)) << 4;
				tmp |=  rx_buf[67] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case VBP_AMP:
				tmp = 0;
				tmp |= (rx_buf[61] & (0xF)) << 4;
				tmp |=  rx_buf[63] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case VBN_AMP:
				tmp = 0;
				tmp |= (rx_buf[57] & (0xF)) << 4;
				tmp |=  rx_buf[59] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case SF:
				tmp = 0;
				tmp |= (rx_buf[53] & (0xF)) << 4;
				tmp |=  rx_buf[55] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case TAIL:
				tmp = 0;
				tmp |= (rx_buf[49] & (0xF)) << 4;
				tmp |=  rx_buf[51] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case TEST:
				tmp = 0;
				tmp |= (rx_buf[45] & (0xF)) << 4;
				tmp |=  rx_buf[47] & (0xF);
				spx3Dacs[i] = tmp;
				break;
			case SPI_CLK_FE_SEL_DATA:
				tmp = 0;
				tmp |= (rx_buf[3] & 0b00000100) >> 2;
				spx3Dacs[i] = tmp;
				break;
			case BACKSIDE_DEBUG_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[3] & 0b00001000) >> 3;
				spx3Dacs[i] = tmp;
				break;
			case VREF_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[3] & 0b00010000) >> 4;
				spx3Dacs[i] = tmp;
				break;
			case BACKSIDE_LOW_LEAK_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[3] & 0b00000010) >> 1;
				spx3Dacs[i] = tmp;
				break;
			case BACKSIDE_INJECT_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[3] & 0b00000001) >> 0;
				spx3Dacs[i] = tmp;
				break;
			case ANALOG_OUT_0_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[1] & 0b00000001) >> 0;
				spx3Dacs[i] = tmp;
				break;
			case ANALOG_OUT_1_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[1] & 0b0000010) >>1;
				spx3Dacs[i] = tmp;
				break;
			case ANALOG_OUT_2_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[1] & 0b0000100) >>2;
				spx3Dacs[i] = tmp;
				break;
			case ANALOG_OUT_3_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[1] & 0b0001000) >>3;
				spx3Dacs[i] = tmp;
				break;
			case BACKSIDE_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[1] & 0b0010000) >>4;
				spx3Dacs[i] = tmp;
				break;
			case TEMP_SENS_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[1] & 0b0100000) >>5;
				spx3Dacs[i] = tmp;
				break;
			case ADC_PIN_EN_DATA:
				tmp = 0;
				tmp |= (rx_buf[1] & 0b1000000) >>6;
				spx3Dacs[i] = tmp;
				break;
			default:
				// Handle unknown value
				break;
		    }
	}
}

#if 1
uint8_t spx3_set_tdac(uint8_t *tx_buf, SPX3_TDAC tdac)
{
	uint8_t ret = SPX_OK;

	switch (tdac) {
		case TDAC_DEFAULT:
			// default TDAC [3:0] = 0b1000 = 0x8
			// -> one px = 0x0008 // 16 bit per pixel
			for(uint32_t i = 0; i < SPX3_PX_MATRIX_BLOCK_SIZE*64; i++){
				if(i%2 == 0){
					tx_buf[i] = 0x00;
				}else{
					tx_buf[i] = 0x08;
				}
			}
			break;
		case TDAC_MAX:
			// default TDAC [3:0] = 0b1000 = 0xf
			// -> one px = 0x000f // 16 bit per pixel
			for(uint32_t i = 0; i < SPX3_PX_MATRIX_BLOCK_SIZE*64; i++){
				if(i%2 == 1){
					tx_buf[i] = 0x00;
				}else{
					tx_buf[i] = 0x0f;
				}
			}
			break;
		case TDAC_MIN:
			// default TDAC [3:0] = 0b1000 = 0x0
			// -> one px = 0x0000 // 16 bit per pixel
			for(uint32_t i = 0; i < SPX3_PX_MATRIX_BLOCK_SIZE*64; i++){
				if(i%2 == 1){
					tx_buf[i] = 0x0;
				}else{
					tx_buf[i] = 0x0;
				}
			}
			break;
		default:
			break;
	}
	return ret;

}
#endif


void spx3_get_vssa(float *vssa)
{
	//write_ltc2635(INJ_EN, 0x0);	// INJECT : LOW
	board_spx3_set_inj_en(SPXA, 0);
	Delay(10);
	spx3_set_dac(spxReadOutConfig.global_config, TEMP_SENS_EN_DATA, 0);
	spx3_set_dac(spxReadOutConfig.global_config, ADC_PIN_EN_DATA, 1);
	Delay(10);
	spx3_global_config(spxReadOutConfig.global_config, NULL);

	const uint32_t rx_data_length = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE*2+1);
	uint8_t *rx_data = (uint8_t *)malloc( rx_data_length );
	memset( rx_data, 0x00, rx_data_length );
	if( rx_data == NULL ){
		log_error("counter_rx_pattern == NULL");
	}
	Delay(10);
	spx3_data_readout(rx_data, 10);

	// Average meas data from 4096 ADC
	uint32_t tmp = 0;
	for(int i = 0; i < 4096*2; i+=2){	// SEND measurment data // one loop means send one pixel, see doc.
		// Mereni1 - start
		uint16_t tmp_px = ((rx_data[i]) << 8) | rx_data[i+1];
		tmp+= tmp_px;
	}

	*vssa = tmp/4096;
	// Return SPX3 into DEFAULT
	spx3_global_config(spxReadOutConfig.global_config_default, NULL);
	SAFE_FREE(rx_data);

}

void spx3_get_temp(float *temp)
{
	//write_ltc2635(INJ_EN, 0x0);	// INJECT : LOW
	board_spx3_set_inj_en(SPXA, 0);
	Delay(10);
	spx3_set_dac(spxReadOutConfig.global_config, TEMP_SENS_EN_DATA, 1);
	spx3_set_dac(spxReadOutConfig.global_config, ADC_PIN_EN_DATA, 1);
	Delay(10);
	spx3_global_config(spxReadOutConfig.global_config, NULL);

	const uint32_t rx_data_length = SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE*2+1);
	uint8_t *rx_data = (uint8_t *)malloc( rx_data_length );
	memset( rx_data, 0x00, rx_data_length );
	if( rx_data == NULL ){
		log_error("counter_rx_pattern == NULL");
	}
	Delay(10);
	spx3_data_readout(rx_data, 10);

	// Average meas data from 4096 ADC
	uint32_t tmp = 0;
	for(int i = 0; i < 4096*2; i+=2){	// SEND measurment data // one loop means send one pixel, see doc.
		// Mereni1 - start
		uint16_t tmp_px = ((rx_data[i]) << 8) | rx_data[i+1];
		tmp+= tmp_px;
	}
	log_debug("Temp all: %d", tmp);

	*temp = (float)(tmp/4096);
	// Return SPX3 into DEFAULT
	spx3_global_config(spxReadOutConfig.global_config_default, NULL);
	SAFE_FREE(rx_data);
}

void spx3_data_readout_adcin(uint8_t *rx_buf, uint32_t acq_time, uint32_t adc_value)
{
	write_ltc2635(ADC_IN, adc_value);
	//write_ltc2635(INJ_EN, DAC_MAX_VALUE);	// INJECT : HIGH
	board_spx3_set_inj_en(SPXA, 1);
	Delay(10);
	spx3_set_dac(spxReadOutConfig.global_config, TEMP_SENS_EN_DATA, 0);
	spx3_set_dac(spxReadOutConfig.global_config, ADC_PIN_EN_DATA, 1);
	Delay(10);
	spx3_global_config(spxReadOutConfig.global_config, NULL);

	uint8_t result = SPX_OK;

	uint8_t tx_buf_n[N] = {0x00};
	for(uint8_t i = 0; i < N; i++){
		tx_buf_n[i] = 0xff;
	}

	uint8_t tx_readout[128] = {0x0};

	// idle state definition os signal
#if 0
	board_spx3_set_row_shift(SPXA, 0);
	board_spx3_set_cnt_en(SPXA, 0);
	board_spx3_set_chip_select(SPXA, 0);
	board_spx3_set_cnf_en(SPXA, 0);
#endif

#if 0
	board_spx3_set_cnf_en(SPXA, 0);
	board_spx3_set_row_shift(SPXA, 0);

	Delay(1);
	board_spx3_set_clr_n(SPXA, 1);
#endif

	Delay(1);
	board_spx3_set_clr_n(SPXA, 0);
	Delay(1);
	board_spx3_set_clr_n(SPXA, 1);
	Delay(1);

	Delay(20);

	// Shutter - exposition
#if 0
	board_spx3_set_cnt_en(SPXA, 1);
	//Delay(1000);
	board_spx3_set_cnt_en(SPXA, 0);
#endif

	// Shutter - exposition
	process_shutter_spx3(acq_time);

	// Bus charging
	Delay(1);
	board_spx3_set_row_shift(SPXA, 1);
	Delay(1);
	board_spx3_set_row_shift(SPXA, 0);
	Delay(1);


	// Id - Nx
	result = board_spi_transmit(TPXA, tx_buf_n, sizeof(tx_buf_n));
	Delay(1);

	for(uint8_t row = 0; row < SPX3_ROW_SIZE; row++){

		// TODO check row status signal
		board_spx3_set_row_shift(SPXA, 1);
		Delay(10);

		result = board_spi_receive(TPXA, (uint8_t *)&rx_buf[row*128], SPX3_PX_MATRIX_BLOCK_SIZE);
		result = board_spi_transmit(TPXA, tx_readout, SPX3_PX_MATRIX_BLOCK_SIZE);

		Delay(10);
		board_spx3_set_row_shift(SPXA, 0);

		// Id - Nx
		result = board_spi_transmit(TPXA, tx_buf_n, sizeof(tx_buf_n));
		Delay(10);
	}

	spx3_set_dac(spxReadOutConfig.global_config, TEMP_SENS_EN_DATA, 0);
	spx3_set_dac(spxReadOutConfig.global_config, ADC_PIN_EN_DATA, 0);
	Delay(10);
	spx3_global_config(spxReadOutConfig.global_config, NULL);
}
