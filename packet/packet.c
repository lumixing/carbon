#include <string.h>
#include "packet.h"

void decode_string(char *buf, int *idx, char *string) {
	int len = decode_byte(buf, idx);
	memcpy(string, buf + *idx, len);
	string[len] = '\0';
	*idx += len;
}

float decode_float(char *buf, int *idx) {
	float value;
	memcpy(&value, buf + *idx, sizeof(float));
	*idx += sizeof(float);

	return value;
}

void encode_string(char *buf, int *idx, char *value) {
	int len = strlen(value);
	encode_byte(buf, idx, len);
	memcpy(buf + *idx, value, len);
	*idx += len;
}

void encode_float(char *buf, int *idx, float value) {
	memcpy(buf + *idx, &value, sizeof(float));
	*idx += sizeof(float);
}
