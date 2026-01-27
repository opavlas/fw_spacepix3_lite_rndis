/*
 * board.c
 *
 */

#include "board.h"
#include <debug.h>
#include "pinout.h"
#include "ux_api.h"
#include "tpx2_comm.h"
#include "udp_cmd_handler.h"


uint8_t board_init(void)
{
#if defined(BOARD_STM32U5)
	// All peripheral are initialized external from CodeGenerator
#elif defined(BOARD_STM32G4)
	// All peripheral are initialized external from CodeGenerator
#elif defined(BOARD_VA416X0)
	// need to do all init
	hal_status_t status = hal_status_ok;
	status = HAL_VA_Init();
	/* Configure the system clock */
	status = CLK_VA_Init();
	/* Initialize all configured peripherals */
	status = SPI2_VA_Init();
	status = GPIO_VA_Init();
	status = UART2_VA_Init();
	return status;
#else
	log_error("Not implemented!");
#endif
	return BOARD_OK;
}

uint8_t board_stop_mclock()
{
#if defined(BOARD_STM32U5)
	if(HAL_TIM_PWM_Stop(MCLOCK_NUMBER, MCLOCK_CHANEL) != HAL_OK){
		log_error("board_stop_mclock()");
		return BOARD_FAILED;
	}
#else
	log_error("Board is not defined");
#endif
	return BOARD_OK;
}

uint8_t board_start_mclock(TOT_FREQ tot)
{
	/* Example:
	// START 20Mhz MCLOCK
	// CLK for TIM15 -> CLK = 160MhZ
	// PRESCALER = 4 - 1
	// ARR = 2 - 1
	// -> f_PWM = (CLK/PRE)/ARR				, [Mhz]
	// -> duty_PWM = 100* (TIM15->CCR2)/ARR , [%]
	 */

	uint32_t pre = 0;
	uint32_t arr = 0;
	uint32_t freq_Hz = 5000;

	// DEFAULT
	uint32_t ccr2 = 1;

#if defined(BOARD_STM32U5)
		switch (tot) {
			case Frequency120:
				break;
			case Frequency100:
				break;
			case Frequency80:
				TIM15->PSC = 1-1;
				TIM15->ARR = 2-1;		// f_MCLOCK = (160/1)/2 = 80 MHz
				TIM15->CCR2 = 1;
				break;
			case Frequency50:
				TIM15->PSC = 2-1;
				TIM15->ARR = 2-1;		// f_MCLOCK = (160/2)/2 = 40 MHz
				TIM15->CCR2 = 1;
				break;
			case Frequency25:
				TIM15->PSC = 2-1;
				TIM15->ARR = 4-1;		// f_MCLOCK = (160/1)/6 = 20 MHz
				TIM15->CCR2 = 2;
				break;
			case Frequency10:
				TIM15->PSC = 5-1;
				TIM15->ARR = 2-1;		// f_MCLOCK = (160/5)/2 = 16 MHz
				TIM15->CCR2 = 1;
				break;
			case Frequency5:
				TIM15->PSC = 20-1;
				TIM15->ARR = 2-1;		// f_MCLOCK = (160/20)/2 = 4 Mhz
				TIM15->CCR2 = 1;
				break;
			case Frequency_HV_150V:
				arr = 1000;
				// duty -> 0.1%
				pre = 160000000/(freq_Hz*arr);
				TIM15->PSC = pre-1;
				TIM15->ARR = arr-1;
				TIM15->CCR2 = ccr2;
				break;
			case TotDisable:
				board_stop_mclock();
				break;
			default:
				break;
		}
		if(HAL_TIM_PWM_Start(MCLOCK_NUMBER, MCLOCK_CHANEL) != HAL_OK){
			log_error("Start MCLOCK freq: %d", tot);
			return BOARD_FAILED;
		} else {
			log_debug("Start MCLOCK freq: %d", tot);
		}
#elif defined(BOARD_STM32G4)
	if(tpxIndex == TPXA){
		TIM1->CCR2 = 5;				// TIM1_CH2N
		//HAL_TIM_Base_Start(TPX2_TOP_MCLOCK);
		if(HAL_TIMEx_PWMN_Start(TPX2_TOP_MCLOCK, TIM_CHANNEL_2) != HAL_OK){
			return BOARD_FAILED;
		}
	}else if(tpxIndex == TPXB){
		TIM3->CCR4 = 5;				// TIM3_CH4
		//HAL_TIM_Base_Start(TPX2_BOT_MCLOCK);
		if(HAL_TIM_PWM_Start(TPX2_BOT_MCLOCK,TIM_CHANNEL_4) != HAL_OK){
			return BOARD_FAILED;
		}
	}else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_VA416X0)
	if(tpxIndex == TPXA){
		if(HAL_Timer_SetupPWMA(TPX2_TOP_MCLOCK, TPX2_TOP_MCLOCK_SPEED, TPX2_TOP_MCLOCK_DUTY_50); != hal_status_ok){
			return BOARD_FAILED;
		}
	}else if(tpxIndex == TPXB){
		// TODO TPX2_BOT
		if(HAL_Timer_SetupPWMA(TPX2_TOP_MCLOCK, TPX2_TOP_MCLOCK_SPEED, TPX2_TOP_MCLOCK_DUTY_50); != hal_status_ok){
					return BOARD_FAILED;
		}
	}else {
		log_error("Not implemented!");
	}
#else
	#error "Board is not defined!!"
#endif
	return BOARD_OK;
}

void board_tpx2_set_pwr_en2v5( TIMEPIX_ID tpxIndex, uint8_t state )
{
#if defined(BOARD_STM32U5)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_EN_2V5_port, TPX2_EN_2V5_pin, state ? GPIO_SET : GPIO_RESET);
	} else {
		log_debug("Not implemented!");
	}
#endif
}

void board_tpx2_set_pwr_en1v2( TIMEPIX_ID tpxIndex, uint8_t state )
{
#if defined(BOARD_STM32U5)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_PWR_EN_port, TPX2_PWR_EN_pin, state ? GPIO_SET : GPIO_RESET);
	} else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_STM32G4)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_TOP_EN_1V8_port, TPX2_TOP_EN_1V8_pin, state ? GPIO_SET : GPIO_RESET);
	}else if( tpxIndex == TPXB ){
		HAL_GPIO_WritePin(TPX2_BOT_EN_1V8_port, TPX2_BOT_EN_1V8_pin, state ? GPIO_SET : GPIO_RESET);
	} else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_VA416X0)
	if( tpxIndex == TPXA ){
		GPIO_WritePin(TPX2_TOP_EN_1V8_port, TPX2_TOP_EN_1V8_pin, state ? GPIO_SET : GPIO_RESET);
	}else if( tpxIndex == TPXB ){
		GPIO_WritePin(TPX2_BOT_EN_1V8_port, TPX2_BOT_EN_1V8_pin, state ? GPIO_SET : GPIO_RESET);
	} else {
		log_error("Not implemented!");
	}
#else
	#error "Board is not defined!!"
#endif
}

void board_tpx2_set_burn_en( TIMEPIX_ID tpxIndex, uint8_t state )
{
#if defined(BOARD_STM32U5)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_BURN_EN_port, TPX2_BURN_EN_pin, state ? GPIO_SET : GPIO_RESET);
	} else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_STM32G4)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_TOP_BURN_EN_port, TPX2_TOP_BURN_EN_pin, state ? GPIO_SET : GPIO_RESET);
	}else if( tpxIndex == TPXB ){
		//HAL_GPIO_WritePin(TPX2_BOT_EN_1V8_port, TPX2_BOT_EN_1V8_pin, state ? GPIO_SET : GPIO_RESET);
	} else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_VA416X0)
	if( tpxIndex == TPXA ){
		//GPIO_WritePin(TPX2_TOP_EN_1V8_port, TPX2_TOP_EN_1V8_pin, state ? GPIO_SET : GPIO_RESET);
	}else if( tpxIndex == TPXB ){
		//GPIO_WritePin(TPX2_BOT_EN_1V8_port, TPX2_BOT_EN_1V8_pin, state ? GPIO_SET : GPIO_RESET);
	} else {
		log_error("Not implemented!");
	}
#else
	#error "Board is not defined!!"
#endif
}

void board_tpx2_set_global_reset( TIMEPIX_ID tpxIndex, uint8_t state )
{
#if defined(BOARD_STM32U5)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_GLOBAL_RESET_port, TPX2_GLOBAL_RESET_pin, state ? GPIO_RESET : GPIO_SET );
	} else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_STM32G4)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_TOP_GLOBAL_RESET_port, TPX2_TOP_GLOBAL_RESET_pin, state ? GPIO_RESET : GPIO_SET );
	}else if( tpxIndex == TPXB ){
		HAL_GPIO_WritePin(TPX2_BOT_GLOBAL_RESET_port, TPX2_BOT_GLOBAL_RESET_pin, state ? GPIO_RESET : GPIO_SET );
	} else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_VA416X0)
	if( tpxIndex == TPXA ){
		GPIO_WritePin(TPX2_TOP_GLOBAL_RESET_port, TPX2_TOP_GLOBAL_RESET_pin, state ? GPIO_RESET : GPIO_SET );
	}else if( tpxIndex == TPXB ){
		GPIO_WritePin(TPX2_BOT_GLOBAL_RESET_port, TPX2_BOT_GLOBAL_RESET_pin, state ? GPIO_RESET : GPIO_SET );
	} else {
		log_error("Not implemented!");
	}

#else
	#error "Board is not defined!!"
#endif
}

void board_tpx2_set_chip_select( TIMEPIX_ID tpxIndex, uint8_t state )
{
#if defined(BOARD_STM32U5)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_nCS_IN_port, TPX2_nCS_IN_pin, state ? GPIO_RESET : GPIO_SET );
	}else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_STM32G4)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_TOP_nCS_port, TPX2_TOP_nCS_pin, state ? GPIO_RESET : GPIO_SET );
	}else if( tpxIndex == TPXB ){
		HAL_GPIO_WritePin(TPX2_BOT_nCS_port, TPX2_BOT_nCS_pin, state ? GPIO_RESET : GPIO_SET );
	}else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_VA416X0)
	if( tpxIndex == TPXA ){
		GPIO_WritePin(TPX2_TOP_nCS_port, TPX2_TOP_nCS_pin, state ? GPIO_RESET : GPIO_SET );
	}else if( tpxIndex == TPXB ){
		GPIO_WritePin(TPX2_BOT_nCS_port, TPX2_BOT_nCS_pin, state ? GPIO_RESET : GPIO_SET );
	}else {
		log_error("Not implemented!");
	}
#else
	#error "Board is not defined!!"
#endif
}

void board_tpx2_set_shutter_counter( TIMEPIX_ID tpxIndex, uint8_t state )
{
#if defined(BOARD_STM32U5)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_SHUTTER_port, TPX2_SHUTTER_pin, state ? GPIO_SET : GPIO_RESET);

	}else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_STM32G4)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_TOP_SHUTTER_COUNTER_port, TPX2_TOP_SHUTTER_COUNTER_pin, state ? GPIO_SET : GPIO_RESET);
	}else if( tpxIndex == TPXB ){
		HAL_GPIO_WritePin(TPX2_BOT_SHUTTER_COUNTER_port, TPX2_BOT_SHUTTER_COUNTER_pin, state ? GPIO_SET : GPIO_RESET);
	}else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_VA416X0)
	if( tpxIndex == TPXA ){
		GPIO_WritePin(TPX2_TOP_SHUTTER_COUNTER_port, TPX2_TOP_SHUTTER_COUNTER_pin, state ? GPIO_SET : GPIO_RESET);
	}else if( tpxIndex == TPXB ){
		GPIO_WritePin(TPX2_BOT_SHUTTER_COUNTER_port, TPX2_BOT_SHUTTER_COUNTER_pin, state ? GPIO_SET : GPIO_RESET);
	}else {
		log_error("Not implemented!");
	}

#else
	#error "Board is not defined!!"
#endif
}

void board_tpx2_set_hv_monitor(TIMEPIX_ID tpxIndex, uint8_t state){
#if defined(BOARD_STM32U5)
	if( tpxIndex == TPXA ){
		HAL_GPIO_WritePin(TPX2_EN_MONITOR_port, TPX2_EN_MONITOR_pin, state ? GPIO_SET : GPIO_RESET);
	}else {
		log_error("Not implemented!");
	}
#else
	#error "Board is not defined!!"
#endif
}

uint8_t board_tpx2_get_temp(TIMEPIX_ID tpxId, float *data){
#if defined(BOARD_STM32U5)
	if(tpxId == TPXA){
		uint8_t rx_buf[2] = {0};
		uint8_t tx_buf[1] = {TEMP_REG};
		int16_t val = 0;
		float temp_c = 0;
		uint8_t addr = 0x48 << 1;

		// SET resulution of sensor -> 12 bits
		uint8_t tx_config_buf[3] = {CONFIG_REG, 0x60, 0x00};
		if(HAL_I2C_Master_Transmit(I2C_PERIPHERY, addr, tx_config_buf, 3, I2C_DEFAULT_TPX_TIMEOUT) != HAL_OK){	// choose REG for reading
			return BOARD_FAILED;
		}

		if(HAL_I2C_Master_Transmit(I2C_PERIPHERY, addr, tx_buf, 1, I2C_DEFAULT_TPX_TIMEOUT) != HAL_OK){	// choose REG for reading
			return BOARD_FAILED;
		}
		if(HAL_I2C_Master_Receive(I2C_PERIPHERY, addr, rx_buf, 2, I2C_DEFAULT_TPX_TIMEOUT) != HAL_OK){	// read REG
			return BOARD_FAILED;
		}
		//Combine the bytes
		val = ((int16_t)rx_buf[0] << 4) | (rx_buf[1] >> 4);
		// Convert to 2's complement, since temperature can be negative
		if ( val > 0x7FF ) {
		  val |= 0xF000;
		}
		// Convert to float temperature value (Celsius)
		temp_c = val * 0.0625;

		*data = temp_c;
	}else{
		log_error("Not implemented!");
	}
	return BOARD_OK;
#endif
}



uint8_t board_tpx2_get_dacout(TIMEPIX_ID tpxId, float *data){
#if defined(BOARD_STM32U5)
	if(tpxId == TPXA){
		//Delay(100);
		HAL_ADCEx_Calibration_Start(ADC_DACOUT, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
		//HAL_ADC_Start(ADC_DACOUT);
		float raw_adc_value = 0;
		float adc_value = 0;
		float raw_adc_value_counter = 0;
		for(int i = 0; i < SIZE_ADC_DACOUT_BUF; i++){
			HAL_ADC_Start(ADC_DACOUT);
			//Delay(10);
			HAL_ADC_PollForConversion(ADC_DACOUT, ADC_DEFAULT_TPX_TIMEOUT);
			raw_adc_value_counter += HAL_ADC_GetValue(ADC_DACOUT);
			HAL_ADC_Stop(ADC_DACOUT);
			//Delay(10);
		}
		raw_adc_value = raw_adc_value_counter/SIZE_ADC_DACOUT_BUF;
		adc_value = raw_adc_value*3.3/((1<<ADC_DACOUT_RESULOTION) -1);		// gets adc value in mV

		*data = adc_value;

	}else{
		log_error("Not implemented!");
	}
	return BOARD_OK;
#endif
}

uint8_t board_tpx2_get_HV(TIMEPIX_ID tpxId, float *data){
#if defined(BOARD_STM32U5)
	if(tpxId == TPXA){
		board_tpx2_set_hv_monitor(TPXA, 1);		// turn on HV monitor
		HAL_ADC_Start(ADC_HV);
		float raw_adc_value = 0;
		float adc_value = 0;
		float hv_value = 0;
		float raw_adc_value_counter = 0;
		for(int i = 0; i < SIZE_ADC_HV_BUF; i++){
			HAL_ADC_Start(ADC_HV);
			HAL_ADC_PollForConversion(ADC_HV, ADC_DEFAULT_TPX_TIMEOUT);
			raw_adc_value_counter += HAL_ADC_GetValue(ADC_HV);
			HAL_ADC_Stop(ADC_HV);
		}
		raw_adc_value = raw_adc_value_counter/SIZE_ADC_HV_BUF;
		adc_value = raw_adc_value*3300/((1<<ADC_HV_RESULOTION) -1);		// gets adc value in mV
		hv_value = (adc_value * ((R2+R1)/R2)) / 1000;					// convert meauring voltage into real HV value, after convert to V
		*data = hv_value;
		if(hv_value < 0){
			return BOARD_FAILED;
		}

	}else{
		log_error("Not implemented!");
	}
	return BOARD_OK;
#endif
}

uint8_t board_tpx2_set_HV(TIMEPIX_ID tpxId, float value){
	uint8_t hal_result = 0;
#if defined(BOARD_STM32U5)
	float distance = 0;
	uint16_t index = 0;		// search for index coresponds to float value of hv
	float nearest_float = roundf(value * 100) / 100;
	// corner search
	//left-side case
	if (nearest_float >= hv_value_arr[1]){
		index = 1;
	}
	//right-side case
	if (nearest_float <= hv_value_arr[SIZE_OF_HV_ARR - 1]){
		index = SIZE_OF_HV_ARR - 1;
	}
	for(uint16_t i = 1; i < SIZE_OF_HV_ARR - 1; i++){
		if(hv_value_arr[i] < nearest_float){
			// it will be hv_value_arr[i] or hv_value_arr[i+1]
			distance = (hv_value_arr[i] - hv_value_arr[i + 1]);
			if(distance >= 0.5){
				index = i+1;
			} else{
				index = i;
			}
			break;
		}
	}

	// SET HV
	if(board_tpx2_set_HV_hex(TPXA, index) != BOARD_OK){
		return BOARD_FAILED;
	}

#else
	#error "Board is not defined!!"
#endif
	return BOARD_OK;
}

uint8_t board_tpx2_set_HV_hex(TIMEPIX_ID tpxId, uint8_t data){
	uint8_t hal_result;
#if defined(BOARD_STM32U5)
	uint8_t data_HV[1] = {data};
	if(tpxId == TPXA){
		HAL_GPIO_WritePin(nCS_HV_port, nCS_HV_pin, GPIO_PIN_SET);
		Delay(1);
		HAL_GPIO_WritePin(nCS_HV_port, nCS_HV_pin, GPIO_PIN_RESET);
		hal_result = HAL_SPI_Transmit(SPI_PERIPHERY, data_HV, sizeof(data_HV), SPI_DEFAULT_TPX_TIMEOUT);
		HAL_GPIO_WritePin(nCS_HV_port, nCS_HV_pin, GPIO_PIN_SET);
		if(hal_result != BOARD_OK){
			return BOARD_FAILED;
		}
	}else{
		log_error("Not implemented!");
	}
#elif defined(BOARD_STM32G4)
	if(tpxId == TPXA){
		HAL_GPIO_WritePin(TPX2_TOP_HV_nCS_port, TPX2_TOP_HV_nCS_pin, GPIO_PIN_RESET);
		hal_result = HAL_SPI_Transmit(SPI_PERIPHERY, data,sizeof(data),SPI_DEFAULT_TPX_TIMEOUT);
		HAL_GPIO_WritePin(TPX2_TOP_HV_nCS_port, TPX2_TOP_HV_nCS_pin, GPIO_PIN_SET);
		if(hal_result != HAL_OK){
			return HV_TPXA_FAILED;
		}
	}else if(tpxId == TPXB){
		HAL_GPIO_WritePin(TPX2_BOT_HV_nCS_port, TPX2_BOT_HV_nCS_pin, GPIO_PIN_RESET);
		hal_result = HAL_SPI_Transmit(SPI_PERIPHERY, data,sizeof(data),SPI_DEFAULT_TPX_TIMEOUT);
		HAL_GPIO_WritePin(TPX2_BOT_HV_nCS_port, TPX2_BOT_HV_nCS_pin, GPIO_PIN_SET);
		if(hal_result != HAL_OK){
			return HV_TPXB_FAILED;
		}
	}else{
		log_error("Not implemented!");
	}
#elif defined(BOARD_VA416X0)
	if(tpxId == TPXA){
			GPIO_WritePin(TPX2_TOP_HV_nCS_port, TPX2_TOP_HV_nCS_pin, GPIO_PIN_RESET);
			hal_result = HAL_Spi_XferB8(SPI_PERIPHERY, data, data, sizeof(data), false, 0);
			GPIO_WritePin(TPX2_TOP_HV_nCS_port, TPX2_TOP_HV_nCS_pin, GPIO_PIN_SET);
			if(hal_result != hal_status_ok){
				return HV_TPXA_FAILED;
			}
		}else if(tpxId == TPXB){
			GPIO_WritePin(TPX2_BOT_HV_nCS_port, TPX2_BOT_HV_nCS_pin, GPIO_PIN_RESET);
			hal_result = HAL_Spi_XferB8(SPI_PERIPHERY, data, data, sizeof(data), false, 0);
			GPIO_WritePin(TPX2_BOT_HV_nCS_port, TPX2_BOT_HV_nCS_pin, GPIO_PIN_SET);
			if(hal_result != hal_status_ok){
				return HV_TPXB_FAILED;
			}
		}else{
			log_error("Not implemented!");
		}

#else
	#error "Board is not defined!!"
#endif
	return BOARD_OK;
}

void board_spx3_set_pwr_en1v8( SPACEPIX_ID spxIndex, uint8_t state )
{
#if defined(BOARD_STM32U5)
	if( spxIndex == SPXA ){
		HAL_GPIO_WritePin(SPX3_PWR_EN_port, SPX3_PWR_EN_pin, state ? GPIO_SET : GPIO_RESET);
	} else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_STM32G4)
	log_error("Not implemented!");
#elif defined(BOARD_VA416X0)
	log_error("Not implemented!");
#else
	#error "Board is not defined!!"
#endif
}

void board_spx3_set_chip_select(SPACEPIX_ID spxIndex, uint8_t state)
{
#if defined(BOARD_STM32U5)
	if( spxIndex == SPXA ){
		HAL_GPIO_WritePin(SPX3_nCS_IN_port, SPX3_nCS_IN_pin, state ? GPIO_SET : GPIO_RESET );
	}else {
		log_error("Not implemented!");
	}
#endif
}

void board_spx3_set_inj_en(SPACEPIX_ID spxIndex, uint8_t state)
{
#if defined(BOARD_STM32U5)
	if( spxIndex == SPXA ){
		HAL_GPIO_WritePin(SPX3_INJ_EN_port, SPX3_INJ_EN_pin, state ? GPIO_SET : GPIO_RESET );
	}else {
		log_error("Not implemented!");
	}
#endif
}
#if 0 // only define with version 1
void board_spx3_set_mode_select(SPACEPIX_ID spxIndex, SPACEPIX_READOUT_MODE state)
{
#if defined(BOARD_STM32U5)
	if( spxIndex == SPXA ){
		HAL_GPIO_WritePin(SPX3_MODE_SELECT_port, SPX3_MODE_SELECT_pin, state ? GPIO_SET : GPIO_RESET );
	}else {
		log_error("Not implemented!");
	}
#endif
}
#endif
void board_spx3_set_cnt_en(SPACEPIX_ID spxIndex, uint8_t state)
{
#if defined(BOARD_STM32U5)
	if( spxIndex == SPXA ){
		HAL_GPIO_WritePin(SPX3_CNT_EN_port, SPX3_CNT_EN_pin, state ? GPIO_SET : GPIO_RESET );
	}else {
		log_error("Not implemented!");
	}
#endif
}

void board_spx3_set_clr_n(SPACEPIX_ID spxIndex, uint8_t state)
{
#if defined(BOARD_STM32U5)
	if( spxIndex == SPXA ){
		HAL_GPIO_WritePin(SPX3_CLR_N_port, SPX3_CLR_N_pin, state ? GPIO_SET : GPIO_RESET );
	}else {
		log_error("Not implemented!");
	}
#endif
}

void board_spx3_set_row_shift(SPACEPIX_ID spxIndex, uint8_t state)
{
#if defined(BOARD_STM32U5)
	if( spxIndex == SPXA ){
		HAL_GPIO_WritePin(SPX3_ROW_SHIFT_port, SPX3_ROW_SHIFT_pin, state ? GPIO_SET : GPIO_RESET );
	}else {
		log_error("Not implemented!");
	}
#endif
}

void board_spx3_set_cnf_en(SPACEPIX_ID spxIndex, uint8_t state)
{
#if defined(BOARD_STM32U5)
	if( spxIndex == SPXA ){
		HAL_GPIO_WritePin(SPX3_CNF_EN_port, SPX3_CNF_EN_pin, state ? GPIO_SET : GPIO_RESET );
	}else {
		log_error("Not implemented!");
	}
#endif
}

void board_scan_pwm()
{
	float f_duty = 0.0;
	uint32_t pre = 0;
	uint32_t arr = 0;
	uint32_t freq_Hz = 5000;

	// DEFAULT
	uint32_t ccr2 = 1;

	// todo choose arr
	arr = 5;

	for(uint32_t i = 10; i < 501; i+=10){
		arr = i;
		f_duty = (float)(100*(1/arr));
		pre = 160000000/(freq_Hz*arr);
		TIM15->PSC = pre-1;
		TIM15->ARR = arr-1;
		TIM15->CCR2 = ccr2;
		HAL_TIM_PWM_Start(MCLOCK_NUMBER, MCLOCK_CHANEL);
		Delay(10);
	}


}

void board_spx3_set_en_hv(SPACEPIX_ID spxIndex, uint8_t state)
{
#if defined(BOARD_STM32U5)
	if( spxIndex == SPXA ){
		HAL_GPIO_WritePin(nCS_HV_port, nCS_HV_pin, state ? GPIO_SET : GPIO_RESET );
	}else {
		log_error("Not implemented!");
	}
#endif
}

void board_spx3_set_oe(SPACEPIX_ID spxIndex, uint8_t state)
{
#if defined(BOARD_STM32U5)
	if( spxIndex == SPXA ){
		HAL_GPIO_WritePin(SPX3_OE_port, SPX3_OE_pin, state ? GPIO_SET : GPIO_RESET );
	}else {
		log_error("Not implemented!");
	}
#endif
}

void process_shutter_spx3(uint32_t time){
	bool open_shutter = true;
	int start_shutter_time = HAL_GetTick();
	board_spx3_set_cnt_en(TPXA, 1);		// SHUTTER ON
	while(open_shutter){
		Delay(1);
		int end_shutter_time = HAL_GetTick();
		int diff_shutter_time = end_shutter_time - start_shutter_time;
		if(diff_shutter_time > time){		// time is longer then set acq. time -> close SHUTTER
			open_shutter = false;
			log_debug("Time: %d", diff_shutter_time);
			break;
		}
	}
	board_spx3_set_cnt_en(TPXA, 0);		// SHUTTER OFF
}

void process_shutter_spx3_inj(uint32_t time, uint16_t inject){
	bool open_shutter = true;
	int start_shutter_time = HAL_GetTick();
	//write_ltc2635(ADC_IN, inject);			// ADC_IN max value
	//write_ltc2635(INJ_EN, DAC_MAX_VALUE);
	write_ltc2635(INJ_EN, 0);	// INJ_EN to 0
	board_spx3_set_cnt_en(TPXA, 1);		// SHUTTER ON								// START charge injection phase
	write_ltc2635(INJ_EN, 0);	// INJ_EN to 0
	while(open_shutter){
		Delay(1);
		int end_shutter_time = HAL_GetTick();
		int diff_shutter_time = end_shutter_time - start_shutter_time;
		if(diff_shutter_time > time){		// time is longer then set acq. time -> close SHUTTER
			open_shutter = false;
			log_debug("Time: %d", diff_shutter_time);
			break;
		}
	}

	Delay(5);
	board_spx3_set_cnt_en(TPXA, 0);		// SHUTTER OFF

	Delay(5);
	write_ltc2635(INJ_EN, 0);	// INJ_EN to 0 -> default state
	//write_ltc2635(ADC_IN, 0);			// ADC_IN max value
}

void process_shutter(uint32_t time){
	bool open_shutter = true;
	int start_shutter_time = HAL_GetTick();
	board_tpx2_set_shutter_counter(TPXA, 0);		// SHUTTER ON
	while(open_shutter){
		Delay(1);
		int end_shutter_time = HAL_GetTick();
		int diff_shutter_time = end_shutter_time - start_shutter_time;
		if(diff_shutter_time > time){		// time is longer then set acq. time -> close SHUTTER
			open_shutter = false;
			log_debug("Time: %d", diff_shutter_time);
			break;
		}
	}
	board_tpx2_set_shutter_counter(TPXA, 1);		// SHUTTER OFF
}

uint8_t board_spi_transmit_receive( TIMEPIX_ID tpxIndex, uint8_t *txbuf, uint8_t *rxbuf, uint16_t len )
{
#if defined(BOARD_STM32U5)
	if( tpxIndex == TPXA ){
		if( HAL_SPI_TransmitReceive(SPI_RX, txbuf, rxbuf, len, SPI_DEFAULT_TPX_TIMEOUT) != HAL_OK){
			return BOARD_FAILED;
		}
	}else {
		log_error("Not implemented!");
	}
#endif
	return BOARD_OK;
}

uint8_t board_spi_transmit( TIMEPIX_ID tpxIndex, uint8_t *txbuf, uint16_t len )
{
#if defined(BOARD_STM32U5)
	if( tpxIndex == TPXA ){

#if 0
		if( HAL_SPI_Transmit(SPI_TPX2_TX, txbuf, len, SPI_DEFAULT_TPX_TIMEOUT) != HAL_OK ){
			log_error("SPI TX fail!");
			return BOARD_FAILED;
		}
#endif

#if 1
		// Disable SPI2
		//CLEAR_BIT(SPI2->CR1, SPI_CR1_SPE);


		// Set TSIZE to 0 (defaut value) for unknown size transfert
		MODIFY_REG(SPI2->CR2, SPI_CR2_TSIZE, 0);



		// Enable SPI2
		if ((SPI2->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE) {
				// If disabled, I enable it
				SET_BIT(SPI2->CR1, SPI_CR1_SPE);
		}

		// Master transfert start
		SET_BIT(SPI2->CR1, SPI_CR1_CSTART);
		// Transfert in 8 bit Mode

		while (len > 0)
		{
		  if (SPI_IsActiveFlag_TXP (SPI2))
		   {
			  *(volatile uint8_t *)&SPI2->TXDR = *txbuf;
			  txbuf += sizeof(uint8_t);
			  len--;

		  }

		}

		// Wait last TxFIFO transmission complete
		while (!(SPI_IsActiveFlag_TXC (SPI2)))
		{

		}

		// Disable SPI2
		//CLEAR_BIT(SPI2->CR1, SPI_CR1_SPE);
#endif


#if 1

#endif

#if 0
		//Delay(1);
		while(tx_done != true){}
		tx_done = false;
		uint8_t result = HAL_SPI_Transmit_DMA(SPI_TPX2_TX, txbuf, len);
		if( result != HAL_OK ){
			log_error("SPI TX, result: %d!", result);
			return BOARD_FAILED;
		}
#endif
	}else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_STM32G4)
	if( tpxIndex == TPXA ){
		if( HAL_SPI_Transmit(SPI_TPX2_TOP, txbuf, len, SPI_DEFAULT_TPX_TIMEOUT) != HAL_OK ){
			return BOARD_FAILED;
		}
	}else if(tpxIndex == TPXB){
		if( HAL_SPI_Transmit(SPI_TPX2_BOT, txbuf, len, SPI_DEFAULT_TPX_TIMEOUT) != HAL_OK ){
			return BOARD_FAILED;
		}
	}else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_VA416X0)
	if( tpxIndex == TPXA ){
		if( HAL_Spi_XferB8(VOR_SPI2, txbuf, txbuf, len, false, 0) != hal_status_ok ){
			return BOARD_FAILED;
		}
	}else if(tpxIndex == TPXB){
		if( HAL_SPI_Transmit(SPI_TPX2_BOT, txbuf, len, SPI_DEFAULT_TPX_TIMEOUT) != hal_status_ok ){
			return BOARD_FAILED;
		}
	}else {
		log_error("Not implemented!");
	}
#else
	#error "Board is not defined!!"
#endif
	return BOARD_OK;
}

uint8_t SPI_IsActiveFlag_TXP(SPI_TypeDef *SPIx)
{
  return ((READ_BIT(SPIx->SR, SPI_SR_TXP) == (SPI_SR_TXP)) ? 1 : 0);
}

uint8_t SPI_IsActiveFlag_TXC (SPI_TypeDef *SPIx)
{
  return ((READ_BIT(SPIx->SR, SPI_SR_TXC) == (SPI_SR_TXC)) ? 1 : 0);
}

uint8_t board_spi_receive( TIMEPIX_ID tpxIndex, uint8_t *rxbuf, uint16_t len )
{
#if defined(BOARD_STM32U5)
	if( tpxIndex == TPXA ){
		if(HAL_SPI_Receive_DMA(SPI_RX, (uint8_t *)rxbuf, len)  != HAL_OK ){
			return BOARD_FAILED;
		}
	}else {
		log_error("Not implemented!");
	}
#elif defined(BOARD_STM32G4)
	if( tpxIndex == TPXA ){
		if( HAL_SPI_Receive(SPI_TPX2_TOP, (uint8_t *)rxbuf, len, SPI_DEFAULT_TPX_TIMEOUT) != HAL_OK ){
			return BOARD_FAILED;
		}
	}else {
		log_error("Not implemented!");
	}
#else
	#error "Board is not defined!!"
#endif
	return BOARD_OK;
}

void Delay(uint32_t Delay)		// delay in ms
{
	#if defined(BOARD_STM32U5)
		//HAL_Delay(Delay);
		// Delay = 1 -> Delay 0.1 ms
		// Warning check SYSTEM_CLOCK setting in project
		// Division of these CLOCK should be 1000
		ux_utility_delay_ms(Delay);			// using RTOS ...
	#elif defined(BOARD_STM32G4)
		Delay(Delay);
	#elif defined(BOARD_VA416X0)
		HAL_Timer_DelayMs(Delay);
	#endif
}

uint8_t BoardGetBiasVoltage(float *bias)
{
	uint8_t status = BOARD_OK;
#if defined(TPX_CMD)
	status = board_tpx2_get_HV(TPXA, bias);
#endif
#if defined(SPX_CMD)
	// TODO read out HV
	*bias = 0.0;
#endif
	return status;
}

uint8_t BoardGetTemp(float *temp)
{
	uint8_t status = BOARD_OK;
#if defined(TIMEPIX_CONNECT)
	status = board_tpx2_get_temp(TPXA, temp);
#endif
#if defined(SPACEPIX_CONNECT)
	status = board_spx3_get_temp(SPXA, temp);
#endif
	return status;
}


uint8_t BoardSetBiasVoltage(float voltage)
{
	uint8_t status = BOARD_OK;
#if defined(TPX_CMD)
	status = board_tpx2_set_HV(TPXA, voltage);
#endif
#if defined(SPX_CMD)
	// TODO set HV
	status = BOARD_OK;
#endif
	return status;
}

void board_set_1v2_pwr_en(TIMEPIX_ID tpxIndex, uint8_t state)
{
	HAL_GPIO_WritePin(EN_1V2_port, EN_1V2_pin, state ? GPIO_SET : GPIO_RESET);
}

uint8_t read_ad799x_raw(AD_799x_ADDR address, AD_799x_CH channel, uint16_t *adcdata)
{
	HAL_StatusTypeDef ret;
	uint8_t addata[2];

	// Set CONFIG register, choose on which channel will be conversion
	ret = HAL_I2C_Master_Transmit(I2C_PERIPHERY, (uint16_t)(address<<1), &channel, 1, I2C_DEFAULT_TPX_TIMEOUT);
	if(ret != HAL_OK)
	{
		return ret;
	}
	Delay(5);

	// Read RAW data from AD799x
	ret = HAL_I2C_Master_Receive(I2C_PERIPHERY, (uint16_t)(address<<1)|0x01, addata, 2, I2C_DEFAULT_TPX_TIMEOUT);
	if(ret != HAL_OK)
	{
		return ret;
	}

	// Assemble adc reading data from two bytes
	// first fout bit set on 0, a pripojim k tomu adata[1]
	*adcdata = ((addata[0] & 0x0F) << 8) | addata[1];
	return HAL_OK;
}

void convert_adc_raw(uint16_t adc_raw, float *voltage)
{
	// adcval * (vref/(2^resulution - 1))
	uint8_t resulution = 12;
	float vref = 5.1;
	*voltage = adc_raw * vref / (float)((1<<resulution) -1);
}

uint8_t write_ltc2635(LTC2635_CH channel, uint16_t data)
{
	uint8_t result = BOARD_OK;
	// pins CA2, CA1, CA0 are tie to GND
	uint8_t address = 0b0010000;
	uint8_t cmd_addr = 0;

	// 12 bit version:
	cmd_addr = (W_REG_N_UPDATE_REG_ALL << 4) | channel;
	uint8_t data1 = (data & 0x0FF0) >> 4;
	uint8_t data0 = (data & 0x000F) << 4;
	uint8_t write_pattern[3] = {cmd_addr, data1, data0};

	result = HAL_I2C_Master_Transmit(I2C_PERIPHERY, (uint16_t)(address<<1), write_pattern, 3, I2C_DEFAULT_TPX_TIMEOUT);
	if(result != BOARD_OK)
	{
		return result;
	}

	return result;
}

uint8_t read_tmp112x_raw(TMP112D_ADDR address, uint16_t *adcdata)
{
	uint8_t result = BOARD_OK;
	uint8_t addata[2] = {0};

	uint8_t pointer_register[1] = {0x0};	// 0x0 > read temperature
	//uint8_t pointer_register[1] = {0x1};	// 0x1 > read config
	result = HAL_I2C_Master_Transmit(I2C_PERIPHERY, (uint16_t)(address<<1), pointer_register, 1, I2C_DEFAULT_TPX_TIMEOUT);
	if(result != BOARD_OK)
	{
		return result;
	}

	result = HAL_I2C_Master_Receive(I2C_PERIPHERY, (uint16_t)(address<<1)|0x01, addata, 2, I2C_DEFAULT_TPX_TIMEOUT);
	if(result != HAL_OK)
	{
		return result;
	}

	*adcdata = ((addata[0] & 0x0F) << 4) | (addata[1] >> 4);
	return BOARD_OK;
}

void convert_temp_raw(uint16_t temp_raw, float *temperature)
{
	// adcval * (vref/(2^resulution - 1))
	*temperature = (temp_raw)*0.0625;
}

uint8_t board_spx3_get_temp(SPACEPIX_ID spxId, float *data)
{
	uint8_t result = SPX_OK;

#if 1
	float temp = 0;
	uint16_t adcdata = 0;
	read_tmp112x_raw(TMP112D0, &adcdata);
	convert_temp_raw(adcdata, &temp);
	//log_debug("TEMP: %.2f C", temperature);
	*data = temp;

#endif
	return result;
}

uint8_t board_spx3_get_analog_out(SPACEPIX_ID spxId, float *data)
{
	uint8_t result = SPX_OK;

	uint16_t raw_adc = 0;
	float adc_voltage = 0;
#if 1
	read_ad799x_raw(AD7991_1, CH1, &raw_adc);
	log_debug("AD7991_1 CH1: 0x%X ", raw_adc);
	convert_adc_raw(raw_adc, &adc_voltage);
	log_debug("AD7991_1 CH1: %.2f V", adc_voltage);
	*data = adc_voltage;

#endif
	return result;
}

uint8_t BoardGetSensorTemp(float *temp)
{
	uint8_t status = BOARD_OK;
#if defined(TPX_CMD)
	status = tpx2_get_temp(TPXA, temp);
#endif
#if defined(SPX_CMD)
	//TODO get Sensor temp
	*temp = 0;
#endif
	return status;
}


uint8_t test_AD_DAC(void)
{
#if 1
	#define ADC_SAMPLE 		5
	#define ADC_12BIT		4095

	uint16_t raw_adc = 0;
	float adc_voltage = 0;
	uint16_t var = 0;
	float voltage_arr[ADC_12BIT] = {0};
	for(uint16_t i = 0; i < ADC_12BIT; i++){
		var++;
		write_ltc2635(DACD, var);
		Delay(20);
		// make mean of adc measurements:
		float voltage_adc_iter = 0;
		for(uint8_t iter = 0; iter < ADC_SAMPLE; iter++){
			raw_adc = 0;
			read_ad799x_raw(AD7991_1, CH0, &raw_adc);
			convert_adc_raw(raw_adc, &adc_voltage);
			voltage_adc_iter+= adc_voltage;
		}
		voltage_arr[i] = voltage_adc_iter/ADC_SAMPLE;
	}
	for(uint16_t k = 0; k < ADC_12BIT; k++){
		log_debug("%.3f", voltage_arr[k]);
	}
#endif
#if 1
	read_ad799x_raw(AD7991_1, CH0, &raw_adc);
	log_debug("ADC CH0: 0x%X ", raw_adc);
	convert_adc_raw(raw_adc, &adc_voltage);
	log_debug("ADC CH0: %.2f V", adc_voltage);
#endif

#if 0
	uint16_t adcdata = 0;
	float temperature = 0;
	read_tmp112x_raw(TMP112D0, &adcdata);
	convert_temp_raw(adcdata, &temperature);
	log_debug("TEMP: %.2f C", temperature);

#endif
	return 0;
}

#if defined(BOARD_VA416X0)
static uint8_t CLK_VA_Init(void)
{
	hal_status_t status = hal_status_ok;
	hal_xtalsel_t gCurrentXtalsel;
	// Configure CLKGEN
  gCurrentXtalsel = hal_xtalsel_xtal_n_en;
  status = HAL_Clkgen_Init((hal_clkgen_init_t){.xtalsel = gCurrentXtalsel,           \
                                                      .clksel = hal_clksel_sys_pll,  \
                                                      .pllcfg = hal_pllcfg_enabled,  \
                                                      .clk_div_sel = hal_clk_div_4x, \
                                                      .lost_det_en = true, \
                                                      .pll_out_mhz = 100});
  SystemCoreClockUpdate();
  NVIC_EnableIRQ(LoCLK_IRQn); // enable IRQ on loss of clock

	// clkgen report (most likely fail is PLL failed to lock, if no/bad external clk)
	if(status != hal_status_ok){
		log_debug("CLKGEN status: %s", HAL_StatusToString(status));
	}
	return status;
}

static uint8_t WATCHDOG_VA_Init(void)
{
	hal_status_t status = hal_status_ok;
	 // Watchdog, EDAC
  // *** IMPORTANT FOR FLIGHT / RADIATION ENVIRONMENT ***
#ifdef ENABLE_WATCHDOG
  EnableWatchdog(); // disable if running in debug to avoid reset on bkpt
#endif
  ConfigureEdac();
	// TMR refresh (0, fastest rate)
  VOR_SYSCONFIG->REFRESH_CONFIG_H = 0x0; // no test mode by default (normal mode)
  VOR_SYSCONFIG->REFRESH_CONFIG_L = 0x0; // this has to be set to 0 (fastest) for UART stability

  // Put the boot FRAM into sleep mode
  // *** IMPORTANT FOR FLIGHT / RADIATION ENVIRONMENT ***
  status = FRAM_Init(ROM_SPI_BANK,ROM_SPI_CSN);
  status = FRAM_UnInit(ROM_SPI_BANK); // UnInit sets sleep mode

	if(status != hal_status_ok){
		log_debug("FRAM/WATCHDOG init error, status code: %d", status);
	}
	return status;
}

#ifdef ENABLE_WATCHDOG		// in board.h
static void EnableWatchdog(void)
{
  VOR_SYSCONFIG->PERIPHERAL_CLK_ENABLE |= CLK_ENABLE_WDOG;
  VOR_SYSCONFIG->PERIPHERAL_RESET &= ~SYSCONFIG_PERIPHERAL_RESET_WDOG_Msk;
  __NOP();
  __NOP();
  VOR_SYSCONFIG->PERIPHERAL_RESET |= SYSCONFIG_PERIPHERAL_RESET_WDOG_Msk;
  VOR_WATCH_DOG->WDOGLOAD = (SystemCoreClock/4/1000)*WDOG_MS;
  VOR_WATCH_DOG->WDOGCONTROL = 3; // enable INTEN and RESEN
  VOR_WATCH_DOG->WDOGLOCK = 1; // lock registers
}
#endif

static uint8_t HAL_VA_Init(void)
{
	hal_status_t status = hal_status_ok;
  status = HAL_Init();
  if(status != hal_status_ok){
		log_debug("HAL init error, status code: %d", status);
	}
	return status;
}


static uint8_t SPI2_VA_Init(void)
{
	hal_status_t status = hal_status_ok;
	//init, TODO: dat do define, nebo jeste probrat jak to vubec udelat s kompatibilitou
	uint8_t wordLen = 8;
	uint16_t clkDiv = 10;
	status = HAL_Iocfg_SetupPin(TPX2_TOP_nCS_port, TPX2_TOP_nCS_pin, en_iocfg_dir_output, IOCFG_REG_PULLDN); // nastaveni PA4 as output, melo by vse byt v jedne funkci pri nastavovani GPIO!

	// SPI2 - TPX2_TOP
	status = HAL_Spi_InitMaster(SPI_TPX2_TOP, en_spi_clkmode_0, wordLen, clkDiv);

	if(status != hal_status_ok){
		log_debug("SPI init error, status code: %d", status);
	}
	return status;
}

static uint8_t UART2_VA_Init(void)
{
	hal_status_t status = hal_status_ok;
	// setup UART2 (Note: PF6-9, PMOD UART J37 on Dev. Board)
  status = HAL_Uart_Init(VOR_UART2, UART_CFG_115K_8N1);
	DBG_SetStdioOutput(en_stdio_uart2);

	if(status != hal_status_ok){
    log_debug("UART2 init error, status code: %d", status);
  }
	return status;
}

static uint8_t GPIO_VA_Init(void)
{
	hal_status_t status = hal_status_ok;

	// Configure IO from board.c file // EXCEL parser.. <-> not great at all
  status = HAL_Iocfg_Init(&ioPinCfgArr[0]);

	// TPX2_TOP GPIO pin setup
	status = HAL_Iocfg_SetupPin(TPX2_TOP_EN_1V8_port, TPX2_TOP_EN_1V8_pin, en_iocfg_dir_output, IOCFG_REG_PULLDN);
	status = HAL_Iocfg_SetupPin(TPX2_TOP_GLOBAL_RESET_port, TPX2_TOP_GLOBAL_RESET_pin, en_iocfg_dir_output, IOCFG_REG_PULLDN);

	if(status != hal_status_ok){
    log_debug("GPIO init error, status code: %d", status);
  }
	return status;
}

void GPIO_WritePin(VOR_GPIO_Type* GPIOx,uint16_t GPIO_Pin, GPIO_PinState PinState){
	if(PinState == GPIO_PIN_SET)
  {
		VOR_GPIO->BANK[0].DATAOUT |= (1 << GPIO_Pin);    // set PA[GPIO_Pin]  output to 1
  }
  else
  {
    VOR_GPIO->BANK[0].DATAOUT &= ~(1 << GPIO_Pin);    // set PA[GPIO_Pin]  output to 0
  }
}

static void ConfigureEdac(void)
{
  VOR_SYSCONFIG->RAM0_SCRUB = 500;
  VOR_SYSCONFIG->RAM1_SCRUB = 500;
  VOR_SYSCONFIG->ROM_SCRUB = 500;

  IRQROUTER_ENABLE_CLOCK();
  NVIC_EnableIRQ(EDAC_MBE_IRQn);
  NVIC_SetPriority(EDAC_MBE_IRQn, 0);
  NVIC_EnableIRQ(EDAC_SBE_IRQn);
  NVIC_SetPriority(EDAC_SBE_IRQn, 0);

  VOR_SYSCONFIG->IRQ_ENB = 0x3f; // enable all IRQ
}
#endif

#if defined(STM32G4)
// INIT SPI TO RX mode
void MX_SPI1_Init_RX(void)
{

  if (HAL_SPI_DeInit(&hspi1) != HAL_OK)
  {
	Error_Handler();
  }
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

void MX_SPI1_Init_TX(void)
{

  if (HAL_SPI_DeInit(&hspi1) != HAL_OK)
  {
	Error_Handler();
  }
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

void MX_SPI1_Init_RX_wait(void)
{

  if (HAL_SPI_DeInit(&hspi1) != HAL_OK)
  {
	Error_Handler();
  }
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_7BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
	Error_Handler();
  }

}

void MX_SPI1_Init_TX_wait(void)
{

  if (HAL_SPI_DeInit(&hspi1) != HAL_OK)
  {
	Error_Handler();
  }
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_9BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }


}

void set_get_header_init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (HAL_SPI_DeInit(&hspi1) != HAL_OK)
  {
	Error_Handler();
  }
  // GPIO setting:
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(TPX2_TOP_nCS_port, TPX2_TOP_nCS_pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TPX2_TOP_DATA_IN_port, TPX2_TOP_DATA_IN_pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TPX2_TOP_DCLOCK_IN_port, TPX2_TOP_DCLOCK_IN_pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TPX2_TOP_DATA_OUT_port, TPX2_TOP_DATA_OUT_pin, GPIO_PIN_SET);

}
#endif

