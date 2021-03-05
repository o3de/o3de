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
#include "QuadShape.h"

namespace LmbrCentral
{
    //! Provide a Component interface for QuadShape functionality.
    class QuadShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(LmbrCentral::QuadShapeComponent, QuadShapeComponentTypeId);
        static void Reflect(AZ::ReflectContext* context);

        //! AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        QuadShape m_quadShape; ///< Stores underlying quad type for this component.
    };

    //! Concrete EntityDebugDisplay implementation for QuadShape.
    class QuadShapeDebugDisplayComponent
        : public EntityDebugDisplayComponent
        , public ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(LmbrCentral::QuadShapeDebugDisplayComponent, "{77B1AD7C-445C-46C1-8A90-6F86F307B7CD}", EntityDebugDisplayComponent);
        static void Reflect(AZ::ReflectContext* context);

        QuadShapeDebugDisplayComponent() = default;

        //! AZ::Component Overrides...
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //! EntityDebugDisplayComponent Overrides...
        void Draw(AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        AZ_DISABLE_COPY_MOVE(QuadShapeDebugDisplayComponent);

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        QuadShapeConfig m_quadShapeConfig; ///< Stores configuration data for quad shape.
        AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne(); ///< Caches non-uniform scale for this entity.
    };
} // namespace LmbrCentral
