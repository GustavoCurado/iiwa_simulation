cmake_minimum_required(VERSION 3.8)
project(iiwa_simulation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(ros2_control REQUIRED)
find_package(ros2_controllers REQUIRED)
find_package(ros_gz_bridge REQUIRED)
find_package(ros_gz_sim REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(iiwa_player_open src/iiwa_player_open.cpp)
ament_target_dependencies(iiwa_player_open rclcpp ros2_control ros2_controllers ros_gz_bridge ros_gz_sim std_msgs)

add_executable(iiwa_player_closed src/iiwa_player_closed.cpp)
ament_target_dependencies(iiwa_player_closed rclcpp ros2_control ros2_controllers ros_gz_bridge ros_gz_sim std_msgs)

install(TARGETS
  iiwa_player_open iiwa_player_closed
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY
  launch urdf meshes
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
