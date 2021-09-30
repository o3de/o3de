/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>

namespace AZ
{
    class TaskGraphSystemComponent
        : public Component
        , public TaskGraphActiveInterface
    {
    public:
        AZ_COMPONENT(AZ::TaskGraphSystemComponent, "{5D56B829-1FEB-43D5-A0BD-E33C0497EFE2}")

        TaskGraphSystemComponent() = default;

        // Implement TaskGraphActiveInterface
        bool IsTaskGraphActive() const override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        /// \ref ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
        /// \ref ComponentDescriptor::GetDependentServices
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);
        /// \red ComponentDescriptor::Reflect
        static void Reflect(ReflectContext* reflection);

        AZ::TaskExecutor*   m_taskExecutor = nullptr;
    };
}
