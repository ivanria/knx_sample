#ifndef FUNC_DEFS_H
#define FUNC_DEFS_H

#include <stdint.h>
#include "knx.h"

KNX_TG_t *parse_line(char *line, KNX_TG_t **tg_p);
void print_knx_frame(KNX_TG_t *tg);
const char *get_prior_str(uint8_t ctrl);
char *get_s_addr_str(uint16_t s_addr, char *str);
char *get_d_addr_str(uint16_t d_addr, uint8_t info, char *str);
float parse_knx_float(uint16_t knx_float);
const char *get_apci_func_name(uint16_t apci);


#endif // FUNC_DEFS_H
