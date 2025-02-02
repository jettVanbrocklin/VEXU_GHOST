import os
import xacro
from ament_index_python.packages import get_package_share_directory

import launch
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

# Opaque Function hack to allow for better CLI arg parsing
def launch_setup(context, *args, **kwargs):
    ghost_sim_share_dir = get_package_share_directory('ghost_sim_examples')
    filename = "tankbot.xacro"

    # Load URDF and process to text
    urdf_path = os.path.join(ghost_sim_share_dir, "urdf", filename)
    doc = xacro.process(urdf_path)
    
    spawn_entity_args = ("-x 0.0 -y 0.0 -z 1.0 -R 0.0 -P 0.0 -Y 0.0 -entity tankbot -topic robot_description").split()

    # Node to spawn robot model in Gazebo
    gazebo_ros = Node(
        package = "gazebo_ros",
        executable = "spawn_entity.py",
        output='screen',
        arguments=spawn_entity_args)

    # Node to publish robot joint transforms
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{'use_sim_time': False}, {"robot_description": doc}])

    return [gazebo_ros, robot_state_publisher]


def generate_launch_description():
    # Load relevant filepaths
    gazebo_ros_share_dir = get_package_share_directory('gazebo_ros')
    ghost_sim_share_dir = get_package_share_directory('ghost_sim_examples')
    home_dir = os.path.expanduser('~')
    world_file = os.path.join(ghost_sim_share_dir, "world", "default.world")

    # Simulator (Doesn't launch Simulator GUI by default, use CLI Arg "sim_gui" for debugging)
    simulation = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(gazebo_ros_share_dir,
                         'launch', 'gazebo.launch.py')
        ),
        launch_arguments={
            'world': world_file,
            'gui': LaunchConfiguration("sim_gui"),
            'verbose': LaunchConfiguration('verbose'),
            }.items()
    )

    return LaunchDescription([
        DeclareLaunchArgument(name='use_joy', default_value='true'),
        DeclareLaunchArgument(name='channel_id', default_value='1'),
        DeclareLaunchArgument('sim_gui', default_value='true'),
        DeclareLaunchArgument('verbose', default_value='true'),
        simulation,
        OpaqueFunction(function = launch_setup)
    ])