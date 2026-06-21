# gstreamer 应用层

## 初始化与选项读取API

```CPP
int
main (int   argc,
      char *argv[])
{
  const gchar *nano_str;
  guint major, minor, micro, nano;

  gst_init (&argc, &argv);

  gst_version (&major, &minor, &micro, &nano);

  if (nano == 1)
    nano_str = "(CVS)";
  else if (nano == 2)
    nano_str = "(Prerelease)";
  else
    nano_str = "";

  printf ("This program is linked against GStreamer %d.%d.%d %s\n",
          major, minor, micro, nano_str);

  return 0;
}
```

* `gst_init()`：初始化GStreamer库，必须在使用任何其他GStreamer函数之前调用。
* `gst_version()`：获取GStreamer库的版本信息。

## 定义参数


```CPP
#include <gst/gst.h>

int
main (int   argc,
      char *argv[])
{
  gboolean silent = FALSE;
  gchar *savefile = NULL;
  GOptionContext *ctx;
  GError *err = NULL;
  GOptionEntry entries[] = {
    { "silent", 's', 0, G_OPTION_ARG_NONE, &silent,
      "do not output status information", NULL },
    { "output", 'o', 0, G_OPTION_ARG_STRING, &savefile,
      "save xml representation of pipeline to FILE and exit", "FILE" },
    { NULL }
  };

  ctx = g_option_context_new ("- Your application");
  g_option_context_add_main_entries (ctx, entries, NULL);
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
    g_print ("Failed to initialize: %s\n", err->message);
    g_clear_error (&err);
    g_option_context_free (ctx);
    return 1;
  }
  g_option_context_free (ctx);

  printf ("Run me with --help to see the Application options appended.\n");

  return 0;
}
```

这是用户自定义参数的程序段，不需要重复调用`gst_init()`。

```CPP
GOptionEntry entries[] = {
    { "silent", 's', 0, G_OPTION_ARG_NONE, &silent, "do not output status information", NULL },
    { "output", 'o', 0, G_OPTION_ARG_STRING, &savefile, "save xml representation of pipeline to FILE and exit", "FILE" },
    { NULL }
  };
```

定义程序可以接受的参数,使用的是`GOption`

`GOptionEntry`包括以下内容

* `long_name` (长选项名)
  * **类型**: `const gchar *` (字符串)
  * **含义**: 用户在终端敲的完整单词。
* `short_name` (短选项名)
  * **类型**: `gchar` (单个字符)
  * **含义**: 供用户偷懒用的单字母缩写。不需要则直接填0.
* `flags` (标志位)
  * **类型**: `gint` (整数宏)
  * **含义**: 一些特殊的控制行为。
* `arg` (参数数据类型)
  * **类型**: `GOptionArg` (枚举值)
  * **含义**: 最核心的字段,告诉解析器，用户在这个参数后面会跟什么类型的值。它直接决定了你的程序会不会因为类型不匹配而报错。
  * **常用枚举**:
    * `G_OPTION_ARG_NONE`: 不带参数的开关（如 --silent）。
    * `G_OPTION_ARG_STRING`: 后接字符串（如 --output=test.mp4）。
    * `G_OPTION_ARG_INT`: 后接整数（如 --framerate=60）。
    * `G_OPTION_ARG_FILENAME`: 专门用于文件名，GLib 会自动帮你处理 Linux 下的路径编码转换问题。
* `arg_data` (数据存储指针)
  * **类型**: `gpointer` (等同于`void *`指针)
  * **含义**: 解析器提取出用户的输入后，把数据塞到你代码里的哪个变量里。
  * **铁律**: 必须传入变量的物理地址（加 & 取址符）
    * 如果是`G_OPTION_ARG_NONE`，这里必须填一个`gboolean`变量的地址（如`&silent`）。
    * 如果是`G_OPTION_ARG_STRING`，这里必须填一个`gchar *`指针的地址（如`&savefile`）。
* `description` (帮助描述文字)
  * **类型**: `const gchar *`(字符串)
  * **含义**: 当用户敲`--help`时，显示在参数右侧的解释说明。
* `arg_description` (参数值占位符)
  * **类型**: `const gchar *` (字符串)
  * **含义**: 当用户敲`--help`时，提示用户这个参数应该传什么“形状”的值。只对带有具体值的参数有效。
  * **例子**: 如果填`"FILE"`，帮助文档里就会显示`--output=FILE`。如果你填`"IP_ADDRESS"`，帮助文档就会显示`--host=IP_ADDRESS`。如果是不带值的开关（如`--silent`），这里填`NULL`即可。

```CPP
ctx = g_option_context_new ("- Your application");
g_option_context_add_main_entries (ctx, entries, NULL);
g_option_context_add_group (ctx, gst_init_get_option_group ());
```

这段代码的作用是创建一个`GOptionContext`对象，并添加主参数和组参数。主参数是我们之前通过`entries`定义的参数，组参数是GStreamer的选项组。

* `g_option_context_new()`：创建一个`GOptionContext`对象。这个对象是解析参数的上下文。
* `g_option_context_add_main_entries()`：添加主参数。
* `g_option_context_add_group()`：添加组参数。
* `gst_init_get_option_group()`：获取GStreamer的选项组。

```CPP
if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
    g_print ("Failed to initialize: %s\n", err->message);
    g_clear_error (&err);
    g_option_context_free (ctx);
    return 1;
  }
  g_option_context_free (ctx);
```

这段代码的作用是解析参数。

* `g_option_context_parse()`：解析参数。
* `g_clear_error()`：清除错误。
* `g_option_context_free()`：释放`GOptionContext`对象。

## GstElement

### 创建释放GstElement

```CPP
#include <gst/gst.h>

int
main (int   argc,
      char *argv[])
{
  GstElement *element;

  /* init GStreamer */
  gst_init (&argc, &argv);

  /* create element */
  element = gst_element_factory_make ("fakesrc", "source");
  if (!element) {
    g_print ("Failed to create element of type 'fakesrc'\n");
    return -1;
  }

  gst_object_unref (GST_OBJECT (element));

  return 0;
}
```

这段代码的作用是创建一个`GstElement`对象。创建了一个名为`source`的`fakesrc`元素。

* `GstElement * gst_element_factory_make (const gchar *factoryname,const gchar *name)`：创建一个`GstElement`对象。`factoryname`是工厂名/类型名.`name`是元素的名称。
* `gst_object_unref()`：释放`GstElement`对象。

```CPP
#include <gst/gst.h>

int
main (int   argc,
      char *argv[])
{
  GstElementFactory *factory;
  GstElement * element;

  /* init GStreamer */
  gst_init (&argc, &argv);

  /* create element, method #2 */
  factory = gst_element_factory_find ("fakesrc");
  if (!factory) {
    g_print ("Failed to find factory of type 'fakesrc'\n");
    return -1;
  }
  element = gst_element_factory_create (factory, "source");
  if (!element) {
    g_print ("Failed to create element, even though its factory exists!\n");
    return -1;
  }

  gst_object_unref (GST_OBJECT (element));
  gst_object_unref (GST_OBJECT (factory));

  return 0;
}
```

这段代码首先通过`gst_element_factory_find()`找到`fakesrc`工厂，然后通过`gst_element_factory_create()`创建一个`GstElement`对象。相比第一种方法，这种方法可以更灵活地创建元素。

* `GstElementFactory * gst_element_factory_find (const gchar *factoryname)`：找到一个`factoryname`对应的工厂。
* `GstElement * gst_element_factory_create (GstElementFactory *factory, const gchar *name)`：创建一个`factory`类型的`name`元素。

### 将GstElement用作GObject

`GstElement`继承了`GObject`，所以一些`GObject`的API也可以用于`GstElement`。可以设置`GstElement`的属性（参数）。

```CPP
#include <gst/gst.h>

int
main (int   argc,
      char *argv[])
{
  GstElement *element;
  gchar *name;

  /* init GStreamer */
  gst_init (&argc, &argv);

  /* create element */
  element = gst_element_factory_make ("fakesrc", "source");

  /* get name */
  g_object_get (G_OBJECT (element), "name", &name, NULL);
  g_print ("The name of the element is '%s'.\n", name);
  g_free (name);

  gst_object_unref (GST_OBJECT (element));

  return 0;
}
```

这段代码获取了`GstElement`的`name`属性。

* `g_object_get()`：获取`GObject`的`name`属性。
* `g_free()`：释放`name`属性。

### 使用GstElementFactory获取GstElement的信息

`GstElementFactory`是`GstElement`的工厂，使用`gst-inspect-1.0`会给通用的信息，比如插件作者、描述名、短名、rank（优先级）、category（分类）等。

`category`用来判断`GstElement`的类型/角色。比如`Codec/Decoder/Video`视频解码器，`Codec/Encoder/Video`视频编码器，`Source/Video`视频源，`Sink/Video`视频输出端等。

```CPP
#include <gst/gst.h>

int
main (int   argc,
      char *argv[])
{
  GstElementFactory *factory;

  /* init GStreamer */
  gst_init (&argc, &argv);

  /* get factory */
  factory = gst_element_factory_find ("fakesrc");
  if (!factory) {
    g_print ("You don't have the 'fakesrc' element installed!\n");
    return -1;
  }

  /* display information */
  g_print ("The '%s' element is a member of the category %s.\n"
           "Description: %s\n",
           gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory)),
           gst_element_factory_get_metadata (factory, GST_ELEMENT_METADATA_KLASS),
           gst_element_factory_get_metadata (factory, GST_ELEMENT_METADATA_DESCRIPTION));

  gst_object_unref (GST_OBJECT (factory));

  return 0;
}
```

这段代码讲解了使用代码获取`GstElementFactory`的信息。

* `gst_plugin_feature_get_name()`：获取工厂的名称。
* `gst_element_factory_get_metadata()`：获取工厂的元数据。

### 连接GstElement

```CPP
#include <gst/gst.h>

int
main (int   argc,
      char *argv[])
{
  GstElement *pipeline;
  GstElement *source, *filter, *sink;

  /* init */
  gst_init (&argc, &argv);

  /* create pipeline */
  pipeline = gst_pipeline_new ("my-pipeline");

  /* create elements */
  source = gst_element_factory_make ("fakesrc", "source");
  filter = gst_element_factory_make ("identity", "filter");
  sink = gst_element_factory_make ("fakesink", "sink");

  /* must add elements to pipeline before linking them */
  gst_bin_add_many (GST_BIN (pipeline), source, filter, sink, NULL);

  /* link */
  if (!gst_element_link_many (source, filter, sink, NULL)) {
    g_warning ("Failed to link elements!");
  }

[..]

}
```

这段代码使用`gst_element_link_many()`快速连接了`source`、`filter`和`sink`。`gst_element_link_many()`会自动处理`source`和`filter`的连接，以及`filter`和`sink`的连接。本质上还是pad的连接。

* `gst_element_link_many()`：快速连接多个元素。
* `gst_bin_add_many()`：添加多个元素到`bin`中。
* `gst_object_unref()`：释放`GstElement`对象。

连接方式从“便捷”到“精细”大致分三层：

* `gst_element_link_many()`：批量按顺序连接元素。
* `gst_element_link()` / `gst_element_link_pads()`：单对元素连接，可指定pad名。
* `gst_pad_link_*()`：拿到具体pad后手动连接，控制最细。

实践中应遵循：

* 先把元素加入同一个`bin/pipeline`，再执行链接（推荐顺序：`create -> add -> link`）。
* 不要直接跨不同层级的内部元素做连接；若跨层级连接，请用`GhostPad`把内部pad暴露到bin边界后再连。

### GstElement状态

创建GstElement后，`GstElement`不会做任何事情，直到将它的状态改变。`GstElement`四个状态如下

* `GST_STATE_NULL`（默认状态）：
  * 不分配任何运行资源。
  * 切换回该状态会释放已分配资源。
  * 元素在引用计数归零并销毁时，必须处于该状态。

* `GST_STATE_READY`（就绪态）：
  * 分配“全局资源”（例如打开设备、申请缓冲区等）。
  * 但还不会真正打开媒体流，因此流位置通常为0。
  * 如果之前打开过流，进入该状态时应关闭流，并重置位置与相关属性。

* `GST_STATE_PAUSED`（暂停态）：
  * 流已经打开，但不进行“驱动时钟前进”的播放。
  * 可以做定位、预读、解码等准备工作，为进入`PLAYING`做预热。
  * 可理解为“和`PLAYING`几乎相同，但时钟不运行”。
  * 例如音视频输出可先缓存数据，视频sink甚至可先显示第一帧（不推进时钟）。

* `GST_STATE_PLAYING`（播放态）：
  * 行为基本与`PAUSED`一致，但时钟开始运行，数据按时间线真实播放。

```CPP
gst_element_set_state (GST_ELEMENT (element), GST_STATE_PLAYING);
```

可以通过`gst_element_set_state()`将`GstElement`的状态改变。`GstElement`会逐级改变元素的状态，比如将`PLAYING`设置为`NULL`，会先变为`READY`，再变为`PAUSED`，再变为`NULL`。

当你对`bin/pipeline`设置目标状态时，这个状态通常会自动向下传播到其内部元素。因此在常见场景中，只需要对顶层`pipeline`调用一次`gst_element_set_state()`，就能完成整条管线的启动或停止。

但有一个关键例外：如果在管线已经运行时动态加入新元素（例如在`pad-added`回调中），新加入的元素不会总是自动进入当前运行状态。此时需要你手动同步状态，常见做法有两种：

* `gst_element_set_state (new_element, target_state)`：直接把新元素切到目标状态。
* `gst_element_sync_state_with_parent (new_element)`：让新元素跟随父`bin/pipeline`的当前状态，通常更安全、也更常用。

## GstBin

### 创建bin

```CPP
#include <gst/gst.h>

int
main (int   argc,
      char *argv[])
{
  GstElement *bin, *pipeline, *source, *sink;

  /* init */
  gst_init (&argc, &argv);

  /* create */
  pipeline = gst_pipeline_new ("my_pipeline");
  bin = gst_bin_new ("my_bin");
  source = gst_element_factory_make ("fakesrc", "source");
  sink = gst_element_factory_make ("fakesink", "sink");

  /* First add the elements to the bin */
  gst_bin_add_many (GST_BIN (bin), source, sink, NULL);
  /* add the bin to the pipeline */
  gst_bin_add (GST_BIN (pipeline), bin);

  /* link the elements */
  gst_element_link (source, sink);

[..]

}
```

这段代码创建了一个`bin`，并将其添加到`pipeline`中。当把`GstElement`添加到`bin`中时，`bin`就获得了这个元素的所有权，当`bin`被销毁时，这些元素会被`unref`。

* `gst_bin_new()`：创建一个`bin`。
* `gst_bin_add_many()`：添加多个元素到`bin`中。
* `gst_bin_add()`：添加一个元素到`bin`中。

## Bus

### 使用BUS

```CPP
#include <gst/gst.h>

static GMainLoop *loop;

static gboolean
my_bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  g_print ("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      g_print ("Error: %s\n", err->message);
      g_error_free (err);
      g_free (debug);

      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_EOS:
      /* end-of-stream */
      g_main_loop_quit (loop);
      break;
    default:
      /* unhandled message */
      break;
  }

  /* we want to be notified again the next time there is a message
   * on the bus, so returning TRUE (FALSE means we want to stop watching
   * for messages on the bus and our callback should not be called again)
   */
  return TRUE;
}

gint
main (gint argc, gchar * argv[])
{
  GstElement *pipeline;
  GstBus *bus;
  guint bus_watch_id;

  /* init */
  gst_init (&argc, &argv);

  /* create pipeline, add handler */
  pipeline = gst_pipeline_new ("my_pipeline");

  /* adds a watch for new message on our pipeline's message bus to
   * the default GLib main context, which is the main context that our
   * GLib main loop is attached to below
   */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, my_bus_callback, NULL);
  gst_object_unref (bus);

  /* [...] */

  /* create a mainloop that runs/iterates the default GLib main context
   * (context NULL), in other words: makes the context check if anything
   * it watches for has happened. When a message has been posted on the
   * bus, the default main context will automatically call our
   * my_bus_callback() function to notify us of that message.
   * The main loop will be run until someone calls g_main_loop_quit()
   */
  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  /* clean up */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
```

这段代码监听`pipeline`的消息总线，当收到消息时，会调用`my_bus_callback()`函数。

消息运行在`g_main_loop`中，所以需要创建一个`GMainLoop`对象。

* `gst_bus_add_watch()`：添加一个监听器到`bus`。
* `g_source_remove()`：移除一个监听器。
* `g_main_loop_unref()`：释放`GMainLoop`对象。
* `g_main_loop_run()`: 运行主循环

Bus回调的线程语义和适用边界：

* 使用`gst_bus_add_watch()`/`gst_bus_add_signal_watch()`时，回调运行在`GMainLoop`所属线程（通常是主线程）。
* Bus是异步通知通道，适合处理`ERROR`、`EOS`、状态变化等控制消息。
* 不适合做强实时媒体处理（如精确crossfade、理论无缝拼接、逐帧特效）。
* 强实时处理应放在pipeline内部上下文中，通常通过元素/插件实现。

默认`GLib mainloop`下，不一定非要使用`gst_bus_add_watch() + switch(message type)`；也可以用`gst_bus_add_signal_watch()`并按`message::<type>`分别连接你关心的消息信号。

```CPP
GstBus *bus;

[..]

bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
gst_bus_add_signal_watch (bus);
g_signal_connect (bus, "message::error", G_CALLBACK (cb_message_error), NULL);
g_signal_connect (bus, "message::eos", G_CALLBACK (cb_message_eos), NULL);

[..]
```

这个程序段使用`gst_bus_add_signal_watch()`监听`message::error`和`message::eos`消息。避免在一个大的callback中使用switch.

### 定义消息类型

`GstBus`上传递的是`GstMessage`。消息类型可扩展：除内置类型外，插件也可定义自定义消息。应用至少应处理`ERROR`消息。

每条消息都包含通用信息：

* `source`：消息来源元素。
* `type`：消息类型。
* `timestamp`：消息时间戳。

常见消息及处理方式：

* `ERROR/WARNING/INFO`：分别用`gst_message_parse_error()`、`gst_message_parse_warning()`、`gst_message_parse_info()`解析；错误消息通常意味着当前数据流中断。
* `EOS`：流结束，pipeline不会自动切状态，但后续处理会停住；可用于切下一首或执行seek后继续播放。
* `TAG`：流元数据（如标题、作者、码率），可能多次出现；用`gst_message_parse_tag()`解析，使用后`gst_tag_list_unref()`。
* `STATE_CHANGED`：状态切换成功后发出；用`gst_message_parse_state_changed()`读取旧状态与新状态。
* `BUFFERING`：网络流缓冲进度消息，可从`gst_message_get_structure()`中读取百分比。

按需处理的消息：

* `ELEMENT`消息：某些元素自定义的高级消息（需查该元素文档）。
* `APPLICATION`消息：应用自定义消息，常用于把流线程的信息投递到主线程。

实践建议：最小必处理集合是`ERROR + EOS`，播放器类应用通常再加`STATE_CHANGED + TAG + BUFFERING`。

## GstPad

### 动态pad

```CPP
#include <gst/gst.h>

static void
cb_new_pad (GstElement *element,
        GstPad     *pad,
        gpointer    data)
{
  gchar *name;

  name = gst_pad_get_name (pad);
  g_print ("A new pad %s was created\n", name);
  g_free (name);

  /* here, you would setup a new pad link for the newly created pad */
[..]

}

int
main (int   argc,
      char *argv[])
{
  GstElement *pipeline, *source, *demux;
  GMainLoop *loop;

  /* init */
  gst_init (&argc, &argv);

  /* create elements */
  pipeline = gst_pipeline_new ("my_pipeline");
  source = gst_element_factory_make ("filesrc", "source");
  g_object_set (source, "location", argv[1], NULL);
  demux = gst_element_factory_make ("oggdemux", "demuxer");

  /* you would normally check that the elements were created properly */

  /* put together a pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, demux, NULL);
  gst_element_link_pads (source, "src", demux, "sink");

  /* listen for newly created pads */
  g_signal_connect (demux, "pad-added", G_CALLBACK (cb_new_pad), NULL);

  /* start the pipeline */
  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

[..]

}
```

这段代码演示了动态Pad的用法。它监听`pad-added`信号.

* `gst_element_link_pads()`：连接两个元素的pad。
* `g_signal_connect()`：连接信号。

### 请求pad

```CPP
static void
some_function (GstElement * tee)
{
  GstPad *pad;
  gchar *name;

  pad = gst_element_request_pad_simple (tee, "src%d");
  name = gst_pad_get_name (pad);
  g_print ("A new pad %s was created\n", name);
  g_free (name);

  /* here, you would link the pad */

  /* [..] */

  /* request pad不再使用时，需要先从元素侧释放 */
  gst_element_release_request_pad (tee, pad);

  /* 然后释放本地引用 */
  gst_object_unref (GST_OBJECT (pad));
}
```

这段代码演示了请求pad的用法。它使用`gst_element_request_pad_simple()`请求一个pad。

* `gst_element_request_pad_simple()`：请求一个pad。
* `gst_pad_get_name()`：获取pad的名称。

```CPP
static void
link_to_multiplexer (GstPad * tolink_pad, GstElement * mux)
{
  GstPad *pad;
  gchar *srcname, *sinkname;

  srcname = gst_pad_get_name (tolink_pad);
  pad = gst_element_get_compatible_pad (mux, tolink_pad, NULL);
  gst_pad_link (tolink_pad, pad);
  sinkname = gst_pad_get_name (pad);
  gst_object_unref (GST_OBJECT (pad));

  g_print ("A new pad %s was created and linked to %s\n", sinkname, srcname);
  g_free (sinkname);
  g_free (srcname);
}
```

自动找 mux 的可兼容 sink pad 并完成连接。

* `gst_element_get_compatible_pad()`：获取一个可兼容的pad。

### Pad的能力（Caps）

Pad是元素对外的数据接口，`Caps`用于描述这个接口“能传什么”或“正在传什么”（例如媒体类型、分辨率、采样率、像素格式等）。

`Caps`会出现在两处：

* `Pad Template`上的`Caps`：描述该模板创建出的pad“可能支持”的媒体类型范围。
* 具体`Pad`上的`Caps`：
  * 若尚未协商（not negotiated），通常是“可选能力列表”（常接近模板能力）。
  * 若已协商完成（negotiated），表示该pad当前实际传输的媒体格式。

模板`Caps`回答“可以是什么”，运行时`Pad Caps`回答“现在是什么”。

### Caps结构层次

`Caps`使用`GstCaps`结构表示，内部包含一个或多个`GstStructure`.

每个`GstStructure`表示一种媒体类型方案（例如一种音频格式约束）

已协商（negotiated）的 pad：`GstCaps` 通常只有 1 个 `structure`，且里面是固定值.

未协商`pad` / `pad template`：可以有多个 `structure`，值也可以是范围/列表，不一定固定.

```text
Pad Templates:
  SRC template: 'src'
    Availability: Always
    Capabilities:
      audio/x-raw
                 format: F32LE
                   rate: [ 1, 2147483647 ]
               channels: [ 1, 256 ]

  SINK template: 'sink'
    Availability: Always
    Capabilities:
      audio/x-vorbis
```

这个例子使用`vorbisdec`讲解了`Caps`的结构层次。

`vorbisdec`有两个始终存在的`pad`：

* `sink`：吃压缩数据 `audio/x-vorbis`
* `src`：吐解码后的原始音频 `audio/x-raw`

`src`的`caps`里会有属性约束，比如：

* `format`: `F32LE`
* `rate`: `[1, 2147483647]`（采样率范围）
* `channels`: `[1, 256]`（声道数范围）

这说明`template`阶段是“能力范围”，不是“当前已定值”。

### Caps中的属性值类型（Properties and values）

`Caps`里的属性使用`key=value`描述附加约束，值类型决定“约束是精确值还是可选范围”。

常见类型可分为四类：

* 基本类型：`G_TYPE_INT`、`G_TYPE_BOOLEAN`、`G_TYPE_FLOAT`、`G_TYPE_STRING`、`GST_TYPE_FRACTION`，表示精确值。
* 范围类型：`GST_TYPE_INT_RANGE`、`GST_TYPE_FLOAT_RANGE`、`GST_TYPE_FRACTION_RANGE`，表示上下界区间。
* 列表类型：`GST_TYPE_LIST`，表示“可选其一”（例如采样率可为`44100`或`48000`）。
* 数组类型：`GST_TYPE_ARRAY`，表示“整体一起解释”的值集合（典型是多声道`channel layout`）。

`LIST`与`ARRAY`的关键区别：

* `LIST`：候选集合，协商时从中选一个。
* `ARRAY`：整体语义，数组内容作为一个整体被解释。

### 使用Caps承载元数据（Using capabilities for metadata）

`Caps`不仅用于“能否连接”，也用于描述媒体格式元数据。结构上可理解为：

* `GstCaps`：由一个或多个`GstStructure`组成。
* `GstStructure`：由多个`field`组成，每个`field`是“字段名 + 带类型的值”。

三类容易混淆的`Caps`语义：

* `possible caps`：pad理论可支持的能力（常见于`pad template`）。
* `allowed caps`：在当前对端约束下允许的能力子集。
* `negotiated caps`：最终协商后的实际格式（应为固定值）。

常用读取方式：

* `gst_caps_get_size()`：获取`GstCaps`里`structure`数量。
* `gst_caps_get_structure(caps, i)`：获取第`i`个`GstStructure`并读取字段。

常见术语：

* `simple caps`：仅包含一个`GstStructure`。
* `fixed caps`：仅一个`GstStructure`且无`range/list`等可变项。
* `ANY caps`：任意格式都可接受。
* `empty caps`：不接受任何格式。

```CPP
static void
read_video_props (GstCaps *caps)
{
  gint width, height;
  const GstStructure *str;

  g_return_if_fail (gst_caps_is_fixed (caps));

  str = gst_caps_get_structure (caps, 0);
  if (!gst_structure_get_int (str, "width", &width) ||
      !gst_structure_get_int (str, "height", &height)) {
    g_print ("No width/height available\n");
    return;
  }

  g_print ("The video size of this set of capabilities is %dx%d\n",
       width, height);
}
```

这段函数用于从一组“已协商完成的固定`Caps`”中读取视频元数据（`width`/`height`）。

* `gst_caps_is_fixed(caps)`：先确保`Caps`已固定（不是范围/列表），否则读取出的值不可靠。
* `gst_caps_get_structure(caps, 0)`：取第一个`GstStructure`（固定`Caps`通常只有一个结构）。
* `gst_structure_get_int(..., "width"/"height", ...)`：按字段名读取整数属性。
* 若字段不存在则打印提示并返回；成功则输出分辨率。

### 使用Caps过滤格式

```CPP
static gboolean
link_elements_with_filter (GstElement *element1, GstElement *element2)
{
  gboolean link_ok;
  GstCaps *caps;

  caps = gst_caps_new_simple ("video/x-raw",
          "format", G_TYPE_STRING, "I420",
          "width", G_TYPE_INT, 384,
          "height", G_TYPE_INT, 288,
          "framerate", GST_TYPE_FRACTION, 25, 1,
          NULL);

  link_ok = gst_element_link_filtered (element1, element2, caps);
  gst_caps_unref (caps);

  if (!link_ok) {
    g_warning ("Failed to link element1 and element2!");
  }

  return link_ok;
}
```

用“过滤后的 Caps”去连接两个元素，只允许特定视频格式通过.

这段代码通过`gst_caps_new_simple()`构造约束：`video/x-raw, format=I420, 384x288, 25fps`，然后用`gst_element_link_filtered()`在连接时强制两端按该格式协商。

补充理解：

* 这里使用`GstElement *element1, *element2`是因为`gst_element_link_filtered()`是元素级API，会在内部选择可连接的pad并执行协商。
* 若任一侧不支持这组格式，连接会失败并返回`FALSE`。
* `gst_caps_unref(caps)`用于释放本地`Caps`引用，避免泄漏。

```CPP
static gboolean
link_elements_with_filter (GstElement *element1, GstElement *element2)
{
  gboolean link_ok;
  GstCaps *caps;

  caps = gst_caps_new_full (
      gst_structure_new ("video/x-raw",
             "width", G_TYPE_INT, 384,
             "height", G_TYPE_INT, 288,
             "framerate", GST_TYPE_FRACTION, 25, 1,
             NULL),
      gst_structure_new ("video/x-bayer",
             "width", G_TYPE_INT, 384,
             "height", G_TYPE_INT, 288,
             "framerate", GST_TYPE_FRACTION, 25, 1,
             NULL),
      NULL);

  link_ok = gst_element_link_filtered (element1, element2, caps);
  gst_caps_unref (caps);

  if (!link_ok) {
    g_warning ("Failed to link element1 and element2!");
  }

  return link_ok;
}
```

这段代码使用`gst_caps_new_full()`一次构造了多个候选`GstStructure`，语义是“满足其一即可”（OR关系）。

与前面的`gst_caps_new_simple()`相比：

* `gst_caps_new_simple()`：单一媒体类型/单一结构，约束更单一。
* `gst_caps_new_full()`：可同时声明多种媒体类型（如`video/x-raw`或`video/x-bayer`），协商更灵活。
* 本例中`video/x-raw`未限定`format`，因此比“固定到I420”更宽松。

### Ghost Pad

`Ghost Pad`把`bin`内部某个元素的 pad“映射”到`bin`外层，让外部把这个`bin` 当普通`element`来连接。

```CPP
#include <gst/gst.h>

int
main (int   argc,
      char *argv[])
{
  GstElement *bin, *sink;
  GstPad *pad;

  /* init */
  gst_init (&argc, &argv);

  /* create element, add to bin */
  sink = gst_element_factory_make ("fakesink", "sink");
  bin = gst_bin_new ("mybin");
  gst_bin_add (GST_BIN (bin), sink);

  /* add ghostpad */
  pad = gst_element_get_static_pad (sink, "sink");
  gst_element_add_pad (bin, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));

[..]

}
```

这段代码给`bin`暴露一个`ghost sink pad`，让外部可以把数据直接连到这个`bin`.

执行过程：

* 创建内部元素`fakesink`与容器`bin`，并将`fakesink`加入`bin`。
* 获取内部`fakesink`的静态`sink pad`（真实接收数据的入口）。
* 基于该真实pad创建`ghost pad`并挂到`bin`上（对外名称为`"sink"`）。
* 释放本地`pad`引用，避免引用泄漏。

结果：

* `bin`对外表现为“拥有一个`sink pad`”的元素。
* 外部可直接连接到`bin:sink`，数据会转发到内部`fakesink:sink`。
* `bin`因此可以像普通`GstElement`一样参与更大pipeline的连接与复用。

原因：

* `bin`与其子元素属于不同层级对象，子元素的pad默认不等于`bin`的pad。
* 外部连接只能面向`bin`自身接口，不能把内部pad直接当作`bin`接口使用。
* `ghost pad`就是把内部真实pad“映射”为`bin`的对外pad，这是标准做法。

