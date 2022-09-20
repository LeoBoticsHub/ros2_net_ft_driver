#include "net_ft_driver/hardware_interface.hpp"

#include <memory>
#include <string>
#include <vector>

#include "hardware_interface/sensor_interface.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "net_ft_driver/interfaces/net_ft_interface.hpp"
#include "rclcpp/rclcpp.hpp"

const auto kLogger = rclcpp::get_logger("NetFtHardwareInerface");

namespace net_ft_driver
{
NetFtHardwareInterface::NetFtHardwareInterface() {}

CallbackReturn NetFtHardwareInterface::on_init(const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::SensorInterface::on_init(info) != CallbackReturn::SUCCESS)
  {
    return CallbackReturn::ERROR;
  }

  ft_sensor_measurements_ = {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};
  lost_packets_ = 0;
  packet_count_ = 0;
  out_of_order_count_ = 0;
  status_ = 0;

  ip_address_ = info_.hardware_parameters["ip_address"];
  sensor_type_ = info_.hardware_parameters["sensor_type"];
  int rdt_rate = std::stoi(info_.hardware_parameters["rdt_sampling_rate"]);

  driver_ = NetFTInterface::create(sensor_type_, ip_address_);

  if (!driver_->set_sampling_rate(rdt_rate))
  {
    RCLCPP_FATAL(kLogger, "Couldn't set RDT sampling rate of the F/T Sensor");
    return CallbackReturn::ERROR;
  }

  RCLCPP_INFO(kLogger, "Initialize connection with F/T Sensor");
  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> NetFtHardwareInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;

  for (auto & sensor : info_.sensors)
  {
    for (size_t j = 0; j < sensor.state_interfaces.size(); ++j)
    {
      state_interfaces.emplace_back(
        hardware_interface::StateInterface(sensor.name, sensor.state_interfaces[j].name, &ft_sensor_measurements_[j]));
    }
  }

  state_interfaces.emplace_back(hardware_interface::StateInterface("diagnostic", "packet_count", &packet_count_));
  state_interfaces.emplace_back(hardware_interface::StateInterface("diagnostic", "lost_packets", &lost_packets_));
  state_interfaces.emplace_back(
    hardware_interface::StateInterface("diagnostic", "out_of_order_count", &out_of_order_count_));
  state_interfaces.emplace_back(hardware_interface::StateInterface("diagnostic", "status", &status_));
  return state_interfaces;
}

CallbackReturn NetFtHardwareInterface::on_activate(const rclcpp_lifecycle::State & /*previous_state*/)
{
  if (driver_->start_streaming())
  {
    std::unique_ptr<SensorData> data = driver_->receive_data();
    if (data)
    {
      offset_ft_values_ = data->ft_values;
      ft_sensor_measurements_ = apply_offset(data->ft_values);

      RCLCPP_INFO(kLogger, "Successfully started data streaming!");
      return CallbackReturn::SUCCESS;
    }
  }
  RCLCPP_FATAL(kLogger, "The data stream could not be started!");
  return CallbackReturn::ERROR;
}

CallbackReturn NetFtHardwareInterface::on_deactivate(const rclcpp_lifecycle::State & /*previous_state*/)
{
  if (driver_->stop_streaming())
  {
    RCLCPP_INFO(kLogger, "Successfully stoped data streaming!");
    return CallbackReturn::SUCCESS;
  }
  RCLCPP_FATAL(kLogger, "The data stream could not be stopped!");
  return CallbackReturn::ERROR;
}

hardware_interface::return_type NetFtHardwareInterface::read()
{
  auto data = driver_->receive_data();
  if (data)
  {
    ft_sensor_measurements_ = apply_offset(data->ft_values);
    lost_packets_ = static_cast<double>(data->lost_packets);
    packet_count_ = static_cast<double>(data->packet_count);
    out_of_order_count_ = static_cast<double>(data->out_of_order_count);
    status_ = static_cast<double>(data->status);
  }
  return hardware_interface::return_type::OK;
}

Vector6D NetFtHardwareInterface::apply_offset(Vector6D ft_values)
{
  Vector6D offseted_values;
  for (size_t i = 0; i < ft_values.size(); i++)
  {
    offseted_values[i] = ft_values[i] - offset_ft_values_[i];
  }
  return offseted_values;
}
}  // namespace net_ft_driver

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(net_ft_driver::NetFtHardwareInterface, hardware_interface::SensorInterface)
