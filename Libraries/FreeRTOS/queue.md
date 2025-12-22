# FreeRTOS Queue

FreeRTOS 提供了队列（Queue）作为任务间通信和数据交换的机制。队列允许任务以线程安全的方式发送和接收数据。

队列还是计数信号量，mutex等实现的基础。

FreeRTOS队列是值语义的，数据项被复制到队列中，而不是传递指针。

FreeRTOS队列可以在任务之间传递数据，也可以在中断服务例程（ISR）中使用。但是，在ISR中使用队列时，需要使用专门的API函数，如`xQueueSendFromISR`和`xQueueReceiveFromISR`。

FreeRTOS队列支持阻塞操作，任务可以选择在发送或接收数据时等待，直到队列有空间或有数据可用。

## 创建队列

```CPP
QueueHandle_t xQueueCreate(
    UBaseType_t uxQueueLength,   // 队列中可以容纳的最大项数
    UBaseType_t uxItemSize       // 每个项的大小（以字节为单位）
);
QueueHandle_t xQueueCreateStatic(
    UBaseType_t uxQueueLength,   // 队列中可以容纳的最大项数
    UBaseType_t uxItemSize,      // 每个项的大小（以字节为单位）
    uint8_t *pucQueueStorage,    // 指向用于存储队列数据的缓冲区
    StaticQueue_t *pxQueueBuffer // 指向用于存储队列结构的缓冲区
);
```

创建队列，一个是静态创建，一个是动态创建。

### 实现

```CPP
typedef struct QueueDefinition /* The old naming convention is used to prevent breaking kernel aware debuggers. */
{
    int8_t * pcHead;           /**< Points to the beginning of the queue storage area. */
    int8_t * pcWriteTo;        /**< Points to the free next place in the storage area. */

    union
    {
        QueuePointers_t xQueue;     /**< Data required exclusively when this structure is used as a queue. */
        SemaphoreData_t xSemaphore; /**< Data required exclusively when this structure is used as a semaphore. */
    } u;

    List_t xTasksWaitingToSend;             /**< List of tasks that are blocked waiting to post onto this queue.  Stored in priority order. */
    List_t xTasksWaitingToReceive;          /**< List of tasks that are blocked waiting to read from this queue.  Stored in priority order. */

    volatile UBaseType_t uxMessagesWaiting; /**< The number of items currently in the queue. */
    UBaseType_t uxLength;                   /**< The length of the queue defined as the number of items it will hold, not the number of bytes. */
    UBaseType_t uxItemSize;                 /**< The size of each items that the queue will hold. */

    volatile int8_t cRxLock;                /**< Stores the number of items received from the queue (removed from the queue) while the queue was locked.  Set to queueUNLOCKED when the queue is not locked. */
    volatile int8_t cTxLock;                /**< Stores the number of items transmitted to the queue (added to the queue) while the queue was locked.  Set to queueUNLOCKED when the queue is not locked. */

    #if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
        uint8_t ucStaticallyAllocated; /**< Set to pdTRUE if the memory used by the queue was statically allocated to ensure no attempt is made to free the memory. */
    #endif

    #if ( configUSE_QUEUE_SETS == 1 )
        struct QueueDefinition * pxQueueSetContainer;
    #endif

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxQueueNumber;
        uint8_t ucQueueType;
    #endif
} xQUEUE;
```

* `pcHead`：指向队列存储区域的起始位置。
* `pcWriteTo`：指向存储区域中下一个可用的位置。

* `xTasksWaitingToSend`：等待发送数据到队列的任务列表。
* `xTasksWaitingToReceive`：等待从队列接收数据的任务列表

* `uxMessagesWaiting`：当前队列中的项数。
* `uxLength`：队列的长度（最大项数）。
* `uxItemSize`：每个项的大小（以字节为单位）。

* `cRxLock` 和 `cTxLock`：用于锁定队列以防止并发访问。

```CPP
QueueHandle_t xQueueGenericCreate( const UBaseType_t uxQueueLength,
                                    const UBaseType_t uxItemSize,
                                    const uint8_t ucQueueType )
{
    Queue_t * pxNewQueue = NULL;
    size_t xQueueSizeInBytes;
    uint8_t * pucQueueStorage;

    traceENTER_xQueueGenericCreate( uxQueueLength, uxItemSize, ucQueueType );

    if( ( uxQueueLength > ( UBaseType_t ) 0 ) &&
        /* Check for multiplication overflow. */
        ( ( SIZE_MAX / uxQueueLength ) >= uxItemSize ) &&
        /* Check for addition overflow. */
        ( ( UBaseType_t ) ( SIZE_MAX - sizeof( Queue_t ) ) >= ( uxQueueLength * uxItemSize ) ) )
    {
        /* Allocate enough space to hold the maximum number of items that
            * can be in the queue at any time.  It is valid for uxItemSize to be
            * zero in the case the queue is used as a semaphore. */
        xQueueSizeInBytes = ( size_t ) ( ( size_t ) uxQueueLength * ( size_t ) uxItemSize );

        /* MISRA Ref 11.5.1 [Malloc memory assignment] */
        /* More details at: https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
        /* coverity[misra_c_2012_rule_11_5_violation] */
        pxNewQueue = ( Queue_t * ) pvPortMalloc( sizeof( Queue_t ) + xQueueSizeInBytes );

        if( pxNewQueue != NULL )
        {
            /* Jump past the queue structure to find the location of the queue
                * storage area. */
            pucQueueStorage = ( uint8_t * ) pxNewQueue;
            pucQueueStorage += sizeof( Queue_t );

            #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
            {
                /* Queues can be created either statically or dynamically, so
                    * note this task was created dynamically in case it is later
                    * deleted. */
                pxNewQueue->ucStaticallyAllocated = pdFALSE;
            }
            #endif /* configSUPPORT_STATIC_ALLOCATION */

            prvInitialiseNewQueue( uxQueueLength, uxItemSize, pucQueueStorage, ucQueueType, pxNewQueue );
        }
        else
        {
            traceQUEUE_CREATE_FAILED( ucQueueType );
            mtCOVERAGE_TEST_MARKER();
        }
    }
    else
    {
        configASSERT( pxNewQueue );
        mtCOVERAGE_TEST_MARKER();
    }

    traceRETURN_xQueueGenericCreate( pxNewQueue );

    return pxNewQueue;
}
```

通用的队列创建函数，队列的内存通过`pvPortMalloc`动态分配。

不只是分配了存储队列数据的内存，还分配了`Queue_t`结构体本身的内存，这样就可以把队列的元数据和数据存储在一起，方便管理。

## 接收

```CPP
BaseType_t xQueueReceive(
    QueueHandle_t xQueue,
    void *pvBuffer,
    TickType_t xTicksToWait
);
```

从队列接收（读取）一个项。

* `xQueue`：要从中接收数据的队列句柄。
* `pvBuffer`：指向用于存储接收数据的缓冲区的指针。
* `xTicksToWait`：如果队列为空，等待数据可用的最大时间（以系统节拍为单位）。

返回值：如果成功接收数据，返回`pdTRUE`；如果在指定时间内未能接收数据，返回`errQUEUE_EMPTY`。

```CPP
BaseType_t xQueueReceiveFromISR( QueueHandle_t xQueue,
                                 void * const pvBuffer,
                                 BaseType_t * const pxHigherPriorityTaskWoken )
```

中断版本，允许在中断服务例程（ISR）中接收数据。

此外，还有一个非阻塞版本`xQueuePeek`，用于查看队列中的下一个项而不将其从队列中移除。

```CPP
BaseType_t xQueuePeek(
    QueueHandle_t xQueue,
    void *pvBuffer,
    TickType_t xTicksToWait
);
BaseType_t xQueuePeekFromISR(
    QueueHandle_t xQueue,
    void *pvBuffer
);
```

### 实现

```CPP
BaseType_t xQueueReceive( QueueHandle_t xQueue,
                          void * const pvBuffer,
                          TickType_t xTicksToWait )
{
    BaseType_t xEntryTimeSet = pdFALSE;
    TimeOut_t xTimeOut;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueReceive( xQueue, pvBuffer, xTicksToWait );

    /* Check the pointer is not NULL. */
    configASSERT( ( pxQueue ) );

    /* The buffer into which data is received can only be NULL if the data size
     * is zero (so no data is copied into the buffer). */
    configASSERT( !( ( ( pvBuffer ) == NULL ) && ( ( pxQueue )->uxItemSize != ( UBaseType_t ) 0U ) ) );

    /* Cannot block if the scheduler is suspended. */
    #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
    {
        configASSERT( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0 ) ) );
    }
    #endif

    for( ; ; )
    {
        taskENTER_CRITICAL();
        {
            const UBaseType_t uxMessagesWaiting = pxQueue->uxMessagesWaiting;

            /* Is there data in the queue now?  To be running the calling task
             * must be the highest priority task wanting to access the queue. */
            if( uxMessagesWaiting > ( UBaseType_t ) 0 )
            {
                /* Data available, remove one item. */
                prvCopyDataFromQueue( pxQueue, pvBuffer );
                traceQUEUE_RECEIVE( pxQueue );
                pxQueue->uxMessagesWaiting = ( UBaseType_t ) ( uxMessagesWaiting - ( UBaseType_t ) 1 );

                /* There is now space in the queue, were any tasks waiting to
                 * post to the queue?  If so, unblock the highest priority waiting
                 * task. */
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
                {
                    if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
                    {
                        queueYIELD_IF_USING_PREEMPTION();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                taskEXIT_CRITICAL();

                traceRETURN_xQueueReceive( pdPASS );

                return pdPASS;
            }
            else
            {
                if( xTicksToWait == ( TickType_t ) 0 )
                {
                    /* The queue was empty and no block time is specified (or
                     * the block time has expired) so leave now. */
                    taskEXIT_CRITICAL();

                    traceQUEUE_RECEIVE_FAILED( pxQueue );
                    traceRETURN_xQueueReceive( errQUEUE_EMPTY );

                    return errQUEUE_EMPTY;
                }
                else if( xEntryTimeSet == pdFALSE )
                {
                    /* The queue was empty and a block time was specified so
                     * configure the timeout structure. */
                    vTaskInternalSetTimeOutState( &xTimeOut );
                    xEntryTimeSet = pdTRUE;
                }
                else
                {
                    /* Entry time was already set. */
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
        taskEXIT_CRITICAL();

        /* Interrupts and other tasks can send to and receive from the queue
         * now the critical section has been exited. */

        vTaskSuspendAll();
        prvLockQueue( pxQueue );

        /* Update the timeout state to see if it has expired yet. */
        if( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) == pdFALSE )
        {
            /* The timeout has not expired.  If the queue is still empty place
             * the task on the list of tasks waiting to receive from the queue. */
            if( prvIsQueueEmpty( pxQueue ) != pdFALSE )
            {
                traceBLOCKING_ON_QUEUE_RECEIVE( pxQueue );
                vTaskPlaceOnEventList( &( pxQueue->xTasksWaitingToReceive ), xTicksToWait );
                prvUnlockQueue( pxQueue );

                if( xTaskResumeAll() == pdFALSE )
                {
                    taskYIELD_WITHIN_API();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                /* The queue contains data again.  Loop back to try and read the
                 * data. */
                prvUnlockQueue( pxQueue );
                ( void ) xTaskResumeAll();
            }
        }
        else
        {
            /* Timed out.  If there is no data in the queue exit, otherwise loop
             * back and attempt to read the data. */
            prvUnlockQueue( pxQueue );
            ( void ) xTaskResumeAll();

            if( prvIsQueueEmpty( pxQueue ) != pdFALSE )
            {
                traceQUEUE_RECEIVE_FAILED( pxQueue );
                traceRETURN_xQueueReceive( errQUEUE_EMPTY );

                return errQUEUE_EMPTY;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    }
}
/*-----------------------------------------------------------*/
```

超时等待，如果队列为空且指定了等待时间，任务将被阻塞，直到有数据可用或超时。

接收数据，并更新队列状态，如果有任务等待发送数据，唤醒最高优先级的任务。

```CPP
BaseType_t xQueueReceiveFromISR( QueueHandle_t xQueue,
                                 void * const pvBuffer,
                                 BaseType_t * const pxHigherPriorityTaskWoken )
{
    BaseType_t xReturn;
    UBaseType_t uxSavedInterruptStatus;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueReceiveFromISR( xQueue, pvBuffer, pxHigherPriorityTaskWoken );

    configASSERT( pxQueue );
    configASSERT( !( ( pvBuffer == NULL ) && ( pxQueue->uxItemSize != ( UBaseType_t ) 0U ) ) );

    /* RTOS ports that support interrupt nesting have the concept of a maximum
     * system call (or maximum API call) interrupt priority.  Interrupts that are
     * above the maximum system call priority are kept permanently enabled, even
     * when the RTOS kernel is in a critical section, but cannot make any calls to
     * FreeRTOS API functions.  If configASSERT() is defined in FreeRTOSConfig.h
     * then portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
     * failure if a FreeRTOS API function is called from an interrupt that has been
     * assigned a priority above the configured maximum system call priority.
     * Only FreeRTOS functions that end in FromISR can be called from interrupts
     * that have been assigned a priority at or (logically) below the maximum
     * system call interrupt priority.  FreeRTOS maintains a separate interrupt
     * safe API to ensure interrupt entry is as fast and as simple as possible.
     * More information (albeit Cortex-M specific) is provided on the following
     * link: https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html */
    portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    /* MISRA Ref 4.7.1 [Return value shall be checked] */
    /* More details at: https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
    /* coverity[misra_c_2012_directive_4_7_violation] */
    uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
    {
        const UBaseType_t uxMessagesWaiting = pxQueue->uxMessagesWaiting;

        /* Cannot block in an ISR, so check there is data available. */
        if( uxMessagesWaiting > ( UBaseType_t ) 0 )
        {
            const int8_t cRxLock = pxQueue->cRxLock;

            traceQUEUE_RECEIVE_FROM_ISR( pxQueue );

            prvCopyDataFromQueue( pxQueue, pvBuffer );
            pxQueue->uxMessagesWaiting = ( UBaseType_t ) ( uxMessagesWaiting - ( UBaseType_t ) 1 );

            /* If the queue is locked the event list will not be modified.
             * Instead update the lock count so the task that unlocks the queue
             * will know that an ISR has removed data while the queue was
             * locked. */
            if( cRxLock == queueUNLOCKED )
            {
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
                {
                    if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
                    {
                        /* The task waiting has a higher priority than us so
                         * force a context switch. */
                        if( pxHigherPriorityTaskWoken != NULL )
                        {
                            *pxHigherPriorityTaskWoken = pdTRUE;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                /* Increment the lock count so the task that unlocks the queue
                 * knows that data was removed while it was locked. */
                prvIncrementQueueRxLock( pxQueue, cRxLock );
            }

            xReturn = pdPASS;
        }
        else
        {
            xReturn = pdFAIL;
            traceQUEUE_RECEIVE_FROM_ISR_FAILED( pxQueue );
        }
    }
    taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

    traceRETURN_xQueueReceiveFromISR( xReturn );

    return xReturn;
}
```

与任务版本类似，但不能阻塞。如果队列为空，立即返回失败。

使用了`taskENTER_CRITICAL_FROM_ISR`和`taskEXIT_CRITICAL_FROM_ISR`来保护临界区，确保在中断上下文中安全访问队列。

返回值：如果成功接收数据，返回`pdPASS`；如果队列为空，返回`pdFAIL`。

`pxHigherPriorityTaskWoken`表示是否有更高优先级的任务被唤醒，如果是，调用者应在中断返回时进行上下文切换。

```CPP
if( xTaskWokenByReceive != ( char ) pdFALSE )
{
    portYIELD_FROM_ISR (xTaskWokenByReceive);
}
```

## 发送

```CPP
BaseType_t xQueueSend(
    QueueHandle_t xQueue,
    const void * pvItemToQueue,
    TickType_t xTicksToWait
);
BaseType_t xQueueSendToFront(
    QueueHandle_t    xQueue,
    const void       *pvItemToQueue,
    TickType_t       xTicksToWait
);

BaseType_t xQueueSendFromISR(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,
    BaseType_t *pxHigherPriorityTaskWoken
);

BaseType_t xQueueSendToFrontFromISR(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,
    BaseType_t *pxHigherPriorityTaskWoken
);
```

发送（写入）一个项到队列。发送到队列的末尾或前端。以及中断版本。

此外，还有一个特殊的发送函数`xQueueOverwrite`，用于覆盖队列中的现有项，仅适用于长度为1的队列。它是为了实现一个线程安全的全局变量。

```CPP
BaseType_t xQueueOverwrite(
    QueueHandle_t xQueue,
    const void * pvItemToQueue
);
BaseType_t xQueueOverwriteFromISR(
    QueueHandle_t xQueue,
    const void * pvItemToQueue,
    BaseType_t *pxHigherPriorityTaskWoken
);
```

### 实现

```CPP
BaseType_t xQueueGenericSend( QueueHandle_t xQueue,
                              const void * const pvItemToQueue,
                              TickType_t xTicksToWait,
                              const BaseType_t xCopyPosition )
{
    BaseType_t xEntryTimeSet = pdFALSE, xYieldRequired;
    TimeOut_t xTimeOut;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueGenericSend( xQueue, pvItemToQueue, xTicksToWait, xCopyPosition );

    configASSERT( pxQueue );
    configASSERT( !( ( pvItemToQueue == NULL ) && ( pxQueue->uxItemSize != ( UBaseType_t ) 0U ) ) );
    configASSERT( !( ( xCopyPosition == queueOVERWRITE ) && ( pxQueue->uxLength != 1 ) ) );
    #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
    {
        configASSERT( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0 ) ) );
    }
    #endif

    for( ; ; )
    {
        taskENTER_CRITICAL();
        {
            /* Is there room on the queue now?  The running task must be the
             * highest priority task wanting to access the queue.  If the head item
             * in the queue is to be overwritten then it does not matter if the
             * queue is full. */
            if( ( pxQueue->uxMessagesWaiting < pxQueue->uxLength ) || ( xCopyPosition == queueOVERWRITE ) )
            {
                traceQUEUE_SEND( pxQueue );

                #if ( configUSE_QUEUE_SETS == 1 )
                {
                    const UBaseType_t uxPreviousMessagesWaiting = pxQueue->uxMessagesWaiting;

                    xYieldRequired = prvCopyDataToQueue( pxQueue, pvItemToQueue, xCopyPosition );

                    if( pxQueue->pxQueueSetContainer != NULL )
                    {
                        if( ( xCopyPosition == queueOVERWRITE ) && ( uxPreviousMessagesWaiting != ( UBaseType_t ) 0 ) )
                        {
                            /* Do not notify the queue set as an existing item
                             * was overwritten in the queue so the number of items
                             * in the queue has not changed. */
                            mtCOVERAGE_TEST_MARKER();
                        }
                        else if( prvNotifyQueueSetContainer( pxQueue ) != pdFALSE )
                        {
                            /* The queue is a member of a queue set, and posting
                             * to the queue set caused a higher priority task to
                             * unblock. A context switch is required. */
                            queueYIELD_IF_USING_PREEMPTION();
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else
                    {
                        /* If there was a task waiting for data to arrive on the
                         * queue then unblock it now. */
                        if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                        {
                            if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                            {
                                /* The unblocked task has a priority higher than
                                 * our own so yield immediately.  Yes it is ok to
                                 * do this from within the critical section - the
                                 * kernel takes care of that. */
                                queueYIELD_IF_USING_PREEMPTION();
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        else if( xYieldRequired != pdFALSE )
                        {
                            /* This path is a special case that will only get
                             * executed if the task was holding multiple mutexes
                             * and the mutexes were given back in an order that is
                             * different to that in which they were taken. */
                            queueYIELD_IF_USING_PREEMPTION();
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                }
                #else /* configUSE_QUEUE_SETS */
                {
                    xYieldRequired = prvCopyDataToQueue( pxQueue, pvItemToQueue, xCopyPosition );

                    /* If there was a task waiting for data to arrive on the
                     * queue then unblock it now. */
                    if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                    {
                        if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                        {
                            /* The unblocked task has a priority higher than
                             * our own so yield immediately.  Yes it is ok to do
                             * this from within the critical section - the kernel
                             * takes care of that. */
                            queueYIELD_IF_USING_PREEMPTION();
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else if( xYieldRequired != pdFALSE )
                    {
                        /* This path is a special case that will only get
                         * executed if the task was holding multiple mutexes and
                         * the mutexes were given back in an order that is
                         * different to that in which they were taken. */
                        queueYIELD_IF_USING_PREEMPTION();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                #endif /* configUSE_QUEUE_SETS */

                taskEXIT_CRITICAL();

                traceRETURN_xQueueGenericSend( pdPASS );

                return pdPASS;
            }
            else
            {
                if( xTicksToWait == ( TickType_t ) 0 )
                {
                    /* The queue was full and no block time is specified (or
                     * the block time has expired) so leave now. */
                    taskEXIT_CRITICAL();

                    /* Return to the original privilege level before exiting
                     * the function. */
                    traceQUEUE_SEND_FAILED( pxQueue );
                    traceRETURN_xQueueGenericSend( errQUEUE_FULL );

                    return errQUEUE_FULL;
                }
                else if( xEntryTimeSet == pdFALSE )
                {
                    /* The queue was full and a block time was specified so
                     * configure the timeout structure. */
                    vTaskInternalSetTimeOutState( &xTimeOut );
                    xEntryTimeSet = pdTRUE;
                }
                else
                {
                    /* Entry time was already set. */
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
        taskEXIT_CRITICAL();

        /* Interrupts and other tasks can send to and receive from the queue
         * now the critical section has been exited. */

        vTaskSuspendAll();
        prvLockQueue( pxQueue );

        /* Update the timeout state to see if it has expired yet. */
        if( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) == pdFALSE )
        {
            if( prvIsQueueFull( pxQueue ) != pdFALSE )
            {
                traceBLOCKING_ON_QUEUE_SEND( pxQueue );
                vTaskPlaceOnEventList( &( pxQueue->xTasksWaitingToSend ), xTicksToWait );

                /* Unlocking the queue means queue events can effect the
                 * event list. It is possible that interrupts occurring now
                 * remove this task from the event list again - but as the
                 * scheduler is suspended the task will go onto the pending
                 * ready list instead of the actual ready list. */
                prvUnlockQueue( pxQueue );

                /* Resuming the scheduler will move tasks from the pending
                 * ready list into the ready list - so it is feasible that this
                 * task is already in the ready list before it yields - in which
                 * case the yield will not cause a context switch unless there
                 * is also a higher priority task in the pending ready list. */
                if( xTaskResumeAll() == pdFALSE )
                {
                    taskYIELD_WITHIN_API();
                }
            }
            else
            {
                /* Try again. */
                prvUnlockQueue( pxQueue );
                ( void ) xTaskResumeAll();
            }
        }
        else
        {
            /* The timeout has expired. */
            prvUnlockQueue( pxQueue );
            ( void ) xTaskResumeAll();

            traceQUEUE_SEND_FAILED( pxQueue );
            traceRETURN_xQueueGenericSend( errQUEUE_FULL );

            return errQUEUE_FULL;
        }
    }
}
/*-----------------------------------------------------------*/
```

进入临界区，检查队列是否有空间。如果有空间，复制数据到队列，并处理等待接收数据的任务。

如果队列满且指定了等待时间，任务将被阻塞，直到有空间可用或超时。

中断版本与任务版本类似，但不能阻塞。如果队列满，立即返回失败。

## 信号量

信号量的底层是长度为count，大小为零的队列。可以用在中断中。

用于资源管理和任务同步。

### 二值信号量

它的计数值只有 0 和 1。 它通常不用于保护资源（互斥），而是用于 任务同步 (Synchronization)。

它实际上是长度为1，大小为零的队列。

创建二值信号量：

```CPP
SemaphoreHandle_t xSemaphoreCreateBinary( void );
SemaphoreHandle_t xSemaphoreCreateBinaryStatic( StaticSemaphore_t *pxStaticSemaphore );
```

发送和接收二值信号量：

```CPP
BaseType_t xSemaphoreGive( SemaphoreHandle_t xSemaphore );
BaseType_t xSemaphoreTake( SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait );
BaseType_t xSemaphoreGiveFromISR( SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken );
BaseType_t xSemaphoreTakeFromISR( SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken );
``` 

### 计数信号量

计数信号量的计数值可以在一个范围内变化，允许多个任务访问共享资源。

它实际上是长度为count，大小为零的队列。

创建计数信号量：

```CPP
SemaphoreHandle_t xSemaphoreCreateCounting( UBaseType_t uxMaxCount, UBaseType_t uxInitialCount );
SemaphoreHandle_t xSemaphoreCreateCountingStatic( UBaseType_t uxMaxCount, UBaseType_t uxInitialCount, StaticSemaphore_t *pxStaticSemaphore );
``` 

发送和接收计数信号量：

```CPP
BaseType_t xSemaphoreGive( SemaphoreHandle_t xSemaphore );
BaseType_t xSemaphoreTake( SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait );
BaseType_t xSemaphoreGiveFromISR( SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken );
BaseType_t xSemaphoreTakeFromISR( SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken );
```

### 实现

```CPP
#define xSemaphoreCreateBinary()    xQueueGenericCreate( ( UBaseType_t ) 1, semSEMAPHORE_QUEUE_ITEM_LENGTH, queueQUEUE_TYPE_BINARY_SEMAPHORE )
#define xSemaphoreCreateCounting( uxMaxCount, uxInitialCount )    xQueueCreateCountingSemaphore( ( uxMaxCount ), ( uxInitialCount ) )
```

可见，信号量实际上是队列的一个特例。

## 互斥量

互斥量（Mutex）用于保护共享资源，确保同一时间只有一个任务可以访问该资源。

互斥量实际上是一个特殊的二值信号量，具有优先级继承机制，以防止优先级反转问题。

互斥量不能在中断服务例程（ISR）中使用。因为ISR不能阻塞，且也没有holder的概念。

创建互斥量：

```CPP
SemaphoreHandle_t xSemaphoreCreateMutex( void );
SemaphoreHandle_t xSemaphoreCreateMutexStatic( StaticSemaphore_t *pxStaticSemaphore );
```

发送和接收互斥量：

```CPP
BaseType_t xSemaphoreGive( SemaphoreHandle_t xMutex );
BaseType_t xSemaphoreTake( SemaphoreHandle_t xMutex, TickType_t xTicksToWait );
```

优先级反转问题：

* 高优先级任务等待低优先级任务释放互斥量。
* 中等优先级任务抢占低优先级任务，导致高优先级任务长时间等待。

注意，死锁的问题：

* 任务一获取互斥量A，然后尝试获取互斥量B。
* 任务二获取互斥量B，然后尝试获取互斥量A。

结果是两个任务都在等待对方释放互斥量，导致系统死锁。

注意归还顺序的问题：

* 任务获取多个互斥量时，必须按照与获取顺序相反的顺序释放它们。
* 例如，如果任务先获取互斥量A，然后获取互斥量B，那么它必须先释放互斥量B，然后释放互斥量A。

否则可能导致优先级反转问题，影响系统的实时性能。

### 实现

```CPP
#define xSemaphoreCreateMutex()   xQueueCreateMutex( queueQUEUE_TYPE_MUTEX )
```

```CPP
QueueHandle_t xQueueCreateMutex( const uint8_t ucQueueType )
{
    QueueHandle_t xNewQueue;
    const UBaseType_t uxMutexLength = ( UBaseType_t ) 1, uxMutexSize = ( UBaseType_t ) 0;

    traceENTER_xQueueCreateMutex( ucQueueType );

    xNewQueue = xQueueGenericCreate( uxMutexLength, uxMutexSize, ucQueueType );
    prvInitialiseMutex( ( Queue_t * ) xNewQueue );

    traceRETURN_xQueueCreateMutex( xNewQueue );

    return xNewQueue;
}
```

可见，互斥量实际上是队列的一个特例。还初始化了互斥量相关的数据结构。

在`xSemaphoreGive`和`xSemaphoreTake`中会检测`queueQUEUE_TYPE_MUTEX`是不会互斥量，从而使用优先级继承机制。

优先级继承机制：

```CPP
BaseType_t xTaskPriorityInherit( TaskHandle_t const pxMutexHolder )
{
    TCB_t * const pxMutexHolderTCB = pxMutexHolder;
    BaseType_t xReturn = pdFALSE;

    traceENTER_xTaskPriorityInherit( pxMutexHolder );

    /* If the mutex is taken by an interrupt, the mutex holder is NULL. Priority
        * inheritance is not applied in this scenario. */
    if( pxMutexHolder != NULL )
    {
        /* If the holder of the mutex has a priority below the priority of
            * the task attempting to obtain the mutex then it will temporarily
            * inherit the priority of the task attempting to obtain the mutex. */
        if( pxMutexHolderTCB->uxPriority < pxCurrentTCB->uxPriority )
        {
            /* Adjust the mutex holder state to account for its new
                * priority.  Only reset the event list item value if the value is
                * not being used for anything else. */
            if( ( listGET_LIST_ITEM_VALUE( &( pxMutexHolderTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == ( ( TickType_t ) 0U ) )
            {
                listSET_LIST_ITEM_VALUE( &( pxMutexHolderTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB->uxPriority );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            /* If the task being modified is in the ready state it will need
                * to be moved into a new list. */
            if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxMutexHolderTCB->uxPriority ] ), &( pxMutexHolderTCB->xStateListItem ) ) != pdFALSE )
            {
                if( uxListRemove( &( pxMutexHolderTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
                {
                    /* It is known that the task is in its ready list so
                        * there is no need to check again and the port level
                        * reset macro can be called directly. */
                    portRESET_READY_PRIORITY( pxMutexHolderTCB->uxPriority, uxTopReadyPriority );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                /* Inherit the priority before being moved into the new list. */
                pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority;
                prvAddTaskToReadyList( pxMutexHolderTCB );
                #if ( configNUMBER_OF_CORES > 1 )
                {
                    /* The priority of the task is raised. Yield for this task
                        * if it is not running. */
                    if( taskTASK_IS_RUNNING( pxMutexHolderTCB ) != pdTRUE )
                    {
                        prvYieldForTask( pxMutexHolderTCB );
                    }
                }
                #endif /* if ( configNUMBER_OF_CORES > 1 ) */
            }
            else
            {
                /* Just inherit the priority. */
                pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority;
            }

            traceTASK_PRIORITY_INHERIT( pxMutexHolderTCB, pxCurrentTCB->uxPriority );

            /* Inheritance occurred. */
            xReturn = pdTRUE;
        }
        else
        {
            if( pxMutexHolderTCB->uxBasePriority < pxCurrentTCB->uxPriority )
            {
                /* The base priority of the mutex holder is lower than the
                    * priority of the task attempting to take the mutex, but the
                    * current priority of the mutex holder is not lower than the
                    * priority of the task attempting to take the mutex.
                    * Therefore the mutex holder must have already inherited a
                    * priority, but inheritance would have occurred if that had
                    * not been the case. */
                xReturn = pdTRUE;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    traceRETURN_xTaskPriorityInherit( xReturn );

    return xReturn;
}
```

如果高等级的任务尝试获取被低等级任务持有的互斥量，低等级任务将临时提升到高等级任务的优先级，以减少等待时间。

## 递归互斥量

递归互斥量允许同一任务多次获取同一互斥量，而不会导致死锁。每次获取递归互斥量时，任务必须相应地释放它。

创建递归互斥量：

```CPP
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex( void );
SemaphoreHandle_t xSemaphoreCreateRecursiveMutexStatic( StaticSemaphore_t *pxStaticSemaphore );
```

发送和接收递归互斥量：

```CPP
BaseType_t xSemaphoreTakeRecursive( SemaphoreHandle_t xMutex, TickType_t xTicksToWait );
BaseType_t xSemaphoreGiveRecursive( SemaphoreHandle_t xMutex );
```

### 实现

```CPP
#define xSemaphoreCreateRecursiveMutex()    xQueueCreateMutex( queueQUEUE_TYPE_RECURSIVE_MUTEX )
```

递归互斥量实际上是队列的一个特例。长度为1，大小为零的队列。

```CPP
typedef struct SemaphoreData
{
    TaskHandle_t xMutexHolder;        /**< The handle of the task that holds the mutex. */
    UBaseType_t uxRecursiveCallCount; /**< Maintains a count of the number of times a recursive mutex has been recursively 'taken' when the structure is used as a mutex. */
} SemaphoreData_t;
```

其中`SemaphoreData_t`结构体用于跟踪持有递归互斥量的任务和递归调用计数。

```CPP
#define xSemaphoreTakeRecursive( xMutex, xBlockTime )    xQueueTakeMutexRecursive( ( xMutex ), ( xBlockTime ) )
#define xSemaphoreGiveRecursive( xMutex )    xQueueGiveMutexRecursive( ( xMutex ) )
```

```CPP
BaseType_t xQueueGiveMutexRecursive( QueueHandle_t xMutex )
{
    BaseType_t xReturn;
    Queue_t * const pxMutex = ( Queue_t * ) xMutex;

    traceENTER_xQueueGiveMutexRecursive( xMutex );

    configASSERT( pxMutex );

    /* If this is the task that holds the mutex then xMutexHolder will not
        * change outside of this task.  If this task does not hold the mutex then
        * pxMutexHolder can never coincidentally equal the tasks handle, and as
        * this is the only condition we are interested in it does not matter if
        * pxMutexHolder is accessed simultaneously by another task.  Therefore no
        * mutual exclusion is required to test the pxMutexHolder variable. */
    if( pxMutex->u.xSemaphore.xMutexHolder == xTaskGetCurrentTaskHandle() )
    {
        traceGIVE_MUTEX_RECURSIVE( pxMutex );

        /* uxRecursiveCallCount cannot be zero if xMutexHolder is equal to
            * the task handle, therefore no underflow check is required.  Also,
            * uxRecursiveCallCount is only modified by the mutex holder, and as
            * there can only be one, no mutual exclusion is required to modify the
            * uxRecursiveCallCount member. */
        ( pxMutex->u.xSemaphore.uxRecursiveCallCount )--;

        /* Has the recursive call count unwound to 0? */
        if( pxMutex->u.xSemaphore.uxRecursiveCallCount == ( UBaseType_t ) 0 )
        {
            /* Return the mutex.  This will automatically unblock any other
                * task that might be waiting to access the mutex. */
            ( void ) xQueueGenericSend( pxMutex, NULL, queueMUTEX_GIVE_BLOCK_TIME, queueSEND_TO_BACK );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        xReturn = pdPASS;
    }
    else
    {
        /* The mutex cannot be given because the calling task is not the
            * holder. */
        xReturn = pdFAIL;

        traceGIVE_MUTEX_RECURSIVE_FAILED( pxMutex );
    }

    traceRETURN_xQueueGiveMutexRecursive( xReturn );

    return xReturn;
}
```

如果当前任务是递归互斥量的持有者，则递归调用计数减一。如果计数为零，释放互斥量。否则，返回失败。
