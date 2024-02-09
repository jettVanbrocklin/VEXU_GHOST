#pragma once

#include "eigen3/Eigen/Geometry"

#include <rclcpp/rclcpp.hpp>

#include <builtin_interfaces/msg/time.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

namespace ghost_sensing {

class IMUFilterNode : public rclcpp::Node {
public:
	IMUFilterNode();
private:
	void imu_callback(const sensor_msgs::msg::Imu::SharedPtr msg);
	visualization_msgs::msg::MarkerArray getMarkerArrayVector3(const Eigen::Vector3d& vector, double scale);
	visualization_msgs::msg::Marker getDefaultArrowMsg(int id, builtin_interfaces::msg::Time stamp);

	// Sensor Params
	double m_sensor_freq;

	// Bias Calibration
	int m_calibration_time;
	bool m_calculate_bias = false;
	bool m_calibration_complete = false;
	int m_msg_count;
	int m_num_msgs_init;
	std::vector<Eigen::Vector3d> m_imu_accel_samples;
	std::vector<Eigen::Vector3d> m_imu_gyro_samples;

	// Sensor Bias and Covariance
	Eigen::Vector3d m_imu_accel_bias;
	Eigen::Vector3d m_imu_gyro_bias;
	Eigen::Matrix3d m_imu_accel_bias_covariance;
	Eigen::Matrix3d m_imu_gyro_bias_covariance;

	// Base Link Transform
	Eigen::Matrix3d m_base_link_to_camera_rotation;
	Eigen::Vector2d m_base_link_to_camera_translation;

	// ROS Topics
	rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr m_imu_sub;
	rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr m_filtered_imu_pub;
	rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr m_vel_viz_pub;
	rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr m_accel_viz_pub;
	rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr m_vel_filtered_viz_pub;
	rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr m_accel_filtered_viz_pub;
};

} // namespace ghost_sensing