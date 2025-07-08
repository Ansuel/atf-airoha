/*! \file xmodem.c \brief XModem Transmit/Receive Implementation with CRC and 1K support. */
//*****************************************************************************
//
// File Name	: 'xmodem.c'
// Title		: XModem Transmit/Receive Implementation with CRC and 1K support
// Author		: Pascal Stang - Copyright (C) 2006
// Created		: 4/22/2006
// Revised		: 7/22/2006
// Version		: 0.1
// Target MCU	: AVR processors
// Editor Tabs	: 4
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************


#include <xmodem.h>
#include <drivers/console.h>
#include <drivers/delay_timer.h>

console_t *ecnt_console = NULL;

static void WriteByte(unsigned char c) 
{
	if (ecnt_console != NULL)
		ecnt_console->putc(c, ecnt_console);
}

static int ReadByte(void) 
{
	if (ecnt_console != NULL)
		return ecnt_console->getc(ecnt_console);
	else
		return ERROR_NO_VALID_CONSOLE;
}

static unsigned char *xmdmBufBase = NULL;
static unsigned int xmdmBufLen = 0;

int WriteBuffer(unsigned char *buffer, int size)
{
	int i;

	for (i = 0; i < size; i++)
		*xmdmBufBase++ = *buffer++;

	return 0;
}

#define XMODEM_BUFFER_SIZE		1024

// pointers to stream I/O functions
static void (*xmodemOut)(unsigned char c);
static int (*xmodemIn)(void);

static unsigned short crc_xmodem_update(unsigned short crc, unsigned char data)
{
	int i;

	crc = crc ^ ((unsigned short)data << 8);
	for (i=0; i<8; i++)
	{
		if(crc & 0x8000)
			crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
	}

	return crc;
}

static int xmodemCrcCheck(int crcflag, const unsigned char *buffer, int size)
{
	// crcflag=0 - do regular checksum
	// crcflag=1 - do CRC checksum

	if(crcflag)
	{
		unsigned short crc=0;
		unsigned short pktcrc = (buffer[size]<<8)+buffer[size+1];
		// do CRC checksum
		while(size--)
			crc = crc_xmodem_update(crc, *buffer++);
		// check checksum against packet
		if(crc == pktcrc)
			return 1;
	}
	else
	{
		int i;
		unsigned char cksum = 0;
		// do regular checksum
		for(i=0; i<size; ++i)
		{
			cksum += buffer[i];
		}
		// check checksum against packet
		if(cksum == buffer[size])
			return 1;
	}

	return 0;
}

static int xmodemInTime(unsigned short timeout_10ms)
{
	unsigned long timeout = timeout_10ms * 1000 * 10;
	int ret = xmodemIn();

	while (timeout && (ret < 0))
	{
		udelay(1);
		ret = xmodemIn();
		timeout--;
	}
	
	return ret;
}

static void xmodemInFlush(void)
{
	while(xmodemInTime(XMODEM_TIMEOUT_DELAY) >= 0);
}

static long xmodemReceive( int (*write)(unsigned char* buffer, int size) )
{
	// create xmodem buffer
	// 1024b for Xmodem 1K
	// 128 bytes for Xmodem std.
	// + 5b header/crc + NULL
	unsigned char xmbuf[XMODEM_BUFFER_SIZE+6];
	unsigned char seqnum=1;		// xmodem sequence number starts at 1
	unsigned short pktsize=128;	// default packet size is 128 bytes
	unsigned char response='C';	// solicit a connection with CRC enabled
	char retry=XMODEM_RETRY_LIMIT;
	unsigned char crcflag=0;
	unsigned long totalbytes=0;
	int i = 0, c = 0, err =XMODEM_ERROR_RETRYEXCEED;

	while(retry > 0)
	{
		// solicit a connection/packet
		xmodemOut(response);
		// wait for start of packet
		if( (c = xmodemInTime(XMODEM_TIMEOUT_DELAY)) >= 0)
		{
			switch(c)
			{
			case SOH:
				pktsize = 128;
				break;
			#if(XMODEM_BUFFER_SIZE>=1024)
			case STX:
				pktsize = 1024;
				break;
			#endif
			case EOT:
				xmodemInFlush();
				xmodemOut(ACK);
				// completed transmission normally
				return totalbytes;
			case CAN:
				if((c = xmodemInTime(XMODEM_TIMEOUT_DELAY)) == CAN)
				{
					xmodemInFlush();
					xmodemOut(ACK);
					// transaction cancelled by remote node
					return XMODEM_ERROR_REMOTECANCEL;
				}
			default:
				break;
			}
		}
		else
		{
			// timed out, try again
			// no need to flush because receive buffer is already empty
			retry--;
			//response = NAK;
			continue;
		}

		// check if CRC mode was accepted
		if(response == 'C') crcflag = 1;
		// got SOH/STX, add it to processing buffer
		xmbuf[0] = c;
		// try to get rest of packet
		for(i=0; i<(pktsize+crcflag+4-1); i++)
		{
			if((c = xmodemInTime(XMODEM_TIMEOUT_DELAY)) >= 0)
			{
				xmbuf[1+i] = (unsigned char) (c & 0xff);
			}
			else
			{
				// timed out, try again
				retry--;
				xmodemInFlush();
				response = NAK;
				break;
			}
		}
		// packet was too small, retry
		if(i<(pktsize+crcflag+4-1))
			continue;

		// got whole packet
		// check validity of packet
		if(	(xmbuf[1] == (unsigned char)(~xmbuf[2])) &&		// sequence number was transmitted w/o error
			xmodemCrcCheck(crcflag, &xmbuf[3], pktsize) )	// packet is not corrupt
		{
			// is this the packet we were waiting for?
			if(xmbuf[1] == seqnum)
			{
				if ((totalbytes + pktsize) > xmdmBufLen)
				{
					err =  XMODEM_ERROR_OUTOFMEMORY;
					goto out;
				}

				// write/deliver data
				write(&xmbuf[3], pktsize);
				//spiflashWrite(flashaddr, pktsize, &xmbuf[3]);
				totalbytes += pktsize;
				// next sequence number
				seqnum++;
				// reset retries
				retry = XMODEM_RETRY_LIMIT;
				// reply with ACK
				response = ACK;

				continue;
			}
			else if(xmbuf[1] == (unsigned char)(seqnum-1))
			{
				// this is a retransmission of the last packet
				// ACK and move on
				response = ACK;
				continue;
			}
			else
			{
				// we are completely out of sync
				// cancel transmission
				err =  XMODEM_ERROR_OUTOFSYNC;
				goto out;
			}
		}
		else
		{
			// packet was corrupt
			// NAK it and try again
			retry--;
			xmodemInFlush();
			response = NAK;
			continue;
		}
	}

out:
	// exceeded retry count
	xmodemInFlush();
	xmodemOut(CAN);
	xmodemOut(CAN);
	xmodemOut(CAN);
	return err;
}


static void xmodemInit(void (*sendbyte_func)(unsigned char c), int (*getbyte_func)(void))
{
	// assign function pointers
	xmodemOut = sendbyte_func;
	xmodemIn = getbyte_func;
}

int XModemReceive(console_t *console, unsigned int bufLen , unsigned char *bufBase)
{
	ecnt_console = console;
	xmodemInit(WriteByte, ReadByte);
	xmdmBufBase = bufBase;
	xmdmBufLen = bufLen;

	return xmodemReceive(WriteBuffer);
}

