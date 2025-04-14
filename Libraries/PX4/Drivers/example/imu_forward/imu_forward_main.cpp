#include "imu_forward.hpp"
#include <px4_platform_common/getopt.h>
#include <px4_platform_common/module.h>

namespace imu_forward
{
IMUForward *g_dev{nullptr};

static int start(const char *port)
{
	if (g_dev != nullptr) {
		PX4_WARN("already started");
		return -1;
	}

	if (port == nullptr) {
		PX4_ERR("no device specified");
		return -1;
	}

	/* create the driver */
	g_dev = new IMUForward(port);

	if (g_dev == nullptr) {
		return -1;
	}

	if (g_dev->init() != PX4_OK) {
		delete g_dev;
		g_dev = nullptr;
		return -1;
	}

	return 0;
}

static int stop()
{
	if (g_dev != nullptr) {
		delete g_dev;
		g_dev = nullptr;

	} else {
		return -1;
	}

	return 0;
}

static int status()
{
	if (g_dev == nullptr) {
		PX4_ERR("driver not running");
		return -1;
	}

	g_dev->print_info();

	return 0;
}

static int usage()
{
	PRINT_MODULE_DESCRIPTION(
		R"DESCR_STR(
### Description

imu forward to serial, use baud rate 926100

### Examples

Attempt to start imu forward on a specified serial device.
$ imu_forward start -d /dev/ttyS1
Stop driver
$ imu_forward stop
)DESCR_STR");

	PRINT_MODULE_USAGE_NAME("imu_forward", "driver");
	PRINT_MODULE_USAGE_COMMAND_DESCR("start", "Start driver");
	PRINT_MODULE_USAGE_PARAM_STRING('d', nullptr, nullptr, "Serial device", false);
	PRINT_MODULE_USAGE_COMMAND_DESCR("stop", "Stop driver");
	return PX4_OK;
}

} // namespace

extern "C" __EXPORT int imu_forward_main(int argc, char *argv[])
{
	const char *device_path = nullptr;
	int ch;
	int myoptind = 1;
	const char *myoptarg = nullptr;

	while ((ch = px4_getopt(argc, argv, "d:", &myoptind, &myoptarg)) != EOF) {
		switch (ch) {
		case 'd':
			device_path = myoptarg;
			break;

		default:
			imu_forward::usage();
			return -1;
		}
	}

	if (myoptind >= argc) {
		imu_forward::usage();
		return -1;
	}

	if (!strcmp(argv[myoptind], "start")) {
		return imu_forward::start(device_path);

	} else if (!strcmp(argv[myoptind], "stop")) {
		return imu_forward::stop();

	} else if (!strcmp(argv[myoptind], "status")) {
		return imu_forward::status();
	}

	imu_forward::usage();
	return -1;
}

