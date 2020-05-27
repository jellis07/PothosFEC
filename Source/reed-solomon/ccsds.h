#pragma once

extern unsigned char Taltab[],Tal1tab[];

int decode_rs_ccsds(unsigned char *data,int *eras_pos,int no_eras);
void encode_rs_ccsds(unsigned char *data,unsigned char *parity);
