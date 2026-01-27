/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.c
  * @author  MCD Application Team
  * @brief   NetXDuo applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "pinout.h"
#include "debug.h"
#include "board.h"
#include "tpx2_comm.h"
#include "meas.h"
#include "spx3_comm.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


// IP ADDRESS of TPX2 Lite without DHCP: 192.168.1.123
#define RNDIS_IP_ADDRESS        IP_ADDRESS(192,168,1,123)
#define RNDIS_NETMASK_ADDR      IP_ADDRESS(255,255,255,0)
#define RNDIS_GATEWAY_ADDR      IP_ADDRESS(192,168,1,3)/* IP address range assigned by the DHCP server */

#define DHCP
#ifdef DHCP
#define START_IP_ADDRESS_LIST  IP_ADDRESS(192, 168, 1, 10)
#define END_IP_ADDRESS_LIST    IP_ADDRESS(192, 168, 1, 100)
/* Network Configuration */
#define NX_DHCP_SERVER_IP_ADDRESS  IP_ADDRESS(192,168,1,2)
#define NX_DHCP_ROUTER_IP_ADDRESS  IP_ADDRESS(192,168,1,2)
#define NX_DHCP_DNS_IP_ADDRESS     IP_ADDRESS(192,168,1,2)
#endif

//#define MASK_ALL_PIXELS
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
NX_UDP_SOCKET 		UDPSocket;
TX_THREAD          	Init_thread;
TX_THREAD 			AppUDPThread;
TX_THREAD 			AcqThread;
NX_IP              	rndis_ip;
NX_PACKET_POOL     	net_packet_pool;
NX_DHCP_SERVER     	dhcp_server;
NX_PACKET_POOL     WebServerPool;

UCHAR data_buffer[512];
NX_PACKET *data_packet;


/* Set nx_server_pool start address to ".UsbxAppSection" */
#if defined ( __ICCARM__ ) /* IAR Compiler */
#pragma location = ".UsbxAppSection"
#elif defined ( __CC_ARM ) || defined(__ARMCC_VERSION) /* ARM Compiler 5/6 */
__attribute__((section(".UsbxAppSection")))
#elif defined ( __GNUC__ ) /* GNU Compiler */
__attribute__((section(".UsbxAppSection")))
#endif

extern READOUT_STATUS readOutStatus;
extern READOUT_CONFIG readOutConfig;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static VOID App_UDP_Thread_Entry(ULONG thread_input);
static VOID Acq_Thread_Entry(ULONG thread_input);
static VOID Init_thread_entry(ULONG thread_input);

/* USER CODE END PFP */

/**
  * @brief  Application NetXDuo Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_NetXDuo_Init(VOID *memory_ptr)
{
  UINT ret = NX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

   /* USER CODE BEGIN App_NetXDuo_MEM_POOL */
  (void)byte_pool;
  /* USER CODE END App_NetXDuo_MEM_POOL */
  /* USER CODE BEGIN 0 */
  CHAR *pointer;
  ULONG addresses_added;
  /* USER CODE END 0 */

  /* USER CODE BEGIN MX_NetXDuo_Init */
  /* Initialize the NetX system */

  /* Setup the working pointer.  */
    nx_system_initialize();

#if 1
  /* Allocate stack for Main thread */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, 8192, TX_NO_WAIT) != TX_SUCCESS)
  {
    Error_Handler();
  }
  /* Create the Main thread */
  if (tx_thread_create(&Init_thread, "MAIN", Init_thread_entry, 0, pointer, 8192,
                       10, 10, TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
  {
    Error_Handler();
  }
#endif

#if 1
  /* Allocate stack for the packet pool */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    Error_Handler();
  }
  /* Create a packet pool.  */
  if (nx_packet_pool_create(&net_packet_pool, "NetX Main Packet Pool",
                            PACKET_PAYLOAD_SIZE, pointer, NX_PACKET_POOL_SIZE) != NX_SUCCESS)
  {
    Error_Handler();
  }
#endif

#if 1
  /* Allocate stack for the RNDIS IP instance */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, 8192, TX_NO_WAIT) != TX_SUCCESS)
  {
    Error_Handler();
  }
  /* Creates an RNDIS Internet Protocol instance */
  if (nx_ip_create(&rndis_ip, "RNDIS IP Instance", RNDIS_IP_ADDRESS, RNDIS_NETMASK_ADDR,
                   &net_packet_pool, _ux_network_driver_entry, pointer,
				   8192, 3) != NX_SUCCESS)
  {
    Error_Handler();
  }
#endif

#if 1
  /* Allocate stack for ARP */
	if (tx_byte_allocate(byte_pool, (VOID **) &pointer, 8192, TX_NO_WAIT) != NX_SUCCESS)
	{
	Error_Handler();
	}
	/* Enable ARP and supply ARP cache memory for IP Instance RNDIS */
	if (nx_arp_enable(&rndis_ip, pointer, 8192) != NX_SUCCESS)
	{
	Error_Handler();
	}
#endif

  /* Enable UDP traffic */
  if (nx_udp_enable(&rndis_ip) != NX_SUCCESS)
  {
    Error_Handler();
  }

  /* Enable ICMP to enable the ping utility */
  if (nx_icmp_enable(&rndis_ip) != NX_SUCCESS)
  {
    Error_Handler();
  }
#if 1
  /* Enable fragment */
  if (nx_ip_fragment_enable(&rndis_ip) != NX_SUCCESS)
  {
    Error_Handler();
  }
#endif
  /* Setup the Gateway address for previously created IP Instance RNDIS */
  if (nx_ip_gateway_address_set(&rndis_ip, RNDIS_GATEWAY_ADDR) != NX_SUCCESS)
  {
    Error_Handler();
  }
  /* Set the IP address and network mask to RNDIS_IP_ADDRESS and RNDIS_NETMASK_ADDR
     for the previously created IP Instance RNDIS. */
  nx_ip_address_set(&rndis_ip, RNDIS_IP_ADDRESS, RNDIS_NETMASK_ADDR);

#ifdef DHCP
	/* Allocate stack for the DHCP */
	if (tx_byte_allocate(byte_pool, (VOID **) &pointer, 2048, TX_NO_WAIT) != TX_SUCCESS)
	{
	  Error_Handler();
	}

	/* Create the DHCP Server instance */
	if (nx_dhcp_server_create(&dhcp_server, &rndis_ip, pointer, 2048,
							  "RNDIS DHCP Server", &net_packet_pool) != NX_SUCCESS)
	{
	  Error_Handler();
	}

	/* Create a IP address pool */
	if (nx_dhcp_create_server_ip_address_list(&dhcp_server, 0, START_IP_ADDRESS_LIST,
											  END_IP_ADDRESS_LIST,
											  &addresses_added) != NX_SUCCESS)
	{
	  Error_Handler();
	}

	/* Set network parameters for DHCP options */
	if (nx_dhcp_set_interface_network_parameters(&dhcp_server, 0,
												 NX_DHCP_SUBNET_MASK,
												 NX_DHCP_SERVER_IP_ADDRESS,
												 NX_DHCP_DNS_IP_ADDRESS) != NX_SUCCESS)
	{
	  Error_Handler();
	}
#endif

#if 1
  /* Allocate the app UDP thread entry pool. */
    ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_UDP_INSTANCE_THREAD_SIZE, TX_NO_WAIT);
    if (ret != TX_SUCCESS)
    {
      log_error("Allocate the UDP thread");
      return NX_NOT_ENABLED;
    }
    /* create the UDP server thread */
    // start this thread after init_thread <-> so now : TX_DONT_START
    ret = tx_thread_create(&AppUDPThread, "App UDP Thread", App_UDP_Thread_Entry, 0, pointer, NX_UDP_INSTANCE_THREAD_SIZE,
                          NX_UDP_INSTANCE_PRIORITY, NX_UDP_INSTANCE_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);

    if (ret != TX_SUCCESS)
    {
      log_error("Create the UDP thread");
      return NX_NOT_ENABLED;
    }
#endif

#if 1
	/* Allocate the ACQ thread entry pool. */
	ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_ACQ_INSTANCE_THREAD_SIZE, TX_NO_WAIT);

	if (ret != TX_SUCCESS)
	{
	  log_error("Allocate the ACQ thread");
	  return NX_NOT_ENABLED;
	}
	/* create the ACQ server thread */
	// start this thread after acq_start <-> so now : TX_DONT_START
	ret = tx_thread_create(&AcqThread, "Acq Thread", Acq_Thread_Entry, 0, pointer, NX_ACQ_INSTANCE_THREAD_SIZE,
			NX_ACQ_THREAD_PRIORITY, NX_ACQ_INSTANCE_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);

	if (ret != TX_SUCCESS)
	{
	  log_error("Create the ACQ thread");
	  return NX_NOT_ENABLED;
	}
#endif

  /* USER CODE END MX_NetXDuo_Init */

  return ret;
}

/* USER CODE BEGIN 1 */
/**
  * @brief  nx_init_thread_entry
  *         Application thread for HTTP web server
  * @param  thread_input: not used
  * @retval none
  */
VOID Init_thread_entry(ULONG thread_input)
{
	// there should be verification if everyting is fine, after run UDP thread
#if 1
	UINT ret;

#ifdef DHCP
	/* Start DHCP Server processing */
	if (nx_dhcp_server_start(&dhcp_server) != NX_SUCCESS)
	{
	Error_Handler();
	}
#endif

	/* create the UDP socket */
	ret = nx_udp_socket_create(&rndis_ip, &UDPSocket, "UDP Server Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, QUEUE_MAX_SIZE);

	if (ret != NX_SUCCESS)
	{
	 Error_Handler();
	}

	/* bind the socket indefinitely on the required port */
	ret = nx_udp_socket_bind(&UDPSocket, DEFAULT_PORT, TX_WAIT_FOREVER);

	if (ret != NX_SUCCESS)
	{
	 Error_Handler();
	}

	ret = UDP_packet_pool();
	if (ret != NX_SUCCESS)
	{
	   Error_Handler();
	}

	// RUN MAIN thread with UDP
	tx_thread_resume(&AppUDPThread);
}

static VOID App_UDP_Thread_Entry(ULONG thread_input)
{
	UINT ret;
	bool first_init = true;
	log_debug("UDP Thread start");
	while(1)
	{
		if(first_init){	// INIT TPX2, first CMD get from TrackLab
			first_init = false;
#if defined(TIMEPIX_CONNECT)
			if(tpx2_init_dig() == TPX_OK){
				log_debug("... UDP Thread waiting on UDP packet ...");
			} else {
				// Program never should be there
				log_error("First INIT of TPX2");
				while(1);	// TODO What should be done in this situation
			}
#endif
#if defined(SPACEPIX_CONNECT)
			if(spx3_init_dig() == SPX_OK){
				log_debug("... SPX3 init done ...");
			} else {
				// Program never should be there
				log_error("First INIT of TPX2");
				while(1);	// TODO What should be done in this situation
			}
#endif
		}
#if 1
		// UDP PACKET RECIEVE
		log_debug("... UDP Thread waiting on UDP packet ...");
		ret = nx_udp_socket_receive(&UDPSocket, &data_packet, TX_WAIT_FOREVER);
		if (ret == NX_SUCCESS)	// UDP recieve success, proces the CMD
		{
			ReadoutMainCmdHandler(&UDPSocket, data_packet);
		} else {
			log_error("Recieve UDP packet");
		}
#endif
#if 1
		//-> should be done inside of CmdHandler
		if(readOutStatus.measurementInProgress == true){	// From UDP thread was set ACQ. START
			tx_thread_resume(&AcqThread);					// Resume ACQ. thread.
		}
#endif
	}

#endif
}


// By Default this Thread is not running -> Resume only if ACQ. Start
static VOID Acq_Thread_Entry(ULONG thread_input)
{
	log_debug("ACQ Thread start");
	UINT ret = 0;
	//bool open_shutter = true;
	const uint32_t counterA_length = GetCounterBitSize(GET_COUNTER_A) / 8;
	const uint32_t counterB_length = GetCounterBitSize(GET_COUNTER_B) / 8;
	const uint32_t counterC_length = GetCounterBitSize(GET_COUNTER_C) / 8;
	const uint32_t counterD_length = GetCounterBitSize(GET_COUNTER_D) / 8;

	uint8_t *counterA_raw = (uint8_t *)malloc(counterA_length);
	if(counterA_raw == NULL){
		log_error("Malloc of counter A");
		Error_Handler();
	}

	uint8_t *counterB_raw = (uint8_t *)malloc(counterB_length);
	if(counterB_raw == NULL){
		log_error("Malloc of counter B");
		Error_Handler();
	}

	uint8_t *counterC_raw = (uint8_t *)malloc(counterC_length);
	if(counterC_raw == NULL){
		log_error("Malloc of counter C");
		Error_Handler();
	}

	uint8_t *counterD_raw = (uint8_t *)malloc(counterD_length);
	if(counterD_raw == NULL ){
		log_error("Malloc of counter D");
		Error_Handler();
	}

	// COUNTER A
	uint16_t counterA_decode_get[MATRIX_SIZE] = {0};

	// COUNTER B
	uint16_t counterB_decode_get[MATRIX_SIZE] = {0};

	// COUNTER C
	uint8_t counterC_decode_get[ SPX3_PX_MATRIX_BLOCK_SIZE*(SPX3_ROW_SIZE*2+1)] = {0};

	// COUNTER D
	uint8_t counterD_decode_get[MATRIX_SIZE] = {0};
#if 0 // for test purpouse
	memset(counterA_raw, 0x0000, counterA_length);
	memset(counterB_raw, 0x0000, counterB_length);
	memset(counterC_raw, 0x00, counterC_length);
	memset(counterD_raw, 0x00, counterD_length);

#endif

	while(1){
		if(readOutStatus.measurementInProgress == true){
			// NOT conti_mode and NOT col_trigger
			if(!readOutConfig.conti_mode && !readOutConfig.col_trigger_en){
				// Process the SHUTTER:
				// Shutter is inside data_readout from spx3
				//process_shutter(readOutConfig.acqTime);
			}

#if 0
			// NOT conti_mode and col_trigger
			if(!readOutConfig.conti_mode && readOutConfig.col_trigger_en){
				readOutStatus.start_col_trig_time = HAL_GetTick();
				board_tpx2_set_shutter_counter(TPXA, 0);
				// make somehow with Thread properties,
				while(tpx2_get_matrix_occ() != 0){
					// TODO think about this delay
					Delay(1);									// in this time process UDP CMD in second thread, not ideal, but works...
					readOutStatus.end_col_trig_time = HAL_GetTick();
					readOutStatus.diff_col_trig_time = readOutStatus.end_col_trig_time - readOutStatus.start_col_trig_time;
					log_debug("Diff time: %d", readOutStatus.diff_col_trig_time);
					if(readOutStatus.diff_col_trig_time > readOutConfig.acqTime){		// time is longer then set acq. time -> close SHUTTER
						log_debug("Column trigger time run out");
						break;
					}
				}
				board_tpx2_set_shutter_counter(TPXA, 1);
			}
#endif

			// TODO Think about when continous mode is turn on

			// Read out the data from TPX2
			MeasurementProcess(&UDPSocket, readOutConfig.sender_ip, readOutConfig.data_mode, readOutConfig.col_trigger_en, counterA_raw, counterB_raw, counterC_raw, counterD_raw,
								counterA_decode_get, counterB_decode_get, counterC_decode_get, counterD_decode_get);
		}

		// measurmetnInProgress == Flase
		else{
			// mask pixel matrix -> should save some power and device shold get cooler
#ifdef MASK_ALL_PIXELS
			uint8_t *conf = (uint8_t *)malloc(SIZE_OF_COUNTER_CD_BYTES);
			if(conf == NULL){
				log_error("Malloc of conf");
				Error_Handler();
			}
			memset(conf, 0xFF, SIZE_OF_COUNTER_CD_BYTES);
			transform_set_counterCD_new(readOutConfig.tpx2Cfg.chipConfig, conf);
			tpx2_setconf(conf);	// SET CONF
			SAFE_FREE(conf);
#endif
#if defined (TPX_CMD)
			read_out_counters(counterA_raw, counterB_raw, counterC_raw, counterD_raw);	// it means that for new acq. counter will be empty
#endif
			tx_thread_suspend(&AcqThread);
		}
	}

}



/* USER CODE END 1 */
