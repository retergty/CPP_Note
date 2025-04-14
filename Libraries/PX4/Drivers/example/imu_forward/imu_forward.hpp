#ifdef __PX4_NUTTX
#include <nuttx/clock.h>
#include <nuttx/arch.h>
#endif

#ifndef __PX4_QURT
#include <poll.h>
#endif

#include <lib/drivers/device/Device.hpp>
#include <px4_platform_common/atomic.h>
#include <px4_platform_common/cli.h>
#include <px4_platform_common/getopt.h>
#include <px4_platform_common/time.h>
#include <px4_platform_common/Serial.hpp>
#include <px4_platform_common/px4_config.h>
#include <px4_platform_common/px4_work_queue/ScheduledWorkItem.hpp>
#include <uORB/SubscriptionCallback.hpp>
#include <uORB/topics/vehicle_imu.h>
#include <cstdint>

struct Payload {
	uint64_t timestamp;
	uint64_t seq_num;
	float acc_x;
	float acc_y;
	float acc_z;
	float gyro_x;
	float gyro_y;
	float gyro_z;
};

struct ImuStream {
	uint8_t start_1; // = 0x11;
	uint8_t start_2; // = 0x22;
	uint8_t start_3; // = 0x33;
	uint8_t start_4; // = 0x44;
	uint32_t payload_length;
	Payload payload;
	uint16_t checksum;
};

static_assert(sizeof(ImuStream) == 56);
static_assert(sizeof(Payload) == 40);

class IMUForward : public px4::ScheduledWorkItem
{
public:
	IMUForward(const char *port);
	~IMUForward() override;
	int init();
	void print_info();
private:
	void Run() override;
	void stop();
	void FillUpImuStream(const vehicle_imu_s &imu);

	device::Serial  _uart {};
	char _port[20] {};
	uORB::SubscriptionCallbackWorkItem _vehicle_imu_sub{this, ORB_ID(vehicle_imu)};
	vehicle_imu_s _vehicle_imu;
	ImuStream _imu_stream;

	perf_counter_t	_sample_perf;
};
