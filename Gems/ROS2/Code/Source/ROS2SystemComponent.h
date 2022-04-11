/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <ROS2/ROS2Bus.h>

#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "builtin_interfaces/msg/time.hpp"
#include "Clock/SimulationClock.h"

namespace ROS2
{
    class ROS2SystemComponent
        : public AZ::Component
        , protected ROS2RequestBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(ROS2SystemComponent, "{cb28d486-afa4-4a9f-a237-ac5eb42e1c87}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ROS2SystemComponent();
        ~ROS2SystemComponent();

        std::shared_ptr<rclcpp::Node> GetNode() const override;
        builtin_interfaces::msg::Time GetROSTimestamp() const override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ROS2RequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZTickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////
    
    private:
        std::shared_ptr<rclcpp::Node> m_ros2Node;
        AZStd::shared_ptr<rclcpp::executors::SingleThreadedExecutor> m_executor;
        SimulationClock m_simulationClock;
    };

} // namespace ROS2
