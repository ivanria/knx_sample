#ifndef __KNX_H
#define __KNX_H


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


#endif // __KNX_H
