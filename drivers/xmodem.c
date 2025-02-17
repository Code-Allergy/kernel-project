/*
 * Copyright 2001-2010 Georges Menie (www.menie.org)
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* this code needs standard functions memcpy() and memset()
   and input/output functions _inbyte() and _outbyte().

   the prototypes of the input/output functions are:
     int _inbyte(unsigned short timeout); // msec timeout
     void _outbyte(int c);

 */

 // TODO bad
#include "bbb/mmc.h"
#include "bbb/uart.h"
#include <kernel/uart.h>

int _inbyte(unsigned int timeout); // msec timeout
void _outbyte(int c);


// #include "uart_irda_cir.h"
// #include "soc_AM335x.h"
#include "crc16.h"
#include <kernel/string.h>
// #include "uartStdio.h"

#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A

#define DLY_1S 1000
#define MAXRETRANS 25

static int check(int crc, const unsigned char *buf, int sz)
{
	if (crc) {
		unsigned short crc = crc16_ccitt(buf, sz);
		unsigned short tcrc = (buf[sz]<<8)+buf[sz+1];
		if (crc == tcrc)
			return 1;
	}
	else {
		int i;
		unsigned char cks = 0;
		for (i = 0; i < sz; ++i) {
			cks += buf[i];
		}
		if (cks == buf[sz])
		return 1;
	}

	return 0;
}

static void flushinput(void)
{
	while (_inbyte(((DLY_1S)*3)>>1) >= 0)
		;
}

int xmodemReceive(unsigned char *dest, int destsz)
{
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	unsigned char *p;
	int bufsz, crc = 0;
	unsigned char trychar = 'C';
	unsigned char packetno = 1;
    int c;
	int i, len = 0;
	int retry;
	int retrans = MAXRETRANS;

	for(;;)
	{
		for( retry = 1; retry; ++retry)
		{
			if (trychar)
			{
				_outbyte(trychar);
			}
			if ((c = _inbyte((DLY_1S)<<10)) >= 0)
			{
				switch (c)
				{
					case SOH:
						bufsz = 128;
						goto start_recv;
					case STX:
						bufsz = 1024;
						goto start_recv;
					case EOT:
	//					flushinput();
						_outbyte(ACK);
						return len; /* normal end */
					case CAN:
						if ((c = _inbyte(DLY_1S)) == CAN)
						{
							flushinput();
							_outbyte(ACK);
							return -1; /* canceled by remote */
						}
						break;
					default:
						break;
				}
			}
		}
		if (trychar == 'C') { trychar = NAK; continue; }
		flushinput();
		_outbyte(CAN);
		_outbyte(CAN);
		_outbyte(CAN);
		return -2; /* sync error */

	start_recv:
		if (trychar == 'C') crc = 1;
		trychar = 0;
		p = xbuff;
		*p++ = c;
		for (i = 0;  i < (bufsz+(crc?1:0)+3); ++i) {
			if ((c = _inbyte(DLY_1S)) < 0) goto reject;
			*p++ = c;
		}

		if (xbuff[1] == (unsigned char)(~xbuff[2]) &&
			(xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno-1) &&
			check(crc, &xbuff[3], bufsz)) {
			if (xbuff[1] == packetno)	{
				register int count = destsz - len;
				if (count > bufsz) count = bufsz;
				if (count > 0) {
					memcpy (&dest[len], &xbuff[3], count);
					len += count;
				}
				++packetno;
				retrans = MAXRETRANS+1;
			}
			if (--retrans <= 0) {
				flushinput();
				_outbyte(CAN);
				_outbyte(CAN);
				_outbyte(CAN);
				return -3; /* too many retry error */
			}
			_outbyte(ACK);
			continue;
		}
	reject:
		flushinput();
		_outbyte(NAK);
	}
}

#if 0
#ifdef TEST_XMODEM_SEND
int xmodemTransmit(unsigned char *src, int srcsz)
{
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	int bufsz, crc = -1;
	unsigned char packetno = 1;
	int i, c, len = 0;
	int retry;

	for(;;) {
		for( retry = 0; retry < 16; ++retry) {
			if ((c = _inbyte((DLY_1S)<<1)) >= 0) {
				switch (c) {
				case 'C':
					crc = 1;
					goto start_trans;
				case NAK:
					crc = 0;
					goto start_trans;
				case CAN:
					if ((c = _inbyte(DLY_1S)) == CAN) {
						_outbyte(ACK);
						flushinput();
						return -1; /* canceled by remote */
					}
					break;
				default:
					break;
				}
			}
		}
		_outbyte(CAN);
		_outbyte(CAN);
		_outbyte(CAN);
		flushinput();
		return -2; /* no sync */

		for(;;) {
		start_trans:
			xbuff[0] = SOH; bufsz = 128;
			xbuff[1] = packetno;
			xbuff[2] = ~packetno;
			c = srcsz - len;
			if (c > bufsz) c = bufsz;
			if (c >= 0) {
				memset (&xbuff[3], 0, bufsz);
				if (c == 0) {
					xbuff[3] = CTRLZ;
				}
				else {
					memcpy (&xbuff[3], &src[len], c);
					if (c < bufsz) xbuff[3+c] = CTRLZ;
				}
				if (crc) {
					unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
					xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
					xbuff[bufsz+4] = ccrc & 0xFF;
				}
				else {
					unsigned char ccks = 0;
					for (i = 3; i < bufsz+3; ++i) {
						ccks += xbuff[i];
					}
					xbuff[bufsz+3] = ccks;
				}
				for (retry = 0; retry < MAXRETRANS; ++retry) {
					for (i = 0; i < bufsz+4+(crc?1:0); ++i) {
						_outbyte(xbuff[i]);
					}
					if ((c = _inbyte(DLY_1S)) >= 0 ) {
						switch (c) {
						case ACK:
							++packetno;
							len += bufsz;
							goto start_trans;
						case CAN:
							if ((c = _inbyte(DLY_1S)) == CAN) {
								_outbyte(ACK);
								flushinput();
								return -1; /* canceled by remote */
							}
							break;
						case NAK:
						default:
							break;
						}
					}
				}
				_outbyte(CAN);
				_outbyte(CAN);
				_outbyte(CAN);
				flushinput();
				return -4; /* xmit error */
			}
			else {
				for (retry = 0; retry < 10; ++retry) {
					_outbyte(EOT);
					if ((c = _inbyte((DLY_1S)<<1)) == ACK) break;
				}
				flushinput();
				return (c == ACK)?len:-5;
			}
		}
	}
}
#endif
#endif

/* LSR */

/* LSR - UART Register */
#define UART_LSR_RX_FIFO_STS   (0x00000080u)
#define UART_LSR_RX_FIFO_STS_SHIFT   (0x00000007u)
#define UART_LSR_RX_FIFO_STS_ERROR   (0x1u)
#define UART_LSR_RX_FIFO_STS_NORMAL   (0x0u)

#define UART_LSR_TX_SR_E   (0x00000040u)
#define UART_LSR_TX_SR_E_SHIFT   (0x00000006u)
#define UART_LSR_TX_SR_E_EMPTY   (0x1u)
#define UART_LSR_TX_SR_E_NOTEMPTY   (0x0u)

#define UART_LSR_TX_FIFO_E   (0x00000020u)
#define UART_LSR_TX_FIFO_E_SHIFT   (0x00000005u)
#define UART_LSR_TX_FIFO_E_EMPTY   (0x1u)
#define UART_LSR_TX_FIFO_E_NOTEMPTY   (0x0u)

#define UART_LSR_RX_BI   (0x00000010u)
#define UART_LSR_RX_BI_SHIFT   (0x00000004u)
#define UART_LSR_RX_BI_BREAK   (0x1u)
#define UART_LSR_RX_BI_NONE   (0x0u)

#define UART_LSR_RX_FE   (0x00000008u)
#define UART_LSR_RX_FE_SHIFT   (0x00000003u)
#define UART_LSR_RX_FE_ERROR   (0x1u)
#define UART_LSR_RX_FE_NONE   (0x0u)

#define UART_LSR_RX_PE   (0x00000004u)
#define UART_LSR_RX_PE_SHIFT   (0x00000002u)
#define UART_LSR_RX_PE_ERROR   (0x1u)
#define UART_LSR_RX_PE_NONE   (0x0u)

#define UART_LSR_RX_OE   (0x00000002u)
#define UART_LSR_RX_OE_SHIFT   (0x00000001u)
#define UART_LSR_RX_OE_ERROR   (0x1u)
#define UART_LSR_RX_OE_NONE   (0x0u)

#define UART_LSR_RX_FIFO_E   (0x00000001u)
#define UART_LSR_RX_FIFO_E_SHIFT   (0x00000000u)
#define UART_LSR_RX_FIFO_E_EMPTY   (0x0u)
#define UART_LSR_RX_FIFO_E_NOTEMPTY   (0x1u)

/* LSR - IrDA Register */
#define UART_LSR_THR_EMPTY   (0x00000080u)
#define UART_LSR_THR_EMPTY_SHIFT   (0x00000007u)
#define UART_LSR_THR_EMPTY_EMPTY   (0x1u)
#define UART_LSR_THR_EMPTY_NOTEMPTY   (0x0u)

#define UART_LSR_STS_FIFO_FULL   (0x00000040u)
#define UART_LSR_STS_FIFO_FULL_SHIFT   (0x00000006u)
#define UART_LSR_STS_FIFO_FULL_FULL   (0x1u)
#define UART_LSR_STS_FIFO_FULL_NOTFULL   (0x0u)

#define UART_LSR_RX_LAST_BYTE   (0x00000020u)
#define UART_LSR_RX_LAST_BYTE_SHIFT   (0x00000005u)
#define UART_LSR_RX_LAST_BYTE_NO   (0x0u)
#define UART_LSR_RX_LAST_BYTE_YES   (0x1u)

#define UART_LSR_FRAME_TOO_LONG   (0x00000010u)
#define UART_LSR_FRAME_TOO_LONG_SHIFT   (0x00000004u)
#define UART_LSR_FRAME_TOO_LONG_ERROR   (0x1u)
#define UART_LSR_FRAME_TOO_LONG_NONE   (0x0u)

#define UART_LSR_ABORT   (0x00000008u)
#define UART_LSR_ABORT_SHIFT   (0x00000003u)
#define UART_LSR_ABORT_ERROR   (0x1u)
#define UART_LSR_ABORT_NONE   (0x0u)

#define UART_LSR_CRC   (0x00000004u)
#define UART_LSR_CRC_SHIFT   (0x00000002u)
#define UART_LSR_CRC_ERROR   (0x1u)
#define UART_LSR_CRC_NONE   (0x0u)

#define UART_LSR_STS_FIFO_E   (0x00000002u)
#define UART_LSR_STS_FIFO_E_SHIFT   (0x00000001u)
#define UART_LSR_STS_FIFO_E_EMPTY   (0x1u)
#define UART_LSR_STS_FIFO_E_NOTEMPTY   (0x0u)

#define UART_LSR_IRDA_RX_FIFO_E   (0x00000001u)
#define UART_LSR_IRDA_RX_FIFO_E_SHIFT   (0x00000000u)
#define UART_LSR_IRDA_RX_FIFO_E_EMPTY   (0x1u)
#define UART_LSR_IRDA_RX_FIFO_E_NOTEMPTY   (0x0u)

/* LSR - CIR Register */
#define UART_LSR_CIR_THR_EMPTY   (0x00000080u)
#define UART_LSR_CIR_THR_EMPTY_SHIFT   (0x00000007u)
#define UART_LSR_CIR_THR_EMPTY_EMPTY   (0x1u)
#define UART_LSR_CIR_THR_EMPTY_NOTEMPTY   (0x0u)

#define UART_LSR_CIR_RX_STOP   (0x00000020u)
#define UART_LSR_RX_STOP_SHIFT   (0x00000005u)
#define UART_LSR_RX_STOP_COMPLETE   (0x1u)
#define UART_LSR_RX_STOP_ONGOING   (0x0u)

#define UART_LSR_CIR_RX_FIFO_E   (0x00000001u)
#define UART_LSR_CIR_RX_FIFO_E_SHIFT   (0x00000000u)
#define UART_LSR_CIR_RX_FIFO_E_EMPTY   (0x1u)
#define UART_LSR_CIR_RX_FIFO_E_NOTEMPTY   (0x0u)

/* XON2_ADDR2 */
#define UART_XON2_ADDR2_XON2_WORD2   (0x000000FFu)
#define UART_XON2_ADDR2_XON2_WORD2_SHIFT   (0x00000000u)

/* Values to be used while switching between register configuration modes. */

#define UART_REG_CONFIG_MODE_A              (0x0080)
#define UART_REG_CONFIG_MODE_B              (0x00BF)
#define UART_REG_OPERATIONAL_MODE           (0x007F)

unsigned int UARTRegConfigModeEnable(unsigned int baseAdd, unsigned int modeFlag)
{
    unsigned int lcrRegValue = 0;

    /* Preserving the current value of LCR. */
    lcrRegValue = REG32(baseAdd + UART_LCR_OFF);

    switch(modeFlag)
    {
        case UART_REG_CONFIG_MODE_A:
        case UART_REG_CONFIG_MODE_B:
            REG32(baseAdd + UART_LCR_OFF) = (modeFlag & 0xFF);
        break;

        case UART_REG_OPERATIONAL_MODE:
            REG32(baseAdd + UART_LCR_OFF) &= 0x7F;
        break;

        default:
        break;
    }

    return lcrRegValue;
}

int UARTCharGetTimeout(unsigned int baseAdd, unsigned int timeOutVal)
{
    unsigned int lcrRegValue = 0;
    int retVal = -1;

    /* Switching to Register Operational Mode of operation. */
    lcrRegValue = UARTRegConfigModeEnable(baseAdd, UART_REG_OPERATIONAL_MODE);

    /*
    ** Wait until either data is not received or the time out value is greater
    ** than zero.
    */
    while((0 == (REG32(baseAdd + UART_LSR_IRDA_OFF) & UART_LSR_RX_FIFO_E)) && timeOutVal)
    {
        timeOutVal--;
    }

    if(timeOutVal)
    {
        retVal = (REG32(baseAdd + UART_RHR_OFF)) & 0x0FF;
    }

    /* Restoring the value of LCR. */
    REG32(baseAdd + UART_LCR_OFF) = lcrRegValue;

    return retVal;
}

int _inbyte(unsigned int timeout) // msec timeout
{

    return(UARTCharGetTimeout(UART0_BASE, timeout));

}
void _outbyte(int c)
{
    uart_driver.putc(c);
}


#if 0
#ifdef TEST_XMODEM_RECEIVE
int main(void)
{
	int st;

	printf ("Send data using the xmodem protocol from your terminal emulator now...\n");
	/* the following should be changed for your environment:
	   0x30000 is the download address,
	   65536 is the maximum size to be written at this address
	 */
	st = xmodemReceive((char *)0x30000, 65536);
	if (st < 0) {
		printf ("Xmodem receive error: status: %d\n", st);
	}
	else  {
		printf ("Xmodem successfully received %d bytes\n", st);
	}

	return 0;
}
#endif
#ifdef TEST_XMODEM_SEND
int main(void)
{
	int st;

	printf ("Prepare your terminal emulator to receive data now...\n");
	/* the following should be changed for your environment:
	   0x30000 is the download address,
	   12000 is the maximum size to be send from this address
	 */
	st = xmodemTransmit((char *)0x30000, 12000);
	if (st < 0) {
		printf ("Xmodem transmit error: status: %d\n", st);
	}
	else  {
		printf ("Xmodem successfully transmitted %d bytes\n", st);
	}

	return 0;
}
#endif
#endif
