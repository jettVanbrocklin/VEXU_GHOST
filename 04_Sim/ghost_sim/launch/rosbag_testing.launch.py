import os
import xacro
from ament_index_python.packages import get_package_share_directory

import launch
from launch import LaunchDescription
from launch.actions import (
    IncludeLaunchDescription,
    DeclareLaunchArgument,
    OpaqueFunction,
)
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


# Opaque Function hack to allow for better CLI arg parsing
def launch_setup(context, *args, **kwargs):
    ghost_sim_share_dir = get_package_share_directory("ghost_sim")
    filename = "test_tank_init.xacro"

    # Load XACRO and process to urdf then to text
    xacro_path = os.path.join(ghost_sim_share_dir, "urdf", filename)
    xml = xacro.process_file(xacro_path)
    doc = xml.toprettyxml(indent="  ")

    # Node to publish robot joint transforms
    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        name="robot_state_publisher",
        output="screen",
        parameters=[{"use_sim_time": True}, {"robot_description": doc}],
    )

    # Joystick (Only launched if joystick CLI arg is set to True)
    joy_launch_description = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ghost_sim_share_dir, "launch", "joystick.launch.py")
        ),
        condition=launch.conditions.IfCondition(LaunchConfiguration("use_joy")),
        launch_arguments={
            "channel_id": LaunchConfiguration("channel_id"),
        }.items(),
    )

    return [robot_state_publisher, joy_launch_description]


def generate_launch_description():
    # Load relevant filepaths
    gazebo_ros_share_dir = get_package_share_directory("gazebo_ros")
    ghost_ros_share_dir = get_package_share_directory("ghost_ros_interfaces")
    ghost_sim_share_dir = get_package_share_directory("ghost_sim")
    ghost_localization_share_dir = get_package_share_directory("ghost_localization")

    home_dir = os.path.expanduser("~")
    ghost_ros_base_dir = os.path.join(
        home_dir, "VEXU_GHOST", "03_ROS", "ghost_ros_interfaces"
    )

    world_file = os.path.join(ghost_sim_share_dir, "worlds", "spin_up.world")
    rviz_config_path = os.path.join(ghost_localization_share_dir, "rviz/ekf_pf.rviz")

    # Simulator (Doesn't launch Simulator GUI by default, use CLI Arg "sim_gui" for debugging)
    simulation = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(gazebo_ros_share_dir, "launch", "gazebo.launch.py")
        ),
        launch_arguments={
            "world": world_file,
            "gui": LaunchConfiguration("sim_gui"),
            "verbose": LaunchConfiguration("verbose"),
        }.items(),
    )

    ekf_pf_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ghost_localization_share_dir, "launch", "ekf_pf.launch.py")
        )
    )

    pf_ekf_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ghost_localization_share_dir, "launch", "pf_ekf.launch.py")
        )
    )

    # Launch RVIZ Display as primary GUI interface
    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        arguments=["-d", rviz_config_path],
    )

    # robot_localization_node = Node(
    #     package='robot_localization',
    #     executable='ekf_node',
    #     name='ekf_localization_node',
    #     output='screen',
    #     parameters=[ghost_ros_base_dir + "/config/robot_localization_config.yaml"]
    # )

    pf_ekf_localization_node = Node(
        package="robot_localization",
        executable="ekf_node",
        name="pf_ekf_localization_node",
        output="screen",
        parameters=[ghost_ros_base_dir + "/config/pf_ekf_localization_config.yaml"],
    )

    plot_juggler_node = Node(
        package="plotjuggler", executable="plotjuggler", name="plot_juggler"
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(name="use_joy", default_value="false"),
            DeclareLaunchArgument(name="channel_id", default_value="1"),
            DeclareLaunchArgument("sim_gui", default_value="false"),
            DeclareLaunchArgument("verbose", default_value="true"),
            # simulation,
            # ekf_pf_launch,
            pf_ekf_launch,
            rviz_node,
            plot_juggler_node,
            # robot_localization_node,
            pf_ekf_localization_node,
            # state_machine_node,
            OpaqueFunction(function=launch_setup),
        ]
    )
