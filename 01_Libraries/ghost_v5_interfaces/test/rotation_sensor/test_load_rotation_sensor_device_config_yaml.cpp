#include <ghost_v5_interfaces/rotation_sensor/load_rotation_sensor_device_config_yaml.hpp>
#include "gtest/gtest.h"
#include "yaml-cpp/yaml.h"

using namespace ghost_v5_interfaces::util;
using namespace ghost_v5_interfaces;

class TestLoadRotationSensorDeviceConfigYAML : public ::testing::Test {
protected:

	void SetUp() override {
		std::string config_path = std::string(getenv("HOME")) + "/VEXU_GHOST/01_Libraries/ghost_v5_interfaces/test/config/test_load_rotation_sensor_config.yaml";
		config_yaml_ = YAML::LoadFile(config_path);

		// Changed every param
		rotation_sensor_1_ = std::make_shared<RotationSensorDeviceConfig>();
		rotation_sensor_1_->port = 1;
		rotation_sensor_1_->name = "rotation_sensor_1";
		rotation_sensor_1_->type = device_type_e::ROTATION_SENSOR;
		rotation_sensor_1_->reversed = true;
		rotation_sensor_1_->data_rate = 10;

		// Default (minimal required params)
		rotation_sensor_2_ = std::make_shared<RotationSensorDeviceConfig>();
		rotation_sensor_2_->port = 2;
		rotation_sensor_2_->name = "rotation_sensor_2";
		rotation_sensor_2_->type = device_type_e::ROTATION_SENSOR;
	}

	std::shared_ptr<RotationSensorDeviceConfig> rotation_sensor_1_;
	std::shared_ptr<RotationSensorDeviceConfig> rotation_sensor_2_;

	YAML::Node config_yaml_;
};

/**
 * @brief Test that all fields are properly set when loading the RotationSensorDeviceConfig class
 */
TEST_F(TestLoadRotationSensorDeviceConfigYAML, testLoadCustomSensorConfigFromYAML){
	auto config_node = config_yaml_["port_configuration"];
	std::string sensor_name = "rotation_sensor_1";
	auto config_ptr = std::make_shared<RotationSensorDeviceConfig>();
	loadRotationSensorDeviceConfigFromYAML(config_node, sensor_name, config_ptr);
	EXPECT_EQ(*config_ptr, *rotation_sensor_1_);
}

/**
 * @brief Test that if the device config is missing the port attribute, it throws.
 */
TEST_F(TestLoadRotationSensorDeviceConfigYAML, testMissingPortThrowsError){
	auto config_node = config_yaml_["port_configuration"];
	std::string sensor_name = "rotation_sensor_no_port";
	auto config_ptr = std::make_shared<RotationSensorDeviceConfig>();
	EXPECT_THROW(loadRotationSensorDeviceConfigFromYAML(config_node, sensor_name, config_ptr), std::runtime_error);
}

/**
 * @brief Test that if the device config is missing the type attribute, it throws
 */
TEST_F(TestLoadRotationSensorDeviceConfigYAML, testMissingTypeThrowsError){
	auto config_node = config_yaml_["port_configuration"];
	std::string sensor_name = "rotation_sensor_no_type";
	auto config_ptr = std::make_shared<RotationSensorDeviceConfig>();
	EXPECT_THROW(loadRotationSensorDeviceConfigFromYAML(config_node, sensor_name, config_ptr), std::runtime_error);
}