#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <string_ext.h>
#include <stdio.h>
#include <trace.h>
#include "vedliot_utils.h"

#define UINT8_DIGIT_MAX_SIZE 	2

void utils_print_byte_array(uint8_t *byte_array, int byte_array_len)
{
	// +byte_array_len for spaces
	int buffer_len = UINT8_DIGIT_MAX_SIZE * byte_array_len + byte_array_len;
	char *buffer = malloc(buffer_len);

	int i, buffer_cursor = 0;
	for (i = 0; i < byte_array_len; ++i)
	{
		buffer_cursor += snprintf(buffer + buffer_cursor, buffer_len - buffer_cursor, "%02x ", byte_array[i]);
	}

	// Replace trhe last space by the string termination char
	buffer[buffer_cursor] = '\0';

	DMSG("[%s]", buffer);

	free(buffer);
}
