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

## 组装与控制API
