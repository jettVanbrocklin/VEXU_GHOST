#pragma once

#include <memory>
#include <ghost_util/test_util.hpp>
#include "ghost_v5_interfaces/devices/joystick_device_interface.hpp"
#include "ghost_v5_interfaces/devices/motor_device_interface.hpp"
#include "ghost_v5_interfaces/devices/rotation_sensor_device_interface.hpp"

namespace ghost_v5_interfaces {

namespace test_util {

devices::MotorDeviceData::SerialConfig getRandomMotorSerialConfig(){
	devices::MotorDeviceData::SerialConfig config;
	config.send_position_command = ghost_util::getRandomBool();
	config.send_velocity_command = ghost_util::getRandomBool();
	config.send_voltage_command = ghost_util::getRandomBool();
	config.send_torque_command = ghost_util::getRandomBool();
	config.send_torque_data = ghost_util::getRandomBool();
	config.send_voltage_data = ghost_util::getRandomBool();
	config.send_current_data = ghost_util::getRandomBool();
	config.send_power_data = ghost_util::getRandomBool();
	config.send_temp_data = ghost_util::getRandomBool();
	return config;
}

devices::RotationSensorDeviceData::SerialConfig getRandomRotationSensorSerialConfig(){
	devices::RotationSensorDeviceData::SerialConfig config;
	config.send_angle_data = ghost_util::getRandomBool();
	config.send_position_data = ghost_util::getRandomBool();
	config.send_velocity_data = ghost_util::getRandomBool();
	return config;
}

std::shared_ptr<devices::JoystickDeviceData> getRandomJoystickData(){
	auto joy_ptr = std::make_shared<devices::JoystickDeviceData>("joy_" + std::to_string(rand() % 2));

	joy_ptr->left_x = ghost_util::getRandomFloat();
	joy_ptr->left_y = ghost_util::getRandomFloat();
	joy_ptr->right_x = ghost_util::getRandomFloat();
	joy_ptr->right_y = ghost_util::getRandomFloat();
	joy_ptr->btn_a = ghost_util::getRandomBool();
	joy_ptr->btn_b = ghost_util::getRandomBool();
	joy_ptr->btn_x = ghost_util::getRandomBool();
	joy_ptr->btn_y = ghost_util::getRandomBool();
	joy_ptr->btn_r1 = ghost_util::getRandomBool();
	joy_ptr->btn_r2 = ghost_util::getRandomBool();
	joy_ptr->btn_l1 = ghost_util::getRandomBool();
	joy_ptr->btn_l2 = ghost_util::getRandomBool();
	joy_ptr->btn_u = ghost_util::getRandomBool();
	joy_ptr->btn_l = ghost_util::getRandomBool();
	joy_ptr->btn_r = ghost_util::getRandomBool();
	joy_ptr->btn_d = ghost_util::getRandomBool();

	return joy_ptr;
}

std::shared_ptr<devices::MotorDeviceData> getRandomMotorData(bool actuator_cmd,
                                                             devices::MotorDeviceData::SerialConfig serial_config = devices::MotorDeviceData::SerialConfig()){
	auto motor_ptr = std::make_shared<devices::MotorDeviceData>("test", serial_config);

	// Actuator Values
	if(actuator_cmd){
		if(serial_config.send_position_command){
			motor_ptr->position_command = ghost_util::getRandomFloat();
			motor_ptr->position_control = ghost_util::getRandomBool();
		}
		if(serial_config.send_velocity_command){
			motor_ptr->velocity_command = ghost_util::getRandomFloat();
			motor_ptr->velocity_control = ghost_util::getRandomBool();
		}
		if(serial_config.send_voltage_command){
			motor_ptr->voltage_command = ghost_util::getRandomFloat();
			motor_ptr->voltage_control = ghost_util::getRandomBool();
		}
		if(serial_config.send_torque_command){
			motor_ptr->torque_command = ghost_util::getRandomFloat();
			motor_ptr->torque_control = ghost_util::getRandomBool();
		}
		motor_ptr->current_limit = ghost_util::getRandomFloat();
	}
	else{
		motor_ptr->curr_position = ghost_util::getRandomFloat();
		motor_ptr->curr_velocity_rpm = ghost_util::getRandomFloat();

		if(serial_config.send_torque_data){
			motor_ptr->curr_torque_nm = ghost_util::getRandomFloat();
		}
		if(serial_config.send_voltage_data){
			motor_ptr->curr_voltage_mv = ghost_util::getRandomFloat();
		}
		if(serial_config.send_current_data){
			motor_ptr->curr_current_ma = ghost_util::getRandomFloat();
		}
		if(serial_config.send_power_data){
			motor_ptr->curr_power_w = ghost_util::getRandomFloat();
		}
		if(serial_config.send_temp_data){
			motor_ptr->curr_temp_c = ghost_util::getRandomFloat();
		}
	}


	return motor_ptr;
}

std::shared_ptr<devices::RotationSensorDeviceData> getRandomRotationSensorData(devices::RotationSensorDeviceData::SerialConfig serial_config = devices::RotationSensorDeviceData::SerialConfig()) {
	auto rot_sensor_ptr = std::make_shared<devices::RotationSensorDeviceData>("test", serial_config);
	if(serial_config.send_angle_data){
		rot_sensor_ptr->angle = ghost_util::getRandomFloat();
	}
	if(serial_config.send_position_data){
		rot_sensor_ptr->position = ghost_util::getRandomFloat();
	}
	if(serial_config.send_velocity_data){
		rot_sensor_ptr->velocity = ghost_util::getRandomFloat();
	}

	return rot_sensor_ptr;
}

}

}