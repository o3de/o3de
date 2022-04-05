/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <memory>
#include <AzCore/Time/ITime.h>
#include <builtin_interfaces/msg/time.hpp>
#include <rosgraph_msgs/msg/clock.hpp>
#include <rclcpp/publisher.hpp>

namespace ROS2
{
    class SimulationClock
    {
    public:
        builtin_interfaces::msg::Time GetROSTimestamp() const;

        // TODO - consider having it called internally, also perhaps in a thread with a given frequency
        void Tick();

    private:
        // Time since start of sim, scaled with t_simulationTickScale
        int64_t GetElapsedTimeMicroseconds() const;

        rclcpp::Publisher<rosgraph_msgs::msg::Clock>::SharedPtr m_clockPublisher;
    };
} // namespace ROS2