
#ifndef _CRC_H
#define _CRC_H

void CRC_Init(unsigned short *crcvalue);
void CRC_ProcessByte(unsigned short *crcvalue, unsigned char data);
unsigned short CRC_Value(unsigned short crcvalue);
unsigned short CRC_Block(unsigned char *start, int count);

#endif /* !_CRC_H */
