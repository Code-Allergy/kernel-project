#ifndef SD_H
#define SD_H

/* Basic Commands */
#define CMD0    0   /* GO_IDLE_STATE: Reset the card to idle state; no response expected */
#define CMD2    2   /* ALL_SEND_CID: Send card identification (CID) */
#define CMD3    3   /* SEND_RELATIVE_ADDR: Ask the card to publish a new relative address (RCA) */
#define CMD7    7   /* SELECT/DESELECT_CARD: Select or deselect the card */
#define CMD8    8   /* SEND_IF_COND: Send interface condition (voltage check) */
#define CMD9    9   /* SEND_CSD: Send card-specific data (CSD) */
#define CMD10   10  /* SEND_CID: Send card identification (CID) */
#define CMD12   12  /* STOP_TRANSMISSION: Stop a multiple block data transfer */
#define CMD13   13  /* SEND_STATUS: Get the card status */
#define CMD16   16  /* SET_BLOCKLEN: Set block length for subsequent block data transfers */
#define CMD17   17  /* READ_SINGLE_BLOCK: Read a single block of data */
#define CMD18   18  /* READ_MULTIPLE_BLOCK: Read multiple blocks of data */
#define CMD24   24  /* WRITE_BLOCK: Write a single block of data */
#define CMD25   25  /* WRITE_MULTIPLE_BLOCK: Write multiple blocks of data */
#define CMD55   55  /* APP_CMD: Signals that the next command is application-specific */
#define CMD58   58  /* READ_OCR: Read the OCR register */

/* Application-specific Commands (must be preceded by CMD55) */
#define ACMD41  41  /* SD_SEND_OP_COND: Initiate initialization process, get operating conditions */
#define ACMD6   6   /* SET_BUS_WIDTH: Set the data bus width */

#endif /* SD_H */
