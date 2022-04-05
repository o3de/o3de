/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SimulationClock.h"
#include <ROS2/ROS2Bus.h>
#include <rclcpp/qos.hpp>

namespace ROS2
{
    builtin_interfaces::msg::Time SimulationClock::GetROSTimestamp() const
    {
        auto elapsedTime = GetElapsedTimeMicroseconds();

        builtin_interfaces::msg::Time timeStamp;
        timeStamp.sec = static_cast<int32_t>(elapsedTime / 1000000);
        timeStamp.nanosec = static_cast<uint32_t>((elapsedTime % 1000000) * 1000);
        return timeStamp;
    }

    int64_t SimulationClock::GetElapsedTimeMicroseconds() const
    {
        if (auto* timeSystem = AZ::Interface<AZ::ITime>::Get())
        {
            return static_cast<int64_t>(timeSystem->GetElapsedTimeUs());
        }
        else
        {
            AZ_Warning("ROS2SystemComponent", false, "No ITime interface available");
            return 0;
        }
    }

    void SimulationClock::Tick()
    {
        if (!m_clockPublisher)
        {   //Lazy construct
            auto ros2Node = ROS2Interface::Get()->GetNode();

            // Standard QoS for /clock topic is best_effort, keep_last 1
            rclcpp::QoS qos(1);
            qos.best_effort();
            m_clockPublisher = ros2Node->create_publisher<rosgraph_msgs::msg::Clock>("/clock", qos);
        }

        rosgraph_msgs::msg::Clock msg;
        msg.clock = GetROSTimestamp();
        m_clockPublisher->publish(msg);
    }
} // namespace ROS2