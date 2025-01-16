# perf

`perf`是`Linux`的代码性能分析工具。

参考文档

* [perf wiki](https://perfwiki.github.io/main/)
* [Perf – Linux下的系統性能調優工具](https://jasonblog.github.io/note/linux_tools/perf_--_linuxxia_de_xi_tong_xing_neng_diao_you_gon.html)

## 重要概念

### performance monitor unit

PMU单元是现代处理器的硬件设施,PMU允许软件对某种硬件事件设置计数器,此后处理器就开始统计事件的发生次数,当超过指定的计数器值后,便产生中断.通过捕获这些中断,便可以考察对程序对这些硬件的利用效率.

### Tracepoints

`Tracepoints`是内核代码里的hook,一旦使能,它们便可以在特定的代码被运行到时被触发,`perf`便利用了这个特性.

### Event

`perf`事件是可以触发`perf`采样的事件.主要分为三类.

* `Hardware Event`是由硬件`PMU`产生的事件.比如`Cache`未命中.
* `Software Event`是内核软件产生的事件,比如进程切换,`tick`数等.
* `Tracepoint Event`是内核`Tracepoints`产生的事件.

### Event-based sampling

`perf`是基于事件的采样方法，当事件发生时，就会递增一个计数器，计数器溢出时触发中断,`perf`就会进行一次采样.`perf`记录当前程序信息.

### CPU bound与IO bound

`CPU bound`型程序大部分时间都在利用`CPU`进行计算,`IO bound`程序大部分时间都耗费在了`IO`上.区分这两种程序有助于针对程序的特点进行调优.

### environment selection

`perf`可以在`per-thread`,`per-process`,`per-cpu`,`system-wide`模式下采集信息.

`per-thread`模式下,`perf`仅监视指定线程的执行。

`per-process`模式下,`perf`会监视指定的进程所有线程.

`per-cpu`模式下,`perf`会监视指定cpu.

## perf list

`perf list`列出所有`perf`可以捕获的事件.

## perf stat

`perf stat`以精简概括被调试程序运行的整体情况与总体数据.

以下是一个实例程序的结果输出.

![perf stat](./picture/perf_stat.png)

* `Task-clock-msecs`表示`CPU`的利用率,这个占总时间的比值越大,意味着程序花费了更多的CPU计算.这个比值就是0.994.
* `Context-switches`上下文切换次数,记录了程序发生了多少次上下文切换,频繁的上下文切换应该被避免.
* `Cache-misses`记录了程序运行中`Cache`的使用情况.如果该值过高,说明程序对`Cache`的利用率不好.
* `CPU-migrations`记录了程序运行过程中发生了多少次CPU迁移.
* `Cycles`记录了处理器时钟,实际运行的CPU频率.注意`cpu_atom`是`intel`的低功耗小核,`cpu_core`是`intel`的高性能大核.
* `Instructions`处理器指令数目.
* `IPC`即`insn per cycle`每个处理器时钟执行的指令数目.
* `Cache-references`,`Cache`命中次数.
* `Cache-misses`,`Cache`未命中次数.
* `branches`处理器的分支数目.
* `branches-missed`分支预测失败数目,以及相对于所有分支的比值.

下方还有总用时,用户态用时,系统调用用时。

使用`-e`可以改变`perf stat`显示的事件.

## perf record

