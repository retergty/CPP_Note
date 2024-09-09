#include <uxr/client/transport.h>
#include <uxr/client/util/time.h>
#include "main.h"
#include "uxr_transport.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#if 1

// --- micro-ROS Transports ---
#define UART_IT_BUFFER_SIZE 2048

static uint8_t it_buffer[UART_IT_BUFFER_SIZE];
static uint8_t it_data;
static size_t it_head = 0, it_tail = 0;

extern UART_HandleTypeDef huart1;

bool my_custom_transport_open(struct uxrCustomTransport * transport){
    HAL_UART_Receive_IT(&huart1, &it_data, 1);
    return true;
}

bool my_custom_transport_close(struct uxrCustomTransport * transport){
    HAL_UART_Abort_IT(&huart1);
    return true;
}

size_t my_custom_transport_write(struct uxrCustomTransport* transport,const uint8_t * buf, size_t len, uint8_t * err){
    HAL_StatusTypeDef ret;
    if (huart1.gState == HAL_UART_STATE_READY){
        ret = HAL_UART_Transmit(&huart1, buf, len, HAL_MAX_DELAY);
        while (ret == HAL_OK && huart1.gState != HAL_UART_STATE_READY);

        return (ret == HAL_OK) ? len : 0;
    }else{
        return 0;
    }
}

size_t my_custom_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err){
    size_t wrote = 0;
    uint32_t start_time = uxr_millis();

    while (wrote < len)
    {
    	if(uxr_millis() - start_time > timeout)
    	{
    		break;
    	}
    	while((it_head != it_tail) && (wrote < len))
    	{
    		buf[wrote] = it_buffer[it_head];
        	it_head = (it_head + 1) % UART_IT_BUFFER_SIZE;
        	wrote++;
    	}
    }

    return wrote;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(it_tail == UART_IT_BUFFER_SIZE)
        it_tail = 0;

    it_buffer[it_tail] = it_data;
    it_tail++;

    HAL_UART_Receive_IT(huart, &it_data, 1);
}

#endif
