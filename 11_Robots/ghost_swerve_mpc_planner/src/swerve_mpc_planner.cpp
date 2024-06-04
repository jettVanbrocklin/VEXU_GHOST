/*
 *   Copyright (c) 2024 Maxx Wilson
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include "ghost_swerve_mpc_planner/swerve_mpc_planner.hpp"

using namespace std::chrono_literals;
using namespace casadi;
namespace plt = matplotlibcpp;
using ghost_planners::IterationCallback;
using ghost_planners::Trajectory;

namespace ghost_swerve_mpc_planner
{

SwerveMPCPlanner::SwerveMPCPlanner()
: rclcpp::Node("swerve_mpc_planner_node")
{
  // Load and validate config params
  loadConfig();
  validateConfig();

  // Set state and parameter names
  generateStateNames();
  generateParameterNames();

  // create empty vectors for states, parameters, cost, constraints, and problem bounds
  populateContainers();

  // Set costs, constraints, and state bounds for swerve drive optimization
  addCosts();
  addConstraints();
  setStateBounds();

  initSolver();

  // Init ROS interfaces
  initROS();
}

void SwerveMPCPlanner::loadConfig()
{
  // Solver Args
  declare_parameter("publish_intermediate_solutions", false);
  publish_intermediate_solutions_ = get_parameter("publish_intermediate_solutions").as_bool();

  declare_parameter("ipopt_verbosity", 0);
  ipopt_verbosity_ = get_parameter("ipopt_verbosity").as_int();

  declare_parameter("max_iterations", 10);
  max_iterations_ = get_parameter("max_iterations").as_int();

  // MPC Config
  declare_parameter("time_horizon", 0.0);
  config_.time_horizon = get_parameter("time_horizon").as_double();

  declare_parameter("dt", 0.0);
  config_.dt = get_parameter("dt").as_double();

  declare_parameter("num_swerve_modules", 0);
  config_.num_swerve_modules = get_parameter("num_swerve_modules").as_int();

  declare_parameter("chassis_width", 0.0);
  config_.chassis_width = get_parameter("chassis_width").as_double() * I2M;

  declare_parameter("wheel_base_width", 0.0);
  config_.wheel_base_width = get_parameter("wheel_base_width").as_double() * I2M;

  declare_parameter("wheel_width", 0.0);
  config_.wheel_width = get_parameter("wheel_width").as_double() * I2M;

  declare_parameter("wheel_diam", 0.0);
  config_.wheel_radius = get_parameter("wheel_diam").as_double() * I2M / 2.0;

  declare_parameter("steering_ratio", 0.0);
  config_.steering_ratio = get_parameter("steering_ratio").as_double();

  declare_parameter("wheel_ratio", 0.0);
  config_.wheel_ratio = get_parameter("wheel_ratio").as_double();

  declare_parameter("motor_model_free_speed_rpm", 0.0);
  config_.motor_model_free_speed_rpm = get_parameter("motor_model_free_speed_rpm").as_double();

  declare_parameter("motor_speed_limit_rpm", 0.0);
  config_.motor_speed_limit_rpm = get_parameter("motor_speed_limit_rpm").as_double();

  declare_parameter("motor_model_stall_torque_nm", 0.0);
  config_.motor_model_stall_torque_nm = get_parameter("motor_model_stall_torque_nm").as_double();

  declare_parameter("motor_torque_limit_nm", 0.0);
  config_.motor_torque_limit_nm = get_parameter("motor_torque_limit_nm").as_double();

  declare_parameter("robot_mass", 0.0);
  config_.robot_mass = get_parameter("robot_mass").as_double();

  declare_parameter("robot_inertia", 0.0);
  config_.robot_inertia = get_parameter("robot_inertia").as_double();

  declare_parameter("steering_inertia", 0.0);
  config_.steering_inertia = get_parameter("steering_inertia").as_double();

  declare_parameter("wheel_constraint_tolerance", 0.0);
  config_.wheel_constraint_tolerance = get_parameter("wheel_constraint_tolerance").as_double();

  declare_parameter("state_normalization", std::vector<double>{});
  auto state_norms = get_parameter("state_normalization").as_double_array();

  declare_parameter("joint_normalization", std::vector<double>{});
  auto joint_norms = get_parameter("joint_normalization").as_double_array();

  norms_ = std::vector<double>{};
  norms_.insert(norms_.end(), state_norms.begin(), state_norms.end());
  norms_.insert(norms_.end(), joint_norms.begin(), joint_norms.end());
  norms_.insert(norms_.end(), joint_norms.begin(), joint_norms.end());
  norms_.insert(norms_.end(), joint_norms.begin(), joint_norms.end());
  norms_.insert(norms_.end(), joint_norms.begin(), joint_norms.end());

  // Weights
  declare_parameter("state_weights", std::vector<double>{});
  auto state_weights = get_parameter("state_weights").as_double_array();

  declare_parameter("joint_weights", std::vector<double>{});
  auto joint_weights = get_parameter("joint_weights").as_double_array();

  weights_ = std::vector<double>{};
  weights_.insert(weights_.end(), state_weights.begin(), state_weights.end());
  weights_.insert(weights_.end(), joint_weights.begin(), joint_weights.end());
  weights_.insert(weights_.end(), joint_weights.begin(), joint_weights.end());
  weights_.insert(weights_.end(), joint_weights.begin(), joint_weights.end());
  weights_.insert(weights_.end(), joint_weights.begin(), joint_weights.end());

  // Module Positions
  module_positions_ = std::vector<Eigen::Vector2d>{
    Eigen::Vector2d(config_.wheel_base_width / 2, config_.wheel_base_width / 2),
    Eigen::Vector2d(-config_.wheel_base_width / 2, config_.wheel_base_width / 2),
    Eigen::Vector2d(-config_.wheel_base_width / 2, -config_.wheel_base_width / 2),
    Eigen::Vector2d(config_.wheel_base_width / 2, -config_.wheel_base_width / 2)
  };

  num_knots_ = int(config_.time_horizon / config_.dt) + 1;
}

void SwerveMPCPlanner::validateConfig()
{
  std::unordered_map<std::string, double> double_params{
    {"time_horizon", config_.time_horizon},
    {"dt", config_.dt},
    {"chassis_width", config_.chassis_width},
    {"wheel_base_width", config_.wheel_base_width},
    {"wheel_width", config_.wheel_width},
    {"wheel_diam", config_.wheel_radius * 2.0},
    {"robot_mass", config_.robot_mass},
    {"robot_inertia", config_.robot_inertia},
    {"steering_inertia", config_.steering_inertia},
    {"wheel_ratio", config_.wheel_ratio},
    {"steering_ratio", config_.steering_ratio},
    {"wheel_constraint_tolerance", config_.wheel_constraint_tolerance},
    {"motor_model_free_speed_rpm", config_.motor_model_free_speed_rpm},
    {"motor_speed_limit_rpm", config_.motor_speed_limit_rpm},
    {"motor_model_stall_torque_nm", config_.motor_model_stall_torque_nm},
    {"motor_torque_limit_nm", config_.motor_torque_limit_nm}
  };

  for (const auto & [key, val] : double_params) {
    if (val <= 0) {
      std::string err_string =
        std::string("[SwerveMPCPlanner::validateConfig] Error: ") + key +
        " must be non-zero and positive!";
      throw std::runtime_error(err_string);
    }
  }

  std::unordered_map<std::string, int> int_params{
    {"num_swerve_modules", config_.num_swerve_modules}};

  for (const auto & [key, val] : int_params) {
    if (val <= 0) {
      std::string err_string =
        std::string("[SwerveMPCPlanner::validateConfig] Error: ") + key +
        " must be non-zero and positive!";
      throw std::runtime_error(err_string);
    }
  }
}

void SwerveMPCPlanner::initROS()
{
  intermediate_trajectory_publisher_ = create_publisher<ghost_msgs::msg::LabeledVectorMap>(
    "/trajectory/swerve_mpc_trajectory_intermediates", 10);

  trajectory_publisher_ = create_publisher<ghost_msgs::msg::LabeledVectorMap>(
    "/trajectory/swerve_mpc_trajectory", 10);

  ipopt_output_publisher_ = create_publisher<ghost_msgs::msg::IPOPTOutput>(
    "/ipopt_output", 10);

  if (publish_intermediate_solutions_) {
    if (!callback_data_buffer_) {
      throw std::runtime_error(
              "[SwerveMPCPlanner::initROS]  Error: callback_data_buffer is nullptr!");
    }
    if (!callback_data_mutex_) {
      throw std::runtime_error(
              "[SwerveMPCPlanner::initROS] Error: callback_data_mutex is nullptr!");
    }
    callback_thread_ = std::thread(
      [&]() {
        while (!(*exit_flag_ptr_)) {
          if (!callback_data_buffer_->empty()) {
            // Retrieve data from queue
            std::unique_lock<std::mutex> lock(*callback_data_mutex_);
            IterationCallback::IPOPTOutput data(callback_data_buffer_->back());
            callback_data_buffer_->pop_back();
            lock.unlock();

            // Publish IPOPT Output for visualization
            ghost_msgs::msg::IPOPTOutput ipopt_msg{};
            ghost_ros_interfaces::msg_helpers::toROSMsg(data, ipopt_msg);
            ipopt_output_publisher_->publish(ipopt_msg);

            // Publish Map of State Trajectories
            ghost_msgs::msg::LabeledVectorMap traj_map_msg{};
            ghost_ros_interfaces::msg_helpers::toROSMsg(
              generateTrajectoryMap(data.state_vector),
              traj_map_msg);
            intermediate_trajectory_publisher_->publish(traj_map_msg);
          }
          std::this_thread::sleep_for(5ms);
        }
      });
  }
}

void SwerveMPCPlanner::generateStateNames()
{
  // Populate state vector
  state_names_ = std::vector<std::string> {
    "base_pose_x",
    "base_pose_y",
    "base_pose_theta",
    "base_vel_x",
    "base_vel_y",
    "base_vel_theta",
    "base_accel_x",
    "base_accel_y",
    "base_accel_theta"};

  std::vector<std::string> joint_state_names = {
    "steering_angle",
    "steering_vel",
    "steering_accel",
    "wheel_vel",
    "wheel_torque",
    "lateral_force",
    "m1_velocity",
    "m2_velocity",
    "m1_torque",
    "m2_torque",
    "m1_voltage",
    "m2_voltage"
  };

  // Add joint states to state name vector
  for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
    std::string module_prefix = "m" + std::to_string(m) + "_";
    for (auto name : joint_state_names) {
      state_names_.push_back(module_prefix + name);
    }
  }

  // Set total number of optimization variables
  num_opt_vars_ = state_names_.size() * num_knots_;

  // Error Checking
  if (weights_.size() != state_names_.size()) {
    throw std::runtime_error(
            "[SwerveMPCPlanner::populateContainers] Error: weights must be the same length as the state vector!");
  }

  if (norms_.size() != state_names_.size()) {
    throw std::runtime_error(
            "[SwerveMPCPlanner::populateContainers] Error: norms must be the same length as the state vector!");
  }
}

void SwerveMPCPlanner::generateParameterNames()
{
  // Add initial state params
  param_names_ = {
    "pose_tracking_mode",
    "init_pose_x",
    "init_pose_y",
    "init_pose_theta",
    "init_vel_x",
    "init_vel_y",
    "init_vel_theta",
  };

  for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
    param_names_.push_back(std::string("init_m") + std::to_string(m) + "_steering_angle");
    param_names_.push_back(std::string("init_m") + std::to_string(m) + "_steering_vel");
    param_names_.push_back(std::string("init_m") + std::to_string(m) + "_wheel_vel");
  }

  for (int k = 0; k < num_knots_; k++) {
    std::string knot_prefix = getKnotPrefix(k);
    for (auto name : state_names_) {
      param_names_.push_back(knot_prefix + name + "_ref");
    }
  }
}

void SwerveMPCPlanner::populateContainers()
{
  cost_ = SX::zeros(1);
  lbx_ = DM::ones(num_opt_vars_) * -DM::inf();
  ubx_ = DM::ones(num_opt_vars_) * DM::inf();
  constraints_ = casadi::Matrix<casadi::SXElem>();
  lbg_ = DM::zeros(0);
  ubg_ = DM::zeros(0);
  latest_solution_ptr_ = std::make_shared<ghost_planners::Trajectory>(state_names_);

  // Populate time vector
  time_vector_ = std::vector<double>(num_knots_);
  for (int i = 0; i < num_knots_; i++) {
    time_vector_[i] = i * config_.dt;
  }

  // Generate optimization variables and populate state_vector and state_index_map
  state_vector_ = casadi::SX::zeros(num_opt_vars_);
  int state_index = 0;
  for (int i = 0; i < num_knots_; i++) {
    std::string knot_prefix = getKnotPrefix(i);
    for (auto name : state_names_) {
      state_index_map_[knot_prefix + name] = state_index;
      state_vector_(state_index) = SX::sym(knot_prefix + name);
      state_index++;
    }
  }

  // Generate optimization parameters and populate param_vector and param_index_map
  param_vector_ = casadi::SX::zeros(param_names_.size());
  int param_index = 0;
  for (auto & param_name : param_names_) {
    param_index_map_[param_name] = param_index;
    param_vector_(param_index) = SX::sym(param_name);
    param_index++;
  }

  int weight_index = 0;
  for (auto & name : state_names_) {
    weight_norm_index_map_[name] = weight_index;
    weight_index++;
  }
}

const Eigen::Vector2d & SwerveMPCPlanner::getModulePosition(int m) const
{
  if (m <= config_.num_swerve_modules) {
    return module_positions_[m - 1];
  } else {
    throw std::runtime_error(
            "[SwerveMPCPlanner::getModulePosition] Module index cannot exceed the number of swerve modules!");
  }
}

double SwerveMPCPlanner::getWeight(std::string name)
{
  if (weight_norm_index_map_.count(name) != 0) {
    return weights_[weight_norm_index_map_.at(name)];
  } else {
    throw std::runtime_error("[getWeight] No state with name " + name);
  }
}

double SwerveMPCPlanner::getNormalization(std::string name)
{
  if (weight_norm_index_map_.count(name) != 0) {
    return norms_[weight_norm_index_map_.at(name)];
  } else {
    throw std::runtime_error("[getNormalization] No state with name " + name);
  }
}

// Shorthand to get symbolic state variable by name
casadi::Matrix<casadi::SXElem> SwerveMPCPlanner::getState(std::string name, int k)
{
  auto state_name = "k" + std::to_string(k) + "_" + name;
  if (state_index_map_.count(state_name) != 0) {
    return state_vector_(state_index_map_.at(state_name));
  } else {
    throw std::runtime_error("[getState] No state with name " + state_name);
  }
}

// Shorthand to get symbolic state variable by name
casadi::Matrix<casadi::SXElem> SwerveMPCPlanner::getState(std::string name)
{
  if (state_index_map_.count(name) != 0) {
    return state_vector_(state_index_map_.at(name));
  } else {
    throw std::runtime_error("[getState] No state with name " + name);
  }
}

casadi::Matrix<casadi::SXElem> SwerveMPCPlanner::getParam(std::string name)
{
  if (param_index_map_.count(name) != 0) {
    return param_vector_(param_index_map_.at(name));
  } else {
    throw std::runtime_error("[getParam] No param with name " + name);
  }
}

casadi::Matrix<casadi::SXElem> SwerveMPCPlanner::getQuadraticTrackingCost(std::string state, int k)
{
  std::string kt1_ = "k" + std::to_string(k) + "_";
  std::string kt2_ = "k" + std::to_string(k + 1) + "_";

  auto s1 = getState(kt1_ + state);
  auto s2 = getState(kt2_ + state);

  auto r1 = getParam(kt1_ + state + "_ref");
  auto r2 = getParam(kt2_ + state + "_ref");
  double weight = getWeight(state);
  double norm = getNormalization(state);

  return weight * 0.5 * config_.dt * (pow((s1 - r1) / norm, 2) + pow((s2 - r2) / norm, 2));
}

casadi::Matrix<casadi::SXElem> SwerveMPCPlanner::getJerkCost(
  std::string state, int k,
  double weight)
{
  std::string kt1_ = "k" + std::to_string(k + 1) + "_";
  std::string kt2_ = "k" + std::to_string(k) + "_";

  auto s1 = getState(kt1_ + state);
  auto s2 = getState(kt2_ + state);
  auto jerk = (s1 - s2) / config_.dt;
  double norm = getNormalization(state) * config_.dt;

  return weight * pow(jerk / norm, 2);
}

std::unordered_map<std::string, std::vector<double>> SwerveMPCPlanner::generateTrajectoryMap(
  const std::vector<double> & solution_vector) const
{
  std::unordered_map<std::string, std::vector<double>> solution_map;

  for (auto & name : state_names_) {
    // Allocate timeseries vector for each state/input variable
    solution_map[name] = std::vector<double>(num_knots_);

    // Iterate through timeseries and add final state values to solution vector
    for (int k = 0; k < num_knots_; k++) {
      std::string knot_prefix = getKnotPrefix(k);
      solution_map[name][k] = solution_vector[state_index_map_.at(knot_prefix + name)];
    }
  }

  // Allocate timeseries vector for each state/input variable
  solution_map["time"] = std::vector<double>(num_knots_);

  // Iterate through timeseries and add final state values to solution vector
  for (int k = 0; k < num_knots_; k++) {
    solution_map["time"][k] = config_.dt * ((double) k);
  }
  return solution_map;
}

void SwerveMPCPlanner::addIntegrationConstraints()
{
  // List pairs of base state and derivative state
  std::vector<std::pair<std::string, std::string>> euler_integration_state_names = {
    std::pair<std::string, std::string>{"base_pose_x", "base_vel_x"},
    std::pair<std::string, std::string>{"base_pose_y", "base_vel_y"},
    std::pair<std::string, std::string>{"base_pose_theta", "base_vel_theta"},
    std::pair<std::string, std::string>{"base_vel_x", "base_accel_x"},
    std::pair<std::string, std::string>{"base_vel_y", "base_accel_y"},
    std::pair<std::string, std::string>{"base_vel_theta", "base_accel_theta"},
  };

  // Add joint states for each swerve module to the integration states pairs
  for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
    std::string module_prefix = "m" + std::to_string(m) + "_";
    euler_integration_state_names.push_back(
      std::pair<std::string,
      std::string>{module_prefix + "steering_angle",
        module_prefix + "steering_vel"});
    euler_integration_state_names.push_back(
      std::pair<std::string,
      std::string>{module_prefix + "steering_vel",
        module_prefix + "steering_accel"});
  }

  // Populate euler integration constraints for state vector
  for (int k = 0; k < num_knots_ - 1; k++) {
    std::string curr_knot_prefix = getKnotPrefix(k);
    std::string next_knot_prefix = getKnotPrefix(k + 1);

    for (auto & pair : euler_integration_state_names) {
      auto x0 = curr_knot_prefix + pair.first;
      auto x1 = next_knot_prefix + pair.first;
      auto dx0 = curr_knot_prefix + pair.second;
      auto dx1 = next_knot_prefix + pair.second;

      // X1 - X0 = 1/2 * DT * (dX1 + dX0)
      auto c = 2 * (getState(x1) - getState(x0)) / config_.dt - getState(dx1) - getState(dx0);
      constraints_ = vertcat(constraints_, c);
      lbg_ = vertcat(lbg_, DM(0));
      ubg_ = vertcat(ubg_, DM(0));

    }
  }
}

void SwerveMPCPlanner::addInitialStateConstraints()
{
  // Initial State Constraints
  std::vector<std::pair<std::string, std::string>> initial_state_constraint_param_pairs{
    std::pair<std::string, std::string>{"k0_base_pose_x", "init_pose_x"},
    std::pair<std::string, std::string>{"k0_base_pose_y", "init_pose_y"},
    std::pair<std::string, std::string>{"k0_base_pose_theta", "init_pose_theta"},
    std::pair<std::string, std::string>{"k0_base_vel_x", "init_vel_x"},
    std::pair<std::string, std::string>{"k0_base_vel_y", "init_vel_y"},
    std::pair<std::string, std::string>{"k0_base_vel_theta", "init_vel_theta"}
  };

  for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
    initial_state_constraint_param_pairs.push_back(
      std::pair<std::string, std::string>{
        std::string("k0_m") + std::to_string(m) + "_steering_angle",
        std::string("init_m") + std::to_string(m) + "_steering_angle",
      });
    initial_state_constraint_param_pairs.push_back(
      std::pair<std::string, std::string>{
        std::string("k0_m") + std::to_string(m) + "_steering_vel",
        std::string("init_m") + std::to_string(m) + "_steering_vel",
      });
    initial_state_constraint_param_pairs.push_back(
      std::pair<std::string, std::string>{
        std::string("k0_m") + std::to_string(m) + "_wheel_vel",
        std::string("init_m") + std::to_string(m) + "_wheel_vel",
      });
  }

  for (const auto & pair : initial_state_constraint_param_pairs) {
    auto c = getState(pair.first) - getParam(pair.second);
    constraints_ = vertcat(constraints_, c);
    lbg_ = vertcat(lbg_, DM(0));
    ubg_ = vertcat(ubg_, DM(0));
  }
}

void SwerveMPCPlanner::addAccelerationDynamicsConstraints()
{
  // acceleration dynamics constraints
  for (int k = 0; k < num_knots_; k++) {
    std::string kt1_ = getKnotPrefix(k);
    auto x_accel_constraint = config_.robot_mass * getState(kt1_ + "base_accel_x");
    auto y_accel_constraint = config_.robot_mass * getState(kt1_ + "base_accel_y");
    auto theta_accel_constraint = config_.robot_inertia * getState(kt1_ + "base_accel_theta");
    auto theta = getState(kt1_ + "base_pose_theta");

    for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
      std::string kt1_mN_ = kt1_ + "m" + std::to_string(m) + "_";
      auto wheel_force = getState(kt1_mN_ + "wheel_torque") / config_.wheel_radius;
      auto lateral_force = getState(kt1_mN_ + "lateral_force");
      auto world_steering_angle = theta + getState(kt1_mN_ + "steering_angle");

      auto x_force = casadi::Matrix<casadi::SXElem>();
      auto y_force = casadi::Matrix<casadi::SXElem>();
      rotateVector(world_steering_angle, wheel_force, lateral_force, x_force, y_force);
      x_accel_constraint -= x_force;
      y_accel_constraint -= y_force;

      auto mod_offset_x = casadi::Matrix<casadi::SXElem>();
      auto mod_offset_y = casadi::Matrix<casadi::SXElem>();
      rotateVector(
        theta, module_positions_[m - 1].x(),
        module_positions_[m - 1].y(), mod_offset_x, mod_offset_y);

      auto base_torque = y_force * (mod_offset_x) - x_force * (mod_offset_y);
      theta_accel_constraint -= base_torque;
    }

    constraints_ = vertcat(constraints_, x_accel_constraint);
    constraints_ = vertcat(constraints_, y_accel_constraint);
    constraints_ = vertcat(constraints_, theta_accel_constraint);

    lbg_ = vertcat(lbg_, DM::zeros(3));
    ubg_ = vertcat(ubg_, DM::zeros(3));
  }
}

void SwerveMPCPlanner::addNoWheelSlipConstraints()
{
  // acceleration dynamics constraints
  for (int k = 0; k < num_knots_; k++) {
    std::string kt1_ = getKnotPrefix(k);
    auto theta = getState(kt1_ + "base_pose_theta");
    auto vel_x = getState(kt1_ + "base_vel_x");
    auto vel_y = getState(kt1_ + "base_vel_y");
    auto vel_theta = getState(kt1_ + "base_vel_theta");

    for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
      std::string kt1_mN_ = kt1_ + "m" + std::to_string(m) + "_";
      auto world_steering_angle = theta + getState(kt1_mN_ + "steering_angle");

      auto mod_offset_x = casadi::Matrix<casadi::SXElem>();
      auto mod_offset_y = casadi::Matrix<casadi::SXElem>();
      rotateVector(
        theta, module_positions_[m - 1].x(),
        module_positions_[m - 1].y(), mod_offset_x, mod_offset_y);

      // Lateral Wheel Velocity Constraint
      auto base_steering_angle = getState(kt1_mN_ + "steering_angle");
      auto tan_vel = vel_theta * sqrt((pow(mod_offset_x, 2) + pow(mod_offset_y, 2)));
      auto r_angle = atan2(module_positions_[m - 1].y(), module_positions_[m - 1].x());                   // base_link
      auto tan_vel_x = tan_vel * -sin(r_angle);
      auto tan_vel_y = tan_vel * cos(r_angle);

      auto world_vel_x = casadi::Matrix<casadi::SXElem>();
      auto world_vel_y = casadi::Matrix<casadi::SXElem>();
      rotateVector(theta, tan_vel_x, tan_vel_y, world_vel_x, world_vel_y);
      world_vel_x += vel_x;
      world_vel_y += vel_y;

      auto forward_velocity = casadi::Matrix<casadi::SXElem>();
      auto lateral_velocity = casadi::Matrix<casadi::SXElem>();
      rotateVector(
        -world_steering_angle, world_vel_x, world_vel_y, forward_velocity,
        lateral_velocity);


      auto wheel_vel = getState(kt1_mN_ + "wheel_vel");
      constraints_ = vertcat(constraints_, lateral_velocity);
      constraints_ = vertcat(constraints_, forward_velocity - wheel_vel * config_.wheel_radius);

      lbg_ = vertcat(lbg_, -DM::ones(2) * config_.wheel_constraint_tolerance);
      ubg_ = vertcat(ubg_, DM::ones(2) * config_.wheel_constraint_tolerance);
    }
  }
}

void SwerveMPCPlanner::addDifferentialConstraints()
{
  double s_ratio = config_.steering_ratio;
  double w_ratio = config_.wheel_ratio;
  for (int k = 0; k < num_knots_; k++) {
    std::string kt1_ = getKnotPrefix(k);
    for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
      // Velocity Constraints
      std::string kt1_mN_ = kt1_ + "m" + std::to_string(m) + "_";
      auto steering_vel = getState(kt1_mN_ + "steering_vel");
      auto wheel_vel = getState(kt1_mN_ + "wheel_vel");
      auto m1_velocity_rad = getState(kt1_mN_ + "m1_velocity") * ghost_util::RPM_TO_RAD_PER_SEC;
      auto m2_velocity_rad = getState(kt1_mN_ + "m2_velocity") * ghost_util::RPM_TO_RAD_PER_SEC;

      auto c1 = 0.5 * (m1_velocity_rad - m2_velocity_rad) * w_ratio - wheel_vel;
      auto c2 = 0.5 * (m1_velocity_rad + m2_velocity_rad) * s_ratio - steering_vel;

      constraints_ = vertcat(constraints_, c1);
      constraints_ = vertcat(constraints_, c2);

      lbg_ = vertcat(lbg_, DM::zeros(2));
      ubg_ = vertcat(ubg_, DM::zeros(2));

      // Torque Constraints
      auto m1_torque = getState(kt1_mN_ + "m1_torque");
      auto m2_torque = getState(kt1_mN_ + "m2_torque");
      auto wheel_torque = getState(kt1_mN_ + "wheel_torque");
      auto steering_torque = config_.steering_inertia * getState(kt1_mN_ + "steering_accel");

      auto c3 = wheel_torque / w_ratio + steering_torque / s_ratio - m1_torque;
      auto c4 = -wheel_torque / w_ratio + steering_torque / s_ratio - m2_torque;

      constraints_ = vertcat(constraints_, c3);
      constraints_ = vertcat(constraints_, c4);

      lbg_ = vertcat(lbg_, DM::zeros(2));
      ubg_ = vertcat(ubg_, DM::zeros(2));
    }
  }

}

void SwerveMPCPlanner::addMotorModelConstraints()
{
  double t_stall = config_.motor_model_stall_torque_nm;
  double w_free = config_.motor_model_free_speed_rpm;
  for (int k = 0; k < num_knots_; k++) {
    std::string kt1_ = getKnotPrefix(k);
    for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
      // Velocity Constraints
      std::string kt1_mN_ = kt1_ + "m" + std::to_string(m) + "_";
      auto m1_velocity = getState(kt1_mN_ + "m1_velocity");
      auto m2_velocity = getState(kt1_mN_ + "m2_velocity");
      auto m1_voltage = getState(kt1_mN_ + "m1_voltage");
      auto m2_voltage = getState(kt1_mN_ + "m2_voltage");
      auto m1_torque = getState(kt1_mN_ + "m1_torque");
      auto m2_torque = getState(kt1_mN_ + "m2_torque");

      auto c1 = t_stall * (m1_voltage / 12000.0 - m1_velocity / w_free) - m1_torque;
      auto c2 = t_stall * (m2_voltage / 12000.0 - m2_velocity / w_free) - m2_torque;

      constraints_ = vertcat(constraints_, c1);
      constraints_ = vertcat(constraints_, c2);

      lbg_ = vertcat(lbg_, DM::zeros(2));
      ubg_ = vertcat(ubg_, DM::zeros(2));
    }
  }
}

void SwerveMPCPlanner::addConstraints()
{
  addIntegrationConstraints();
  addInitialStateConstraints();
  addAccelerationDynamicsConstraints();
  addNoWheelSlipConstraints();
  addDifferentialConstraints();
  addMotorModelConstraints();

  if (lbg_.size1() != ubg_.size1()) {
    throw std::runtime_error(
            "[SwerveMPCPlanner::addConstraints] Error: lbg and ubg are not the same size!");
  }

  if (lbg_.size1() != constraints_.size1()) {
    throw std::runtime_error(
            "[SwerveMPCPlanner::addConstraints] Error: constraint bounds and constraint vector are not the same size!");
  }
}

void SwerveMPCPlanner::setStateBounds()
{
  // Optimization Variables Limits
  lbx_ = DM::ones(num_opt_vars_) * -DM::inf();
  ubx_ = DM::ones(num_opt_vars_) * DM::inf();
  double translation_speed_limit = 3.0;       // 1.389
  double translation_accel_limit = 15.0;       // 5.5755

  for (int k = 0; k < num_knots_; k++) {
    std::string curr_knot_prefix = getKnotPrefix(k);
    lbx_(state_index_map_[curr_knot_prefix + "base_vel_x"]) = -translation_speed_limit;
    ubx_(state_index_map_[curr_knot_prefix + "base_vel_x"]) = translation_speed_limit;
    lbx_(state_index_map_[curr_knot_prefix + "base_vel_y"]) = -translation_speed_limit;
    ubx_(state_index_map_[curr_knot_prefix + "base_vel_y"]) = translation_speed_limit;
    lbx_(state_index_map_[curr_knot_prefix + "base_vel_theta"]) = -7.0305;
    ubx_(state_index_map_[curr_knot_prefix + "base_vel_theta"]) = 7.0305;

    lbx_(state_index_map_[curr_knot_prefix + "base_accel_x"]) = -translation_accel_limit;
    ubx_(state_index_map_[curr_knot_prefix + "base_accel_x"]) = translation_accel_limit;
    lbx_(state_index_map_[curr_knot_prefix + "base_accel_y"]) = -translation_accel_limit;
    ubx_(state_index_map_[curr_knot_prefix + "base_accel_y"]) = translation_accel_limit;
    lbx_(state_index_map_[curr_knot_prefix + "base_accel_theta"]) = -45.97811;
    ubx_(state_index_map_[curr_knot_prefix + "base_accel_theta"]) = 45.97811;

    for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
      std::string module_prefix = curr_knot_prefix + "m" + std::to_string(m) + "_";

      lbx_(state_index_map_[module_prefix + "lateral_force"]) = -config_.robot_mass * 9.81 * 1.25;
      ubx_(state_index_map_[module_prefix + "lateral_force"]) = config_.robot_mass * 9.81 * 1.25;

      lbx_(state_index_map_[module_prefix + "m1_velocity"]) = -config_.motor_speed_limit_rpm;
      ubx_(state_index_map_[module_prefix + "m1_velocity"]) = config_.motor_speed_limit_rpm;

      lbx_(state_index_map_[module_prefix + "m2_velocity"]) = -config_.motor_speed_limit_rpm;
      ubx_(state_index_map_[module_prefix + "m2_velocity"]) = config_.motor_speed_limit_rpm;

      lbx_(state_index_map_[module_prefix + "m1_torque"]) = -config_.motor_torque_limit_nm;
      ubx_(state_index_map_[module_prefix + "m1_torque"]) = config_.motor_torque_limit_nm;

      lbx_(state_index_map_[module_prefix + "m2_torque"]) = -config_.motor_torque_limit_nm;
      ubx_(state_index_map_[module_prefix + "m2_torque"]) = config_.motor_torque_limit_nm;

      lbx_(state_index_map_[module_prefix + "m1_voltage"]) = -12000.0;
      ubx_(state_index_map_[module_prefix + "m1_voltage"]) = 12000.0;

      lbx_(state_index_map_[module_prefix + "m2_voltage"]) = -12000.0;
      ubx_(state_index_map_[module_prefix + "m2_voltage"]) = 12000.0;
    }
  }
}

void SwerveMPCPlanner::addStateTrackingCosts()
{
  // Apply Quadratic costs
  for (int k = 0; k < num_knots_ - 1; k++) {
    cost_ += getParam("pose_tracking_mode") * getQuadraticTrackingCost("base_pose_x", k);
    cost_ += getParam("pose_tracking_mode") * getQuadraticTrackingCost("base_pose_y", k);
    cost_ += getParam("pose_tracking_mode") * getQuadraticTrackingCost("base_pose_theta", k);

    cost_ += getQuadraticTrackingCost("base_vel_x", k);
    cost_ += getQuadraticTrackingCost("base_vel_y", k);
    cost_ += getQuadraticTrackingCost("base_vel_theta", k);

    cost_ += getQuadraticTrackingCost("base_accel_x", k);
    cost_ += getQuadraticTrackingCost("base_accel_y", k);
    cost_ += getQuadraticTrackingCost("base_accel_theta", k);

    for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
      std::string mN_ = "m" + std::to_string(m) + "_";

      cost_ += getQuadraticTrackingCost(mN_ + "lateral_force", k);
      cost_ += getQuadraticTrackingCost(mN_ + "steering_vel", k);

      // Motor States
      cost_ += getQuadraticTrackingCost(mN_ + "m1_velocity", k);
      cost_ += getQuadraticTrackingCost(mN_ + "m2_velocity", k);
      cost_ += getQuadraticTrackingCost(mN_ + "m1_torque", k);
      cost_ += getQuadraticTrackingCost(mN_ + "m2_torque", k);
      cost_ += getQuadraticTrackingCost(mN_ + "m1_voltage", k);
      cost_ += getQuadraticTrackingCost(mN_ + "m2_voltage", k);
    }
  }
}

void SwerveMPCPlanner::addJerkCosts()
{
  for (int k = 0; k < num_knots_ - 1; k++) {
    cost_ += getJerkCost("base_accel_x", k, 0.01);
    cost_ += getJerkCost("base_accel_y", k, 0.01);
    cost_ += getJerkCost("base_accel_theta", k, 0.01);

    for (int m = 1; m < config_.num_swerve_modules + 1; m++) {
      std::string mN_ = "m" + std::to_string(m) + "_";
      cost_ += getJerkCost(mN_ + "lateral_force", k, 0.001);
      cost_ += getJerkCost(mN_ + "wheel_torque", k, 0.01);
    }
  }
}


void SwerveMPCPlanner::addCosts()
{
  addStateTrackingCosts();
  addJerkCosts();
}

void SwerveMPCPlanner::initSolver()
{
  callback_data_buffer_ = std::make_shared<std::deque<IterationCallback::IPOPTOutput>>();
  callback_data_mutex_ = std::make_shared<std::mutex>();
  exit_flag_ptr_ = std::make_shared<std::atomic_bool>(false);

  iteration_callback_ = std::make_shared<IterationCallback>(
    num_opt_vars_,
    constraints_.size1(),
    callback_data_buffer_,
    callback_data_mutex_,
    exit_flag_ptr_);

  Dict solver_config{
    {"verbose", false},
    {"ipopt.print_level", ipopt_verbosity_},
    {"iteration_callback", *iteration_callback_},
    {"ipopt.max_iter", max_iterations_}
  };

  SXDict nlp_config{
    {"x", getStateVector()},
    {"f", getCost()},
    {"g", getConstraints()},
    {"p", getParamVector()}};

  solver_args_["lbx"] = getStateLowerBounds();
  solver_args_["ubx"] = getStateUpperBounds();
  solver_args_["lbg"] = getConstraintLowerBounds();
  solver_args_["ubg"] = getConstraintUpperBounds();

  solver_ = nlpsol("swerve_mpc_planner", "ipopt", nlp_config, solver_config);
}

DM SwerveMPCPlanner::convertVectorToDM(std::vector<double> vector)
{
  DM dm = DM::zeros(vector.size());
  for (int i = 0; i < vector.size(); i++) {
    dm(i) = vector[i];
  }
  return dm;
}

std::string SwerveMPCPlanner::getKnotPrefix(int i)
{
  return "k" + std::to_string(i) + "_";
}

void SwerveMPCPlanner::shiftTimeVectorToPresent(double current_time)
{
  double offset = current_time - time_vector_[0];
  for (int i = 0; i < time_vector_.size(); i++) {
    time_vector_[i] += offset;
  }
}

void SwerveMPCPlanner::updateInitialSolution(const ghost_planners::Trajectory & x0)
{
  auto x0_flat = x0.getFlattenedTrajectory(time_vector_);

  if (x0_flat.size() != getNumOptVars()) {
    throw std::runtime_error(
            "[SwerveMPCPlanner::runSolver] Error: x0 size must match number of optimization variables!");

    solver_args_["x0"] = convertVectorToDM(x0_flat);

  }
}

void SwerveMPCPlanner::updateReferenceTrajectory(
  const std::vector<double> & current_state,
  const Trajectory & reference_trajectory,
  bool track_pose)
{
  auto ref_flat = reference_trajectory.getFlattenedTrajectory(time_vector_);


  if (ref_flat.size() != getNumOptVars()) {
    throw std::runtime_error(
            "[SwerveMPCPlanner::runSolver] Error: reference trajectory size must match number of optimization variables!");

  }

  DM pose_tracking = (track_pose) ? DM(1.0) : DM(0.0);
  auto initial_state_params = convertVectorToDM(current_state);
  auto reference_trajectory_params = convertVectorToDM(ref_flat);

  solver_args_["p"] = vertcat(pose_tracking, initial_state_params, reference_trajectory_params);
}

void SwerveMPCPlanner::convertSolutionToTrajectory()
{

  std::unique_lock<std::mutex> lock(latest_solution_mutex_);
  latest_solution_ptr_->clearNodes();
  for (int k = 0; k < num_knots_; k++) {
    double time = time_vector_[k];
    std::vector<double> Node(
      latest_solution_vector_.begin() + k * state_names_.size(),
      latest_solution_vector_.begin() + (k + 1) * state_names_.size());
    latest_solution_ptr_->addNode(time, Node);
  }
}

void SwerveMPCPlanner::publishStateTrajectoryMap() const
{
  ghost_msgs::msg::LabeledVectorMap traj_map_msg{};
  ghost_ros_interfaces::msg_helpers::toROSMsg(
    generateTrajectoryMap(latest_solution_vector_),
    traj_map_msg);
  trajectory_publisher_->publish(traj_map_msg);
}

void SwerveMPCPlanner::runSolver(
  double current_time,
  const std::vector<double> & current_state,
  const Trajectory & x0,
  const Trajectory & reference_trajectory,
  bool track_pose)
{
  solver_active_ = true;

  shiftTimeVectorToPresent(current_time);

  updateInitialSolution(x0);
  updateReferenceTrajectory(current_state, reference_trajectory, track_pose);

  std::map<std::string, DM> res = solver_(solver_args_);
  latest_solution_vector_ = std::vector<double>(res.at("x"));

  convertSolutionToTrajectory();
  publishStateTrajectoryMap();

  solver_active_ = false;
}

} // namespace ghost_swerve_mpc_planner