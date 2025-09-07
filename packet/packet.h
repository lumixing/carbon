typedef enum {
	// ServerBound (client->server)
	PID_SB_JOIN            = 0x00,
	PID_SB_LEAVE           = 0x01,
	PID_SB_CHAT            = 0x02,
	PID_SB_PLAYER_POSITION = 0x04,
	PID_SB_PLAYER_ROTATION = 0x05,
	PID_SB_BLOCK_UPDATE    = 0x10,

	// ClientBound (server->client)
	PID_CB_JOIN            = 0x80,
	PID_CB_LEAVE           = 0x81,
	PID_CB_CHAT            = 0x82,
	PID_CB_PLAYER_POSITION = 0x84,
	PID_CB_PLAYER_ROTATION = 0x85,
	PID_CB_BLOCK_UPDATE    = 0x90,
	PID_CB_CHUNK           = 0x91,
} PacketID;

#define PKSTRSIZE 256 + 1

#define decode_byte(buf, idx) (buf[(*idx)++])
void decode_string(char *buf, int *idx, char *string);
float decode_float(char *buf, int *idx);

#define encode_byte(buf, idx, byte) (buf[(*idx)++] = byte)
void encode_string(char *buf, int *idx, char *value);
void encode_float(char *buf, int *idx, float value);
