/*
 * uartRingBufDMA.c
 *
 *  Created on: Aug 12, 2021
 *      Author: controllerstech.com
 */

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "uartRingBufDMA.h"
#include "string.h"


extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern osThreadId_t UARTProcTaskHandle;
extern osMessageQueueId_t uartQueueHandle;

osThreadId gPendingTXThreadID;

#define UART huart1
#define DMA hdma_usart1_rx

/* Define the Size Here */
#define RxBuf_SIZE 64
#define TxBuf_SIZE 64
#define MainBuf_SIZE 256
#define UartQueue_SIZE 4

buffer_circ_t buf;
uint8_t RxBuf[RxBuf_SIZE];

uint8_t MainBuf[MainBuf_SIZE];
uint8_t QueueMemory[UartQueue_SIZE][RxBuf_SIZE];
uint8_t QueueMemoryPtr = 0;

uint8_t TxBuf[TxBuf_SIZE]; 
uint8_t count =0;

osMemoryPoolId_t mpid_UART_MemPool; // memory pool id

/* Initialize the Ring Buffer */
void Ringbuf_Init ()
{
	mpid_UART_MemPool = osMemoryPoolNew(UART_MEMPOOL_OBJECTS, sizeof(UART_MEM_BLOCK_t), NULL);
  	if (mpid_UART_MemPool == NULL) {
		 // MemPool object not created, handle failure
		while(1);    	
  	}
	Ringbuf_Reset ();	
  	HAL_UARTEx_ReceiveToIdle_DMA(&UART, RxBuf, RxBuf_SIZE);
  __HAL_DMA_DISABLE_IT(&DMA, DMA_IT_HT);
}

/* Resets the Ring buffer */
void Ringbuf_Reset ()
{
	memset(RxBuf, '\0', RxBuf_SIZE);
	memset(MainBuf, '\0', MainBuf_SIZE);
	buf.size = MainBuf_SIZE;
	buf.wr_offset = 0;
	buf.rd_offset = 0;
	buf.level = 0;
	buf.data = MainBuf;
}

size_t labscim_buffer_direct_input(void *data, size_t size)
{
	size_t max_writesize;
	size_t write_size;
	if (buf.data == NULL)
	{
		/* check your buffer parameter */
		return 0;
	}
	if (buf.wr_offset >= buf.rd_offset)
	{
		max_writesize = buf.size - buf.wr_offset;
	}
	else
	{
		max_writesize = buf.rd_offset - buf.wr_offset;
	}
	if (buf.level == buf.size)
	{
		max_writesize = 0;
	}
	write_size = LABSCIM_MIN(max_writesize, size);
	memcpy(buf.data + buf.wr_offset, data, write_size);
	buf.wr_offset += write_size;
	buf.level += write_size;
	if (buf.wr_offset == buf.size) // warp
	{
		buf.wr_offset = 0;
		return write_size + labscim_buffer_direct_input(data + write_size, size - write_size);
	}
	return write_size;
}

static size_t labscim_buffer_peek(void *data, size_t size)
{
	if(buf.data == NULL) {
		/* check your buffer parameter */
		return(0);
	}
	size = LABSCIM_MIN(size,labscim_buffer_used(buf));
	if(buf.rd_offset + size < buf.size)
	{
		//no warp
		memcpy(data, buf.data + buf.rd_offset, size);
	}
	else
	{
		//warp
		size_t sz1 = buf.size - buf.rd_offset;
		memcpy(data, buf.data + buf.rd_offset, sz1);
		memcpy(data+sz1,buf.data,size-sz1);
	}
	return size;
}

int32_t labscim_find_char_on_buffer(uint8_t c)
{
	if(buf.data == NULL) {
		/* check your buffer parameter */
		return(-1);
	}
	if(buf.level == 0)
	{
		return -1;
	}
	if (buf.rd_offset < buf.wr_offset)
	{
		//no warp
		for (uint32_t i = buf.rd_offset; i < buf.wr_offset; i++)
		{
			if(buf.data[i]==c)
			{
				return i-buf.rd_offset+1;
			}
		}
	}
	else
	{
		//warp
		for (uint32_t i = buf.rd_offset; i < buf.size; i++)
		{
			if(buf.data[i]==c)
			{
				return i-buf.rd_offset+1;
			}
		}
		for (uint32_t i = 0; i < buf.wr_offset; i++)
		{
			if(buf.data[i]==c)
			{
				return buf.size-buf.rd_offset+i+1;
			}
		}
	}	
	return -1;
}


int32_t labscim_buffer_retrieve(void *data, uint32_t size)
{
	size_t rd = labscim_buffer_peek(data,size);
	buf.level -= rd;
	buf.rd_offset= (buf.rd_offset + rd) % buf.size;
	return rd;
}

size_t labscim_buffer_available()
{
	return buf.size - buf.level;
}

size_t labscim_buffer_used()
{
	return buf.level;    
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	labscim_buffer_direct_input(RxBuf, (size_t)Size);
	/* start the DMA again */
	HAL_UARTEx_ReceiveToIdle_DMA(&UART, (uint8_t *)RxBuf, RxBuf_SIZE);
	__HAL_DMA_DISABLE_IT(&DMA, DMA_IT_HT);
	osThreadFlagsSet(UARTProcTaskHandle, 0x0001U);
}

size_t SendToUART(uint8_t *msg, size_t size)
{
	osSemaphoreAcquire(UARTTXSemaphoreHandle,osWaitForever);		
	uint32_t flagsX;
	gPendingTXThreadID = osThreadGetId();
	HAL_UART_Transmit_DMA(&UART, msg, size);
	flagsX = osThreadFlagsWait(0x0001U, osFlagsWaitAny, osWaitForever); 
	osSemaphoreRelease(UARTTXSemaphoreHandle);		
	return size;
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart)
{
  	//do nothing for now
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	//notify the pending thread that the transfer has completed
  	osThreadFlagsSet(gPendingTXThreadID, 0x0001U); 
}

void UARTProcTaskCode(void const *argument)
{
	uint32_t flagsX;
	int32_t size;
	UART_MEM_BLOCK_t *pMem;
	osStatus_t status;
	for (;;)
	{
		// Wait for someone to notify
		//xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		flagsX = osThreadFlagsWait(0x0001U, osFlagsWaitAny, osWaitForever); /* B */
		size = labscim_find_char_on_buffer('\n');
		if(size > 0)
		{
			pMem = (UART_MEM_BLOCK_t *)osMemoryPoolAlloc(mpid_UART_MemPool, 0U);  // get Mem Block
			if (pMem != NULL)
			{ // Mem Block was available
				taskENTER_CRITICAL();
				labscim_buffer_retrieve(pMem->Buf, size);
				//Radioenge	modem sometimes sends \n\r instead \r\n -> filters this crap
				uint8_t b;
				labscim_buffer_peek(&b,sizeof(uint8_t));
				if(b=='\r')
				{
					labscim_buffer_retrieve(&b, sizeof(uint8_t));
				}
				taskEXIT_CRITICAL();
				osMessageQueuePut(uartQueueHandle, &pMem,0U,0U);
			}
			else
			{
				// ignore and try again later
			}
		}
	}
}
