/*
 * udp_cmd_handler.h
 *
 *  Created on: Dec 12, 2024
 *      Author: opavl
 */

#ifndef APP_UDP_CMD_HANDLER_H_
#define APP_UDP_CMD_HANDLER_H_

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

//#define TPX_CMD
#define SPX_CMD

void ReadoutMainCmdHandler( NX_UDP_SOCKET *udp_socket, NX_PACKET *data_packet );
void CommandInternal( HW_CMD_ID cmdId );

void Convert32bitTo4xByteArray( uint32_t in, uint8_t *out4);
uint32_t Convert4xByteArrayTo32bit( uint8_t *in4 );



#endif /* APP_UDP_CMD_HANDLER_H_ */
