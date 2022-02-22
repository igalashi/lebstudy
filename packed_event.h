/*
 *
 */
#ifndef INC_PACKED_EVENT
#define INC_PACKED_EVENT

struct packed_event {
	uint32_t magic;
	uint32_t length;
	uint16_t id;
	uint16_t nodes;
	uint32_t flag;
};

#endif
