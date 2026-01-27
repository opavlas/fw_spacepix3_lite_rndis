/*
 * debug.h
 *
 */

#ifndef INC_DEBUG_H_
#define INC_DEBUG_H_

#define DEBUG_LOG_GLOBAL
#ifdef DEBUG_LOG_GLOBAL
	#define DEBUG_LOG_Debug
	#define DEBUG_LOG_Warning
	#define DEBUG_LOG_Error
#endif

void log_buffer_flush();
void log_debug( const char *format, ... );
void log_warning( const char *format, ... );
void log_error( const char *format, ... );

#endif /* INC_DEBUG_H_ */
