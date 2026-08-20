import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, RegisterEventHandler, SetEnvironmentVariable, TimerAction
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch.conditions import IfCondition
from launch_ros.actions import Node
from launch.event_handlers import OnProcessExit

def generate_launch_description():
    pkg_share = get_package_share_directory('iiwa_simulation')
    workspace_share_dir = os.path.abspath(os.path.join(pkg_share, '..'))

    configurar_recursos_gz = SetEnvironmentVariable(
        name='GZ_SIM_RESOURCE_PATH',
        value=workspace_share_dir
    )

    # Argumento atualizado para inglês
    controller_arg = DeclareLaunchArgument(
        'controller',
        default_value='closed',
        description='Choose the controller: "open" or "closed"'
    )

    iniciar_gazebo = ExecuteProcess(
        cmd=['ign', 'gazebo', 'zero_gravity.sdf'],
        output='screen'
    )

    caminho_urdf = os.path.join(pkg_share, 'urdf', 'iiwa_14_sim.urdf')
    spawn_robo = TimerAction(
        period=2.0,
        actions=[
            Node(
                package='ros_gz_sim',
                executable='create',
                arguments=['-file', caminho_urdf, '-name', 'iiwa14'],
                output='screen'
            )
        ]
    )

    despausar_simulacao = TimerAction(
        period=4.0,
        actions=[
            ExecuteProcess(
                cmd=[
                    'ign', 'service', '-s', '/world/zero_gravity_world/control',
                    '--reqtype', 'ignition.msgs.WorldControl',
                    '--reptype', 'ignition.msgs.Boolean',
                    '--req', 'pause: false',
                    '--timeout', '2000'
                ],
                output='screen'
            )
        ]
    )

    argumentos_ponte = [
        '/model/iiwa14/joint/iiwa_joint_1/cmd_force@std_msgs/msg/Float64]ignition.msgs.Double',
        '/model/iiwa14/joint/iiwa_joint_2/cmd_force@std_msgs/msg/Float64]ignition.msgs.Double',
        '/model/iiwa14/joint/iiwa_joint_3/cmd_force@std_msgs/msg/Float64]ignition.msgs.Double',
        '/model/iiwa14/joint/iiwa_joint_4/cmd_force@std_msgs/msg/Float64]ignition.msgs.Double',
        '/model/iiwa14/joint/iiwa_joint_5/cmd_force@std_msgs/msg/Float64]ignition.msgs.Double',
        '/model/iiwa14/joint/iiwa_joint_6/cmd_force@std_msgs/msg/Float64]ignition.msgs.Double',
        '/model/iiwa14/joint/iiwa_joint_7/cmd_force@std_msgs/msg/Float64]ignition.msgs.Double',
        '/world/zero_gravity_world/model/iiwa14/joint_state@sensor_msgs/msg/JointState[ignition.msgs.Model'
    ]
    
    iniciar_ponte = TimerAction(
        period=4.0,
        actions=[
            Node(
                package='ros_gz_bridge',
                executable='parameter_bridge',
                arguments=argumentos_ponte,
                output='screen'
            )
        ]
    )

    # Condições de execução atualizadas para inglês
    no_malha_aberta = TimerAction(
        period=6.0,
        actions=[
            Node(
                package='iiwa_simulation',
                executable='iiwa_player_open',
                output='screen',
                condition=IfCondition(PythonExpression(["'", LaunchConfiguration('controller'), "' == 'open'"]))
            )
        ]
    )

    no_malha_fechada = TimerAction(
        period=6.0,
        actions=[
            Node(
                package='iiwa_simulation',
                executable='iiwa_player_closed',
                output='screen',
                condition=IfCondition(PythonExpression(["'", LaunchConfiguration('controller'), "' == 'closed'"]))
            )
        ]
    )

    abrir_grafico_fechada = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=no_malha_fechada.actions[0], 
            on_exit=[ExecuteProcess(cmd=['python3', 'plot_trajetoria.py'], output='screen')]
        )
    )

    abrir_grafico_aberta = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=no_malha_aberta.actions[0],
            on_exit=[ExecuteProcess(cmd=['python3', 'plot_trajetoria.py'], output='screen')]
        )
    )

    return LaunchDescription([
        configurar_recursos_gz,
        controller_arg,
        iniciar_gazebo,
        spawn_robo,
        despausar_simulacao,
        iniciar_ponte,
        no_malha_aberta,
        no_malha_fechada,
        abrir_grafico_fechada,
        abrir_grafico_aberta
    ])
