#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#ifdef DEBUG
	#define DEBUG_MODE 1
#else
	#define DEBUG_MODE 0
#endif

#define PR_DEBUG(fmt, ...) \
	do { \
		if (DEBUG_MODE) { \
			fprintf(stderr, "DEBUG [%s:%d]: " fmt, \
					__func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)

#define SRC_FILE "tg.sample"
// This is only for the demo version of the program,
// in reality i would get a buffer and analyze it byte by byte
#define MES_LEN 11
#define DATA_SIZE 4
#ifndef __BIGGEST_ALIGNMENT__
	#define __BIGGEST_ALIGNMENT__ (sizeof(void *) * 2)
#endif

/* ==========================================================================*/
// bit macross
/* ==========================================================================*/
// control byte macross

#define KNX_CTRL_IS_EXTENDED_FRAME(ctrl)	(!(((ctrl) >> 7) & 1))
#define KNX_CTRL_IS_REPEAT(ctrl)		(!(((ctrl) >> 5) & 1))
// Is regular brcast TP or IP telegram
#define KNX_CTRL_IS_REGULAR_BRCAST(ctrl)	(((ctrl) >> 4) & 1)
#define KNX_CTRL_IS_REQUEST_ACK(ctrl)		(((ctrl) >> 1) & 1)
#define KNX_CTRL_IS_ERROR_FRAME(ctrl)		((ctrl) & 1)
#define KNX_CTRL_GET_PRIOR(ctrl)		(((ctrl) >> 2) & 0x03)

// address parsing macross

// group address
#define GET_MAIN(addr)   (((addr) >> 11) & 0x1f)
#define GET_MID(addr)    (((addr) >> 8)  & 0x07)
#define GET_SUB(addr)    ((addr) & 0xff)

// source address
#define GET_AREA(addr)   (((addr) >> 12) & 0x0f)
#define GET_LINE(addr)   (((addr) >> 8)  & 0x07)
#define GET_DEV(addr)    ((addr) & 0xff)

// Info byte parsing macros
#define GET_INFO_GROUP_BIT(info)	(((info) >> 7) & 1)
#define GET_HOP_COUNT(info)		(((info) >> 4) & 0x07)
// need to + 1 to calculate data size
#define GET_BITS_DATA_LENGTH(info)	((info) & 0xf)

// Hight data byte parsing macros TPCI
#define IS_CONTROL_DATA(h_data_byte)	(((h_data_byte) >> 7) & 1)
#define IS_NUMBT_DATA(h_data_byte)	(((h_data_byte) >> 6) & 1)
// Sequence number bits. If unnumbered transmittion, then 0000
#define GET_SEQ_NUM(h_data_byte)	(((h_data_byte) >> 2) & 0x0f)

// APCI parsing
#define GET_APCI_ACTION(apci)		(((apci) >> 6) & 0x000f)

/* ==========================================================================*/

// Info byte 0xe3 1110 0011 Bits are: 7-th bit 1-group, 0-individual.
// 6-4 Hop Count - TTL in TCP/IP terms. In example is 110
// 3-0 Length. Length is 4 bits + 1. in example 0011 = 3 + 1 = 4
//
// NPDU Network Protocol Data Unit
// 4 bytes: 0x0080 0x1f60 it is data
// TPCI Transport Control Layer Protocol Information
// 2 bit 00 it is TPCI data codes 2 bits: 00 Unnumbered Data, 01 Numbered Data,
// 10 Unnambered Control, 11 Numbered Control.
// 4 bit 0000 secuence number, works similarly to the TCP sequence number.
// in TCP 2 byte
// 10 bit APCI Codes.
// 2 byte is float. if sign is 1, then mantissa is inverted, otherwise,
// if sign +, mantissa as is
typedef struct {
	uint8_t ctrl;
	uint16_t src_addr;
	uint16_t dest_addr;
	uint8_t info; // Length: 4 least significant bits + 1
	// data size is info 4 bits + 1 + 1
	uint8_t data[]; // Last element of array is ctrl byte
} __attribute__((packed)) KNX_TG_t;
// The attribute packed would be necessary in a real program
// But this imposes certain restrictions on the transfer of unaligned structure
// fields to functions; modern processors can handle this,
// but embedded systems don't always

KNX_TG_t *parse_line(char *line, KNX_TG_t **tg_p);
void print_knx_frame(KNX_TG_t *tg);
const char *get_prior_str(uint8_t ctrl);
char *get_s_addr_str(uint16_t s_addr, char *str);
char *get_d_addr_str(uint16_t d_addr, uint8_t info, char *str);
float parse_knx_float(uint16_t knx_float);
const char *get_apci_func_name(uint16_t apci);

int main(void)
{
	FILE *in_fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread = 0;
	KNX_TG_t *tg_p = NULL, *tmp_p = NULL;


	if ((in_fp = fopen(SRC_FILE, "r")) == NULL) {
		perror("fopen");
		fprintf(stderr, "source file: %s cant openned\n", SRC_FILE);
		exit(EXIT_FAILURE); // may be a goto ERR or special function
				    // for release resources
	}

	while ((nread = getline(&line, &len, in_fp)) != -1) {

		printf("frame is: %s\n", line);
		// Parsing line and allocate memory for tg_p
		if ((tmp_p = parse_line(line, &tg_p)) == NULL) {
			free(line); line = NULL;
			continue;
		}
		tg_p = tmp_p;
		PR_DEBUG("parsing is ok\n");

		// Parsing frame in *tg_p
		print_knx_frame(tg_p);

		printf("\n\n\n");
		printf("===============================================\n");
		printf("                  next frame\n");
		printf("===============================================\n");
		printf("\n\n\n");

		free(tg_p); tg_p = NULL;
		free(line); line = NULL;
	}

	if (line) free(line);
	fclose(in_fp);
	exit(EXIT_SUCCESS);
}

KNX_TG_t *parse_line(char *line, KNX_TG_t **tg_p)
{
	uint8_t bytes_tg[MES_LEN], lrc;
	int ret;
	size_t alloc_size, data_size, alignment, pow_of_two, i;
	unsigned int byte;
	KNX_TG_t *tg;

	// This is only for the demo version of the program,
	// in reality i would get a buffer and analyze it byte by byte
	// i mean MES_LEN
	for (i = 0, byte = 0; i < MES_LEN; i++, byte = 0) {
		if (sscanf(line + (i * 2), "%02X", &byte) != 1) {
			fprintf(stderr, "line %s is broken\n", line);
			return NULL;
		}
		bytes_tg[i] = (uint8_t)(byte & 0x000000ff);
	}

	data_size = GET_BITS_DATA_LENGTH(bytes_tg[5]) + 1;
	PR_DEBUG("Size of data is: %zu\n", data_size);

	// This is a demonstration of how memory could be allocated
	// in real-world conditions where the packet length is large.
	alignment = sizeof(void *) * 2;
	for (; alignment < __BIGGEST_ALIGNMENT__; alignment <<= 1);
	PR_DEBUG("Biggest Alignment is: %zu\n", alignment);

	alignment >= (sizeof(KNX_TG_t) + data_size + 1) ?
		(alloc_size = alignment) :
		(alloc_size = (sizeof(KNX_TG_t) + data_size + 1));

	for (pow_of_two = sizeof(void *) * 2; pow_of_two < alloc_size;
			pow_of_two <<= 1);
	alloc_size = pow_of_two;

	ret = posix_memalign((void **)tg_p, alignment, alloc_size);
	if (ret == EINVAL) {
		fprintf(stderr, "alloc_size must be a multiple of alignment\n");
		fprintf(stderr, "alloc_size is: %zu, lignment is: %zu\n",
				alloc_size, alignment);
		return NULL;
	}
	if (ret != 0) {
		fprintf(stderr, "something went wrong\n");
		return NULL;
	}

	tg = *tg_p;

	memset(tg, 0, alloc_size);

	tg->ctrl = bytes_tg[0];
	tg->src_addr = bytes_tg[1];
	tg->src_addr <<= 8;
	tg->src_addr += bytes_tg[2];
	tg->dest_addr = bytes_tg[3];
	tg->dest_addr <<= 8;
	tg->dest_addr += bytes_tg[4];
	tg->info = bytes_tg[5];

	for (i = 0; i < data_size; i++) {
		tg->data[i] = bytes_tg[6 + i];
	}

	tg->data[i] = bytes_tg[6 + i]; // check byte

	// Check for lrc sum
	lrc = 0;
	for (i = 0; i < data_size + 6; i++) {
		lrc ^= bytes_tg[i];
	}
	lrc = ~lrc;
	if (lrc != tg->data[data_size]) {
		fprintf(stderr, "LRC error, current: %02x, received: %02x\n",
				lrc, tg->data[data_size]);
		free(tg); tg = NULL;
		return NULL;
	} else {
		PR_DEBUG("lrc is ok\n");
	}

	PR_DEBUG("ctrl byte is: %02x\n", tg->ctrl);
	PR_DEBUG("tg.src_addr is: %x\n", tg->src_addr);
	PR_DEBUG("tg.dest_addr is: %x\n", tg->dest_addr);
	PR_DEBUG("tg.info is: %x\n", tg->info);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: data is: ");
	for (i = 0; i < data_size; i++) {
		fprintf(stderr, "%02x", tg->data[i]);
	}
	fprintf(stderr, "\n");
#endif

	PR_DEBUG("chk_byte is: %x\n", tg->data[data_size]);

	return tg;
}

void print_knx_frame(KNX_TG_t *tg)
{
	char str[255];
	uint8_t ctrl = tg->ctrl, info = tg->info;
	uint16_t s_addr = tg->src_addr, d_addr = tg->dest_addr, knx_float;
	uint16_t apci, first_two;

	printf("Ctrl byte parsing:\n");
	printf("===================================\n");
	if (KNX_CTRL_IS_EXTENDED_FRAME(ctrl))
		printf("is extended\n");
	else
		printf("is standard frame\n");

	if (KNX_CTRL_IS_REPEAT(ctrl))
		printf("is original tg\n");
	else
		printf("is repeated tg\n");

	if (KNX_CTRL_IS_REGULAR_BRCAST(ctrl))
		printf("is regular broadcast\n");
	else
		printf("is system broadcast\n");

	if (KNX_CTRL_IS_REQUEST_ACK(ctrl))
		printf("request ack\n");
	else
		printf("is not request ack\n");

	if (KNX_CTRL_IS_ERROR_FRAME(ctrl))
		printf("has error in frame\n");
	else
		printf("has not error in frame\n");

	printf("Priority is: %s\n\n", get_prior_str(ctrl));

	printf("Address src and dest:\n");
	printf("===================================\n");
	printf("source addr is: %s\n", get_s_addr_str(s_addr, str));
	printf("destination addr is: %s\n", get_d_addr_str(d_addr, info, str));
	printf("\n");

	printf("Info byte parsing:\n");
	printf("===================================\n");
	printf("is group address 0 or 1: %x\n", GET_INFO_GROUP_BIT(info));
	printf("hop count \"TTL\" is: %x\n", GET_HOP_COUNT(info)); 
	printf("data length is: %x\n", GET_BITS_DATA_LENGTH(info) + 1);
	printf("\n");

	printf("TPCI data parsing:\n");
	printf("===================================\n");
	printf("control data = 1, regular data = 0. is: %x\n",
			IS_CONTROL_DATA(tg->data[0]));
	printf("Unnum data = 0, Numbered data = 1. is: %x\n",
			IS_NUMBT_DATA(tg->data[0]));
	if (IS_NUMBT_DATA(tg->data[0]))
		printf("Secuence number is: %x\n", GET_SEQ_NUM(tg->data[0]));
	printf("\n");

	printf("APCI data parsing\n");
	printf("===================================\n");
	first_two = ((tg->data[0] << 8) | tg->data[1]);
	apci = (first_two & 0x03ff);
	printf("GroupVal Read or Response or Write:\n");
	printf("APCI Command is: %s\n", get_apci_func_name(apci));
	printf("\n");

	printf("KNX Float\n");
	printf("===================================\n");

	knx_float = 0;
	knx_float = tg->data[2];
	knx_float <<= 8;
	knx_float += tg->data[3];

	printf("KNX Float is: %.2f\n", parse_knx_float(knx_float));
}

char *get_d_addr_str(uint16_t d_addr, uint8_t info, char *str)
{
	uint8_t main, middle, sub;
	bool is_group = GET_INFO_GROUP_BIT(info);

	if (is_group) {
		main = ((d_addr & 0xf800) >> 11);
		middle = ((d_addr & 0x0700) >> 8);
		sub = (d_addr & 0x00ff);
		sprintf(str, "%u/%u/%u (Group)", main, middle, sub);
	} else {
		main = ((d_addr & 0xf000) >> 12);
		middle = ((d_addr & 0x0f00) >> 8);
		sub = (d_addr & 0x00ff);
		sprintf(str, "%u.%u.%u (Individual)", main, middle, sub);
	}

	return str;
}

char *get_s_addr_str(uint16_t s_addr, char *str)
{
	uint8_t area, line, bus_device;

	area = ((s_addr & 0xf000) >> 12);
	line = ((s_addr & 0x0f00) >> 8);
	bus_device = (s_addr & 0x00ff);

	sprintf(str, "%u.%u.%u", area, line, bus_device);

	return str;
}

float parse_knx_float(uint16_t knx_float)
{
	bool sign = (knx_float & 0x8000);
	uint8_t exp = ((knx_float >> 11) & 0x000f);
	int16_t M = (knx_float & 0x07ff);

	if (sign) {
		M |= 0xf800; // fill leftmost 5 bits 
	}

	return 0.01f * (float)M * (float)(1 << exp);
}

static const char * const priors_names[] = {
	"System", "Alarm", "High", "Low",
};

const char *get_prior_str(uint8_t ctrl)
{
	return priors_names[KNX_CTRL_GET_PRIOR(ctrl)];
}

const char *get_apci_func_name(uint16_t apci)
{
	uint16_t func_code = GET_APCI_ACTION(apci);
	switch (func_code) {
		case 0x0: return "GroupValueRead"; break;
		case 0x1: return "GroupValueResponse"; break;
		case 0x2: return "GroupValueWrite"; break;
		case 0x3: return "PhysAddrWrite"; break;
		case 0x4: return "PhysAddrRead"; break;
		case 0x5: return "PhysAddrResponse"; break;
		case 0x6: return "AdcRead"; break;
		case 0x7: return "AdcResponse"; break;
		case 0x8: return "MemoryRead"; break;
		case 0x9: return "MemoryResponse"; break;
		case 0xa: return "MemoryWrite"; break;
		case 0xb: return "UserMessage"; break;
		case 0xc: return "DeviceDescriptorRead"; break;
		case 0xd: return "DeviceDescriptorResponse"; break;
		case 0xe: return "Restart"; break;
		case 0xf: return "Escape"; break;
		default: return "Many other function may be. Total 1024";
	}

}
