/*
 * uartRingBufDMA.h
 *
 *  Created on: Aug 12, 2021
 *      Author: controllerstech.com
 */

#ifndef INC_UARTRINGBUFDMA_H_
#define INC_UARTRINGBUFDMA_H_

#include "cmsis_os.h"

#define LABSCIM_MIN(x,y) ((x)>(y)?(y):(x))
#define LABSCIM_MAX(x,y) ((x)>(y)?(x):(y))

typedef struct
{
    uint32_t wr_offset;
    uint32_t rd_offset;
    uint32_t level;
    size_t size;
    uint8_t* data;
} buffer_circ_t;

#define UART_MEMPOOL_OBJECTS 8                      // number of Memory Pool Objects 
typedef struct {                                // object data type
  uint8_t Buf[64];  
} UART_MEM_BLOCK_t; 

/* Initialize the Ring buffer
 * It will also initialize the UART RECEIVE DMA
 * */
void Ringbuf_Init (void);

/* Reset the ring buffer
 * Resets the Head and Tail, also the buffers
 * */
void Ringbuf_Reset (void);

size_t labscim_buffer_direct_input(void *data, size_t size);
static size_t labscim_buffer_peek(void *data, size_t size);
int32_t labscim_find_char_on_buffer(uint8_t c);
int32_t labscim_buffer_retrieve(void *data, uint32_t size);
size_t labscim_buffer_available();
size_t labscim_buffer_used();
size_t SendToUART(uint8_t *msg, size_t size);


extern osMemoryPoolId_t mpid_UART_MemPool;
extern osSemaphoreId_t UARTTXSemaphoreHandle;

#endif /* INC_UARTRINGBUFDMA_H_ */
