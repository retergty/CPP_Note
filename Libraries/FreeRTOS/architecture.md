# arm体系

## M7核的缓存

`I-Cache`：指令缓存

`D-Cache`：数据缓存

### 数据缓存的写时策略

`Write-Back (回写)`：数据先写入缓存，只有在缓存行被替换时才写回主存。提高了写操作的效率，但可能导致数据不一致。

`Write-Through (直写)`：数据同时写入缓存和主存。确保数据一致性，但写操作较慢。

当DMA访问内存时，若D-Cache启用且使用Write-Back策略，可能导致DMA读取到过时的数据。为避免此问题，可以：

* 禁用D-Cache
* 使用Write-Through策略

* 开辟一块不缓存的内存区域，专门用于DMA操作.

    在连接脚本中

    ```linker
    /* 在 linker script 中 */
    .dma_buffer_section (NOLOAD) : {
        *(.dma_buffer)
    } > RAM_D3
    ```

    在代码中

    ```CPP
    // 把变量强行放到这个段里
    __attribute__((section(".dma_buffer"))) uint8_t dma_rx_buf[1024];
    ```

* 在DMA操作前后手动清除或刷新D-Cache

    ```CPP
    void Send_Data_Via_DMA(uint8_t* buf, uint32_t len) {
        // 1. CPU 填充数据
        buf[0] = 0x12; 
        buf[1] = 0x34;

        // 2. 【关键】在开启 DMA 前，把 Cache 里的新数据刷入 RAM
        // 注意：地址通常需要 32 字节对齐（后面会讲）
        SCB_CleanDCache_by_Addr((uint32_t*)buf, len);

        // 3. 启动 DMA
        HAL_UART_Transmit_DMA(&huart1, buf, len);
    }

    // DMA 传输完成中断回调
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
        // 1. 【关键】DMA 改写了 RAM，我们需要让 CPU 里的 Cache 失效
        // 这样 CPU 下次读取 buf 时，就会去 RAM 拿最新的数据
        SCB_InvalidateDCache_by_Addr((uint32_t*)rx_buffer, rx_len);
        
        // 2. 现在 CPU 可以安全读取了
        Process_Data(rx_buffer);
    }
    ```

    `up_clean_dcache(start_addr, end_addr)`：CPU 检查指定地址范围内的 Cache 行（Cache Line）。如果发现有被修改（Dirty）的 Cache 行，就把它们的内容写回到主存中，确保主存数据是最新的。

    `SCB_InvalidateDCache_by_Addr(start_addr, end_addr)`：CPU 检查指定地址范围内的 Cache 行，并将它们标记为无效（Invalid）。这样，下一次 CPU 访问这些地址时，会直接从主存中读取数据，而不是使用缓存中的数据，确保读取到的是最新的数据。**它不管 Cache 里的数据是不是“脏（Dirty）”的，直接丢弃掉。**这意味着，如果`Cache`里的数据被修改过但还没写回主存，调用这个函数会导致这些修改丢失(未对齐导致的数据丢失)。

    注意`up_clean_dcache`和`SCB_InvalidateDCache_by_Addr`方向正好相反，一个是写回主存，一个是让缓存失效。

    注意内存地址通常需要 32 字节对齐，确保缓存行完整覆盖数据区域，也保证其它与其相邻的数据不会丢失，否则可能无法正确刷新或失效缓存。

    ```CPP
    // GCC 语法：强制 32 字节对齐
    __attribute__((aligned(32))) uint8_t dma_buf[32];
    ```

## 寄存器

ARMCortex-M7 处理器包含16个通用寄存器，用于存储数据和控制处理器的操作。主要寄存器包括：

中断发生时，会自动保存一部分寄存器，但在任务切换时，FreeRTOS会保存更多寄存器以确保任务状态完整恢复。

* `R0 - R3` (Caller Saved / 参数寄存器)
    当调用一个 C 函数`foo(a, b, c, d)`时,`a, b, c, d`分别存放在`R0, R1, R2, R3`中,返回值通常放在R0中。
* `R4 - R11` (Callee Saved / 被调用者保存寄存器)
    存放函数的局部变量，编译器会尽量把 int i, j, k 这种局部变量分配在这里。C 语言标准规定：如果一个子函数要使用`R4-R11`，它必须在使用前先把旧值保存起来，退出前恢复。这意味着 R4-R11 的值通常比较稳定。
* `R12` (Intra-Procedure-call scratch register)
    通常用作临时过渡寄存器。
* `R13/SP` (Stack Pointer / 堆栈指针)
    指向当前堆栈的顶部指向最后压入数据的那个地址。堆栈用于存储函数调用的返回地址、局部变量等。
  * `MSP (Main Stack Pointer)`：用于处理器在特权模式下运行时的堆栈操作，通常用于中断处理和操作系统内核。
  * `PSP (Process Stack Pointer)`：用于处理器在非特权模式下运行时的堆栈操作，通常用于用户任务。
* `R14/LR` (Link Register / 链接寄存器)
    存储函数调用的返回地址。当一个函数被调用时，返回地址会被存储在`LR`中，以便函数执行完毕后能够正确返回调用点。
* `R15/PC` (Program Counter / 程序计数器)
    指向当前正在执行的指令
* `xPSR` (Program Status Register / 程序状态寄存器)
    包含处理器的状态信息，如条件标志、中断使能状态等。

此外还有一些特殊寄存器：

* `CONTROL` (Control Register / 控制寄存器)
    用于控制处理器的特权级别和堆栈指针的选择。
* `BASEPRI` (Base Priority Register / 基本优先级寄存器)
    用于设置中断的优先级屏蔽阈值，低于该优先级的中断将被屏蔽。

中断发生时，处理器会自动保存`R0-R3, R12, LR, PC, xPSR`等寄存器到当前堆栈中，以便中断处理完成后能够恢复现场。

FreeRTOS在任务切换时，会保存更多寄存器（包括`R4-R11`），以确保任务状态的完整恢复。

此外，如果启用了浮点运算单元（FPU），FreeRTOS还会保存和恢复浮点寄存器（`S0-S31`），以确保浮点运算的正确性。

## 中断流程

1. 中断触发：外设或事件触发中断信号。
    * CPU状态：Thread Mode
    * SP指针：PSP
2. 中断响应：硬件自动压栈。
    * CPU动作：自动保存`R0-R3, R12, LR, PC, xPSR`到当前堆栈（PSP）。
3. 堆栈切换：切换到MSP堆栈。
    * CPU状态：Handler Mode
    * SP指针：MSP
4. 中断处理：执行中断服务程序（ISR）。
    * CPU跳转到中断向量表中的ISR地址，开始执行中断处理代码。
    * 使用的栈：MSP堆栈。
5. 中断返回：硬件自动弹栈，恢复寄存器。
    * CPU: 检测到要返回Thread Mode
    * 堆栈切换：SP指针切换回PSP堆栈。
    * 自动恢复`R0-R3, R12, LR, PC, xPSR`等寄存器。
6. 继续执行：返回中断前的代码位置，继续执行任务。
