import launch
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    single_sensor_launch_path = PathJoinSubstitution(
        [FindPackageShare("net_ft_driver"), "launch", "net_ft_broadcaster_namespace.launch.py"]
    )

    sensor_1 = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(single_sensor_launch_path),
        launch_arguments={
            "namespace": "sensor_1",
            "ip_address": "192.168.0.211",
            "controller_yaml": "net_ft_broadcaster_sensor_1.yaml",
        }.items(),
    )

    sensor_2 = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(single_sensor_launch_path),
        launch_arguments={
            "namespace": "sensor_2",
            "ip_address": "192.168.0.212",
            "controller_yaml": "net_ft_broadcaster_sensor_2.yaml",
        }.items(),
    )

    return launch.LaunchDescription([sensor_1, sensor_2])
