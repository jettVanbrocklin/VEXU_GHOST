#pragma once

#include <cstring>
#include "ghost_control/models/dc_motor_model.hpp"
#include "ghost_control/motor_controller.hpp"
#include "ghost_estimation/filters/second_order_low_pass_filter.hpp"
#include "ghost_util/byte_utils.hpp"
#include "ghost_v5_interfaces/devices/device_interfaces.hpp"

using ghost_util::packByte;
using ghost_util::unpackByte;

using ghost_control::DCMotorModel;
using ghost_control::MotorController;

namespace ghost_v5_interfaces {

namespace devices {

enum ghost_gearset {
	GEARSET_100,
	GEARSET_200,
	GEARSET_600
};

enum ghost_brake_mode {
	BRAKE_MODE_COAST,
	BRAKE_MODE_BRAKE,
	BRAKE_MODE_HOLD,
	BRAKE_MODE_INVALID
};

enum ghost_encoder_unit {
	ENCODER_DEGREES,
	ENCODER_ROTATIONS,
	ENCODER_COUNTS,
	ENCODER_INVALID
};

class MotorDeviceConfig : public DeviceConfig {
public:

	std::shared_ptr<DeviceBase> clone() const override {
		return std::make_shared<MotorDeviceConfig>(*this);
	}

	bool operator==(const DeviceBase &rhs) const override {
		const MotorDeviceConfig *d_rhs = dynamic_cast<const MotorDeviceConfig *>(&rhs);
		return (d_rhs != nullptr) && (port == d_rhs->port) && (name == d_rhs->name) && (type == d_rhs->type) &&
		       (reversed == d_rhs->reversed) && (gearset == d_rhs->gearset) && (brake_mode == d_rhs->brake_mode) &&
		       (encoder_units == d_rhs->encoder_units) && (controller_config == d_rhs->controller_config) &&
		       (filter_config == d_rhs->filter_config) && (model_config == d_rhs->model_config);
	}

	bool reversed = false;
	MotorController::Config controller_config;
	SecondOrderLowPassFilter::Config filter_config;
	DCMotorModel::Config model_config;

	// These three map 1:1 to their PROS counterpart on the V5 Side.
	ghost_encoder_unit encoder_units{ghost_encoder_unit::ENCODER_COUNTS};
	ghost_gearset gearset{ghost_gearset::GEARSET_200};
	ghost_brake_mode brake_mode{ghost_brake_mode::BRAKE_MODE_COAST};
};

class MotorDeviceData : public DeviceData {
public:
	MotorDeviceData(){
		type = device_type_e::MOTOR;
	}

	// Msg Size
	int getActuatorPacketSize() const override {
		return 5 * 4 + 1;
	}

	int getSensorPacketSize() const override {
		return 7 * 4;
	}

	// Actuator Values
	float position_command = 0.0;   // Degrees
	float velocity_command = 0.0;   // RPM
	float torque_command = 0.0;     // N - m
	float voltage_command = 0.0;    // Normalized (-1.0->1.0)
	float current_limit = 0.0;      // milliAmps
	bool position_control = false;
	bool velocity_control = false;
	bool torque_control = false;
	bool voltage_control = false;

	// Sensor Values
	float curr_position = 0.0;
	float curr_velocity_rpm = 0.0;
	float curr_torque_nm = 0.0;     // N - m
	float curr_voltage_mv = 0.0;    // MilliVolts
	float curr_current_ma = 0.0;    // MilliAmps
	float curr_power_w = 0.0;       // Watts
	float curr_temp_c = 0.0;        // Celsius

	void update(std::shared_ptr<DeviceData> data_ptr) override {
		auto motor_data_ptr = data_ptr->as<MotorDeviceData>();
		position_command = motor_data_ptr->position_command;
		velocity_command = motor_data_ptr->velocity_command;
		torque_command = motor_data_ptr->torque_command;
		voltage_command = motor_data_ptr->voltage_command;
		current_limit = motor_data_ptr->current_limit;
		position_control = motor_data_ptr->position_control;
		velocity_control = motor_data_ptr->velocity_control;
		torque_control = motor_data_ptr->torque_control;
		voltage_control = motor_data_ptr->voltage_control;
		curr_position = motor_data_ptr->curr_position;
		curr_velocity_rpm = motor_data_ptr->curr_velocity_rpm;
		curr_torque_nm  = motor_data_ptr->curr_torque_nm;
		curr_voltage_mv = motor_data_ptr->curr_voltage_mv;
		curr_current_ma = motor_data_ptr->curr_current_ma;
		curr_power_w = motor_data_ptr->curr_power_w;
		curr_temp_c = motor_data_ptr->curr_temp_c;
	}

	std::shared_ptr<DeviceBase> clone() const override {
		return std::make_shared<MotorDeviceData>(*this);
	}

	bool operator==(const DeviceBase &rhs) const override {
		const MotorDeviceData *d_rhs = dynamic_cast<const MotorDeviceData *>(&rhs);
		return (d_rhs != nullptr) && (name == d_rhs->name) && (type == d_rhs->type) &&
		       (position_command == d_rhs->position_command) && (velocity_command == d_rhs->velocity_command) &&
		       (torque_command == d_rhs->torque_command) && (voltage_command == d_rhs->voltage_command) &&
		       (current_limit == d_rhs->current_limit) && (position_control == d_rhs->position_control) &&
		       (velocity_control == d_rhs->velocity_control) && (torque_control == d_rhs->torque_control) &&
		       (voltage_control == d_rhs->voltage_control) && (curr_position == d_rhs->curr_position) &&
		       (curr_velocity_rpm == d_rhs->curr_velocity_rpm) && (curr_torque_nm == d_rhs->curr_torque_nm) &&
		       (curr_voltage_mv == d_rhs->curr_voltage_mv) && (curr_current_ma == d_rhs->curr_current_ma) &&
		       (curr_power_w == d_rhs->curr_power_w) && (curr_temp_c == d_rhs->curr_temp_c);
	}

	std::vector<unsigned char> serialize(hardware_type_e hardware_type) const override {
		std::vector<unsigned char> msg;
		if(hardware_type == hardware_type_e::COPROCESSOR){
			msg.resize(getActuatorPacketSize(), 0);
			auto msg_buffer = msg.data();
			int byte_offset = 0;
			memcpy(msg_buffer + byte_offset, &position_command, 4);
			byte_offset += 4;
			memcpy(msg_buffer + byte_offset, &velocity_command, 4);
			byte_offset += 4;
			memcpy(msg_buffer + byte_offset, &torque_command, 4);
			byte_offset += 4;
			memcpy(msg_buffer + byte_offset, &voltage_command, 4);
			byte_offset += 4;
			memcpy(msg_buffer + byte_offset, &current_limit, 4);
			byte_offset += 4;

			unsigned char ctrl_byte = packByte(
				std::vector<bool>{
						position_control,
						velocity_control,
						torque_control,
						voltage_control,
						0,
						0,
						0,
						0});

			memcpy(msg_buffer + byte_offset, &ctrl_byte, 1);
			byte_offset++;
			checkMsgSize(msg, getActuatorPacketSize());
		}
		else if(hardware_type == hardware_type_e::V5_BRAIN){
			msg.resize(getSensorPacketSize(), 0);
			auto msg_buffer = msg.data();
			int byte_offset = 0;
			memcpy(msg_buffer + byte_offset, &curr_position, 4);
			byte_offset += 4;
			memcpy(msg_buffer + byte_offset, &curr_velocity_rpm, 4);
			byte_offset += 4;
			memcpy(msg_buffer + byte_offset, &curr_torque_nm, 4);
			byte_offset += 4;
			memcpy(msg_buffer + byte_offset, &curr_voltage_mv, 4);
			byte_offset += 4;
			memcpy(msg_buffer + byte_offset, &curr_current_ma, 4);
			byte_offset += 4;
			memcpy(msg_buffer + byte_offset, &curr_power_w, 4);
			byte_offset += 4;
			memcpy(msg_buffer + byte_offset, &curr_temp_c, 4);
			byte_offset += 4;
			checkMsgSize(msg, getSensorPacketSize());
		}
		else{
			throw std::runtime_error("[MotorDeviceData::deserialize] Error: Received unsupported hardware type " + std::to_string(hardware_type) + " on motor " + name);
		}
		return msg;
	}

	void deserialize(const std::vector<unsigned char>& msg, hardware_type_e hardware_type) override {
		if(hardware_type == hardware_type_e::V5_BRAIN){
			// Actuator Msg
			checkMsgSize(msg, getActuatorPacketSize());
			auto msg_buffer = msg.data();
			int byte_offset = 0;
			memcpy(&position_command, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			memcpy(&velocity_command, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			memcpy(&torque_command, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			memcpy(&voltage_command, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			memcpy(&current_limit, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			unsigned char ctrl_byte;
			memcpy(&ctrl_byte, msg_buffer + byte_offset, 1);

			auto ctrl_vec = unpackByte(ctrl_byte);

			position_control = ctrl_vec[0];
			velocity_control = ctrl_vec[1];
			torque_control = ctrl_vec[2];
			voltage_control = ctrl_vec[3];
		}
		else if(hardware_type == hardware_type_e::COPROCESSOR){
			// Sensor Msg
			checkMsgSize(msg, getSensorPacketSize());
			auto msg_buffer = msg.data();
			int byte_offset = 0;
			memcpy(&curr_position, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			memcpy(&curr_velocity_rpm, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			memcpy(&curr_torque_nm, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			memcpy(&curr_voltage_mv, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			memcpy(&curr_current_ma, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			memcpy(&curr_power_w, msg_buffer + byte_offset, 4);
			byte_offset += 4;
			memcpy(&curr_temp_c, msg_buffer + byte_offset, 4);
		}
		else{
			throw std::runtime_error("[MotorDeviceData::deserialize] Error: Received unsupported hardware type " + std::to_string(hardware_type) + " on motor " + name);
		}
	}
};

} // namespace devices

} // namespace ghost_v5_interfaces