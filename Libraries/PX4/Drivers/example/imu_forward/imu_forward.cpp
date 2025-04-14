/****************************************************************************
 *
 *   Copyright (c) 2013-2019 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file gps.cpp
 * Driver for the GPS on a serial/spi port
 */

#ifdef __PX4_NUTTX
#include <nuttx/clock.h>
#include <nuttx/arch.h>
#endif

#ifndef __PX4_QURT
#include <poll.h>
#endif

#include <cstring>

#include <drivers/drv_sensor.h>
#include <lib/parameters/param.h>
#include <mathlib/mathlib.h>
#include <matrix/math.hpp>
#include <px4_platform_common/atomic.h>
#include <px4_platform_common/cli.h>
#include <px4_platform_common/getopt.h>
#include <px4_platform_common/module.h>
#include <px4_platform_common/time.h>
#include <px4_platform_common/Serial.hpp>

extern "C"
{
#include <lib/crc/crc.h>
}

#include "imu_forward.hpp"


IMUForward::IMUForward(const char *port) : ScheduledWorkItem(MODULE_NAME, px4::serial_port_to_wq(port)),
	_sample_perf(perf_alloc(PC_ELAPSED, MODULE_NAME": read"))
{
	/* store port name */
	strncpy(_port, port, sizeof(_port) - 1);

	/* enforce null termination */
	_port[sizeof(_port) - 1] = '\0';
}
IMUForward::~IMUForward()
{
	stop();
}
void IMUForward::stop()
{
	ScheduleClear();
}
void IMUForward::FillUpImuStream(const vehicle_imu_s &imu)
{
	_imu_stream.payload.timestamp = imu.timestamp_sample;
	_imu_stream.payload.acc_x = imu.delta_velocity[0] / imu.delta_velocity_dt * 1e6f;
	_imu_stream.payload.acc_y = imu.delta_velocity[1] / imu.delta_velocity_dt * 1e6f;
	_imu_stream.payload.acc_z = imu.delta_velocity[2] / imu.delta_velocity_dt * 1e6f;
	_imu_stream.payload.gyro_x = imu.delta_angle[0]  / imu.delta_angle_dt * 1e6f;
	_imu_stream.payload.gyro_y = imu.delta_angle[1]  / imu.delta_angle_dt * 1e6f;
	_imu_stream.payload.gyro_z = imu.delta_angle[2]  / imu.delta_angle_dt * 1e6f;
	_imu_stream.payload.seq_num++;

	_imu_stream.checksum = crc16_signature(CRC16_INITIAL, sizeof(Payload), (uint8_t *)&_imu_stream.payload);
}
void IMUForward::Run()
{
	perf_begin(_sample_perf);

	if (!_uart.isOpen()) {
		// Configure UART port
		if (!_uart.setPort(_port)) {
			PX4_ERR("Error configuring serial device on port %s", _port);
			return;
		}

		// Configure the desired baudrate if one was specified by the user.
		// Otherwise the default baudrate will be used.
		if (! _uart.setBaudrate(921600U)) {
			PX4_ERR("Error setting baudrate to %u on %s", 921600U, _port);
			return;
		}

		// Open the UART. If this is successful then the UART is ready to use.
		if (! _uart.open()) {
			PX4_ERR("Error opening serial device  %s", _port);
			return;
		}
	}

	if (_uart.isOpen()) {
		if (_vehicle_imu_sub.update(&_vehicle_imu)) {
			FillUpImuStream(_vehicle_imu);
			size_t sended = _uart.write(&_imu_stream, sizeof(_imu_stream));

			if (sended != sizeof(_imu_stream)) {
				PX4_ERR("Error in forwarding imu, Sended %u, expected %u ", sended, sizeof(_imu_stream));
			}
		}
	}

	perf_end(_sample_perf);
}
void IMUForward::print_info()
{
	PX4_INFO("Now forwarding packet:\n");
	PX4_INFO("start_1: %hhu, start_2: %hhu, start_3: %hhu, start_4: %hhu\n", _imu_stream.start_1, _imu_stream.start_2,
		 _imu_stream.start_3, _imu_stream.start_4);
	PX4_INFO("payload length: %lu", _imu_stream.payload_length);
	PX4_INFO("timestamp: %llu, sequence number: %llu\n", _imu_stream.payload.timestamp, _imu_stream.payload.seq_num);
	PX4_INFO("accel, x: %f, y: %f, z: %f\n", (double)_imu_stream.payload.acc_x, (double)_imu_stream.payload.acc_y,
		 (double)_imu_stream.payload.acc_z);
	PX4_INFO("gyro, x: %f, y: %f, z: %f\n\n", (double)_imu_stream.payload.gyro_x, (double)_imu_stream.payload.gyro_y,
		 (double)_imu_stream.payload.gyro_z);
	PX4_INFO("Serial information:\n");
	PX4_INFO("configured serial port: %s, expected %s,", _uart.getPort(), _port);

	if (_uart.isOpen()) { PX4_INFO("opening\n"); }

	else { PX4_INFO("closed\n"); }

	PX4_INFO("configured serial baudrate: %lu\n", _uart.getBaudrate());
	PX4_INFO("configured byte size: 8 bits, parity: none, step bits: 1 bit, flow control: no\n");
	perf_print_counter(_sample_perf);
}
int IMUForward::init()
{
	_imu_stream.start_1 = 0x11;
	_imu_stream.start_2 = 0x22;
	_imu_stream.start_3 = 0x33;
	_imu_stream.start_4 = 0x44;
	_imu_stream.payload.seq_num = 0;
	_imu_stream.payload_length = sizeof(Payload);

	if (!_vehicle_imu_sub.registerCallback()) {
		PX4_ERR("callback registration failed");
		return PX4_ERROR;
	}

	return PX4_OK;
}
