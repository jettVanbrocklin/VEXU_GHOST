#include "main.h"
#include "pros/motors.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <unistd.h>

#include "pros/apix.h"
#include "pros/rtos.h"

#include "ghost_v5/motor/ghost_motor.hpp"
#include "ghost_v5/serial/v5_serial_node.hpp"

using ghost_v5_config::v5_motor_id_enum;
using ghost_v5_config::v5_sensor_id_enum;

void zero_motors(){
	// Zero all motor commands
	for(auto & m : v5_globals::motors){
		m.second->setMotorCommand(0.0);
	}
}

void update_motors(){
	// Update velocity filter and motor controller for all motors
	for(auto & m : v5_globals::motors){
		m.second->updateMotor();
	}
}

void button_callback()
{
}

void actuator_timeout_loop()
{
	uint32_t loop_time = pros::millis();
	while (v5_globals::run)
	{
		if(pros::millis() > v5_globals::last_cmd_time + v5_globals::cmd_timeout_ms){
			zero_motors();
		}
		pros::c::task_delay_until(&loop_time, v5_globals::cmd_timeout_ms);
	}
}

void reader_loop(){
	uint32_t loop_time = pros::millis();
	while (v5_globals::run)
	{
		// Process incoming msgs and update motor cmds
		bool update_recieved = v5_globals::serial_node_.readV5ActuatorUpdate();
		
		// Reset actuator timeout
		if(update_recieved){
			v5_globals::last_cmd_time = pros::millis();
		}
		pros::c::task_delay_until(&loop_time, v5_globals::cmd_timeout_ms);
	}
}

void ghost_main_loop(){
	// Send robot state over serial to coprocessor
	v5_globals::serial_node_.writeV5StateUpdate();
	
	// Zero All Motors if disableds
	if(pros::competition::is_disabled()){
		zero_motors();
	}

	update_motors();
}

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize()
{
	// Motor ports
	// Motor IDs are zero based index. The only time we actually use ports are in init, so we add 1 for V5 Port ID scheme
	v5_globals::motors[v5_motor_id_enum::DRIVE_LEFT_FRONT_MOTOR]   	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::DRIVE_LEFT_FRONT_MOTOR,    false, drive_motor_config);
    v5_globals::motors[v5_motor_id_enum::DRIVE_LEFT_BACK_MOTOR]    	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::DRIVE_LEFT_BACK_MOTOR,     false, drive_motor_config);
    v5_globals::motors[v5_motor_id_enum::DRIVE_RIGHT_FRONT_MOTOR]  	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::DRIVE_RIGHT_FRONT_MOTOR,   false, drive_motor_config);
    v5_globals::motors[v5_motor_id_enum::DRIVE_RIGHT_BACK_MOTOR]   	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::DRIVE_RIGHT_BACK_MOTOR,    false, drive_motor_config);
    v5_globals::motors[v5_motor_id_enum::DRIVE_BACK_LEFT_1_MOTOR]  	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::DRIVE_BACK_LEFT_1_MOTOR,   false, drive_motor_config);
    v5_globals::motors[v5_motor_id_enum::DRIVE_BACK_RIGHT_1_MOTOR] 	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::DRIVE_BACK_RIGHT_1_MOTOR,  false, drive_motor_config);
    v5_globals::motors[v5_motor_id_enum::DRIVE_BACK_LEFT_2_MOTOR]  	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::DRIVE_BACK_LEFT_2_MOTOR,   true,  drive_motor_config);
    v5_globals::motors[v5_motor_id_enum::DRIVE_BACK_RIGHT_2_MOTOR] 	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::DRIVE_BACK_RIGHT_2_MOTOR,  true,  drive_motor_config);
    v5_globals::motors[v5_motor_id_enum::TURRET_MOTOR]             	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::TURRET_MOTOR,              false, turret_motor_config);
    v5_globals::motors[v5_motor_id_enum::INTAKE_MOTOR]             	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::INTAKE_MOTOR,              false, intake_motor_config);
    v5_globals::motors[v5_motor_id_enum::INDEXER_MOTOR]            	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::INDEXER_MOTOR,             true, 	indexer_motor_config);
    v5_globals::motors[v5_motor_id_enum::SHOOTER_LEFT_MOTOR]       	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::SHOOTER_LEFT_MOTOR,        true, 	shooter_motor_config);
    v5_globals::motors[v5_motor_id_enum::SHOOTER_RIGHT_MOTOR]      	= std::make_shared<ghost_v5::GhostMotor>(v5_motor_id_enum::SHOOTER_RIGHT_MOTOR,       false, shooter_motor_config);

	// // Encoder Ports
	v5_globals::encoders[v5_sensor_id_enum::STEERING_LEFT_ENCODER]	= std::make_shared<pros::Rotation>(v5_sensor_id_enum::STEERING_LEFT_ENCODER),
    v5_globals::encoders[v5_sensor_id_enum::STEERING_RIGHT_ENCODER]	= std::make_shared<pros::Rotation>(v5_sensor_id_enum::STEERING_RIGHT_ENCODER),
    v5_globals::encoders[v5_sensor_id_enum::STEERING_BACK_ENCODER]	= std::make_shared<pros::Rotation>(v5_sensor_id_enum::STEERING_BACK_ENCODER),
	
	zero_motors();

	pros::lcd::initialize();
	pros::Controller controller_main(pros::E_CONTROLLER_MASTER);

	// Perform and necessary Serial Init
	v5_globals::serial_node_.initSerial();
	// pros::Task producer_thread(producer_main, "producer thread");
	// pros::Task actuator_timeout_thread(actuator_timeout_loop, "actuator timeout thread");
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled()
{
	uint32_t loop_time = pros::millis();
	while (pros::competition::is_disabled())
	{
		ghost_main_loop();
		pros::c::task_delay_until(&loop_time, 10);
	}
}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous()
{
	uint32_t loop_time = pros::millis();
	while (pros::competition::is_autonomous())
	{
		ghost_main_loop();
		pros::c::task_delay_until(&loop_time, 10);
	}
}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
void opcontrol()
{
	uint32_t loop_time = pros::millis();
	while (!pros::competition::is_autonomous() && !pros::competition::is_disabled())
	{
		ghost_main_loop();
		pros::c::task_delay_until(&loop_time, 10);
	}
}

/*	Differential Swerve Transform
	double wheel_vel = controller_main.get_analog(ANALOG_RIGHT_Y)*500.0/127.0;	// 500 RPM
	double module_vel = controller_main.get_analog(ANALOG_LEFT_Y)*200.0/127.0;	// 200 RPM

	std::cout << wheel_vel << ", " << module_vel << std::endl;
	std::cout << 0.9*(0.6*wheel_vel + 1.5*module_vel) << std::endl;
	std::cout << 0.9*(-0.6*wheel_vel + 1.5*module_vel) << std::endl;
	std::cout << std::endl;

	m1.move_velocity(0.9*(0.6*wheel_vel + 1.5*module_vel));
	m2.move_velocity(0.9*(-0.6*wheel_vel + 1.5*module_vel));
*/