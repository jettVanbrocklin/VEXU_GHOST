#ifndef GHOST_V5__GHOST_MOTOR_HPP
#define GHOST_V5__GHOST_MOTOR_HPP

#include <map>
#include "pros/motors.hpp"
#include "ghost_estimation/filters/second_order_low_pass_filter.hpp"
#include "ghost_control/models/dc_motor_model.hpp"
#include "ghost_ros/robot_config/v5_robot_config_defs.hpp"
#include "ghost_ros/robot_config/v5_robot_config.hpp"

namespace ghost_v5
{
    // PROS Config Mappings
    const std::map<ghost_v5_config::ghost_gearset, pros::motor_gearset_e_t> RPM_TO_GEARING_MAP{
        {ghost_v5_config::ghost_gearset::GEARSET_100,    pros::E_MOTOR_GEAR_100},
        {ghost_v5_config::ghost_gearset::GEARSET_200,    pros::E_MOTOR_GEAR_200},
        {ghost_v5_config::ghost_gearset::GEARSET_600,    pros::E_MOTOR_GEAR_600},
        {ghost_v5_config::ghost_gearset::GEARSET_3600,   pros::E_MOTOR_GEAR_600}
    };

    std::map<ghost_v5_config::ghost_brake_mode, pros::motor_brake_mode_e_t> GHOST_BRAKE_MODE_MAP{
        {ghost_v5_config::ghost_brake_mode::BRAKE_MODE_COAST,   pros::E_MOTOR_BRAKE_COAST},
        {ghost_v5_config::ghost_brake_mode::BRAKE_MODE_BRAKE,   pros::E_MOTOR_BRAKE_BRAKE},
        {ghost_v5_config::ghost_brake_mode::BRAKE_MODE_HOLD,    pros::E_MOTOR_BRAKE_HOLD},
        {ghost_v5_config::ghost_brake_mode::BRAKE_MODE_INVALID, pros::E_MOTOR_BRAKE_INVALID}
    };

    std::map<ghost_v5_config::ghost_encoder_unit, pros::motor_encoder_units_e_t> GHOST_ENCODER_UNIT_MAP{
        {ghost_v5_config::ghost_encoder_unit::ENCODER_DEGREES,   pros::E_MOTOR_ENCODER_DEGREES},
        {ghost_v5_config::ghost_encoder_unit::ENCODER_ROTATIONS, pros::E_MOTOR_ENCODER_ROTATIONS},
        {ghost_v5_config::ghost_encoder_unit::ENCODER_COUNTS,    pros::E_MOTOR_ENCODER_COUNTS},
        {ghost_v5_config::ghost_encoder_unit::ENCODER_INVALID,   pros::E_MOTOR_ENCODER_INVALID}
    };

    class GhostMotor : public pros::Motor
    {
    public:
        GhostMotor(int motor_port, bool reversed, ghost_v5_config::GhostMotorConfig &config);

        void updateMotor();

        float getVoltageCommand(){
            return cmd_voltage_mv_;
        }

        float getVelocityFilteredRPM(){
            return velocity_filter_.getCurrentState();
        }

        bool getDeviceIsConnected(){
            return device_connected_;
        }

        void setPositionCommand(float position){
            des_pos_encoder_ = position;
        }
        
        void setVelocityCommand(float velocity){
            des_vel_rpm_ = velocity;
        }
        
        void setTorqueCommand(float torque){
            des_torque_nm_ = torque;
        }

        void setVoltageCommand(float voltage){
            des_voltage_norm_ = voltage;
        }

        void setControlMode(bool voltage_active, bool torque_active, bool velocity_active, bool position_active){
            position_active_ = position_active;
            velocity_active_ = velocity_active;
            torque_active_ = torque_active;
            voltage_active_ = voltage_active;
        }

    private:
        void move_voltage_trq_lim(float voltage_mv);
    
        // Motor Config
        ghost_v5_config::GhostMotorConfig config_;

        bool motor_is_3600_cart_;
        float trq_lim_norm_;

        // Velocity Filtering
        ghost_estimation::SecondOrderLowPassFilter velocity_filter_;
        float curr_vel_rpm_;
        bool device_connected_;

        // Motor Controller
        int32_t des_pos_encoder_;
        float des_vel_rpm_;
        float des_torque_nm_;
        float des_voltage_norm_;
        float cmd_voltage_mv_;
        float ctl_rpm_deadband_;
        
        bool position_active_;
        bool velocity_active_;
        bool torque_active_;
        bool voltage_active_;

        // Motor Models
        ghost_control::DCMotorModel motor_model_;
    };

} // namespace ghost_v5

#endif // GHOST_V5__GHOST_MOTOR_HPP