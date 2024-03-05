#pragma once

#include <string>
#include "behaviortree_cpp/behavior_tree.h"
#include "rclcpp/rclcpp.hpp"

// SyncActionNode (synchronous action) with an input port.
class LoggingNode : public BT::SyncActionNode,
		             public rclcpp::Node {
public:
		// If your Node has ports, you must use this constructor signature
		LoggingNode(const std::string& name, const BT::NodeConfig& config);

		// It is mandatory to define this STATIC method.
		static BT::PortsList providedPorts();

		// Override the virtual function tick()
		BT::NodeStatus tick() override;
};