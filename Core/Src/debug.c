#include <debug.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>



#define LINE_BUFFER_SIZE	120
static char line_buffer[LINE_BUFFER_SIZE + 3];	// + reserve for "\r\n" and 0x00

void log_buffer_flush()
{
#ifdef DEBUG_LOG_GLOBAL
	//_write( 0, line_buffer, strlen(line_buffer) );
	printf( line_buffer );
#endif
	line_buffer[0] = 0;
}

void log_debug( const char *format, ... )
{
#ifdef DEBUG_LOG_GLOBAL
#ifdef DEBUG_LOG_Debug
	va_list vargs;
	va_start(vargs, format);
	strncpy( line_buffer, "Debug: ", LINE_BUFFER_SIZE - strlen(line_buffer) );
	vsnprintf( line_buffer + strlen(line_buffer), LINE_BUFFER_SIZE - strlen(line_buffer), format, vargs );
	strcpy( line_buffer + strlen(line_buffer), "\r\n" );
	va_end(vargs);
	log_buffer_flush();
#endif
#endif
}

void log_warning( const char *format, ... )
{
#ifdef DEBUG_LOG_GLOBAL
#ifdef DEBUG_LOG_Warning
	va_list vargs;
	va_start(vargs, format);
#ifdef CONSOLE_FORMATED_OUTPUT
	strncpy( line_buffer, "\e[33mWarning:\e[39m ", LINE_BUFFER_SIZE - strlen(line_buffer) );
#else
	strncpy( line_buffer, "Warning: ", LINE_BUFFER_SIZE - strlen(line_buffer) );
#endif
	vsnprintf( line_buffer + strlen(line_buffer), LINE_BUFFER_SIZE - strlen(line_buffer), format, vargs );
	strcpy( line_buffer + strlen(line_buffer), "\r\n" );
	va_end(vargs);
	log_buffer_flush();
#endif
#endif
}

void log_error( const char *format, ... )
{
#ifdef DEBUG_LOG_GLOBAL
#ifdef DEBUG_LOG_Error
	va_list vargs;
	va_start(vargs, format);
#ifdef CONSOLE_FORMATED_OUTPUT
	strncpy( line_buffer, "\e[31mError:\e[39m ", LINE_BUFFER_SIZE - strlen(line_buffer) );
#else
	strncpy( line_buffer, "Error: ", LINE_BUFFER_SIZE - strlen(line_buffer) );
#endif
	vsnprintf( line_buffer + strlen(line_buffer), LINE_BUFFER_SIZE - strlen(line_buffer), format, vargs );
	strcpy( line_buffer + strlen(line_buffer), "\r\n" );
	va_end(vargs);
	log_buffer_flush();
#endif
#endif
}
