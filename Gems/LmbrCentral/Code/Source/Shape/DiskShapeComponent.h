/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include "Rendering/EntityDebugDisplayComponent.h"
#include "DiskShape.h"

namespace LmbrCentral
{
    //! Provide a Component interface for DiskShape functionality.
    class DiskShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(DiskShapeComponent, DiskShapeComponentTypeId);
        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        DiskShape m_diskShape; ///< Stores underlying disk type for this component.
    };

    //! Concrete EntityDebugDisplay implementation for DiskShape.
    class DiskShapeDebugDisplayComponent
        : public EntityDebugDisplayComponent
        , public ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(DiskShapeDebugDisplayComponent, "{05CAEA04-C439-45F4-BBE7-3EDA8753D83B}", EntityDebugDisplayComponent)
        static void Reflect(AZ::ReflectContext* context);

        DiskShapeDebugDisplayComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // EntityDebugDisplayComponent
        void Draw(AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        AZ_DISABLE_COPY_MOVE(DiskShapeDebugDisplayComponent)

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        DiskShapeConfig m_diskShapeConfig; ///< Stores configuration data for disk shape.
    };
} // namespace LmbrCentral
