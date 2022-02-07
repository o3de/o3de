/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/limits.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>
#include <PhysX/EditorJointBus.h>

namespace AzToolsFramework
{
    class AngularManipulator;
    class LinearManipulator;
    class PlanarManipulator;
} // namespace AzToolsFramework

namespace PhysX
{
    class JointsSubComponentModeAngleCone final
        : public PhysXSubComponentModeBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;

        JointsSubComponentModeAngleCone(const AZStd::string& propertyName, float max, float min);

        // PhysXSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        void ConfigureLinearView(
            float axisLength,
            const AZ::Color& axis1Color,
            const AZ::Color& axis2Color,
            const AZ::Color& axis3Color = AZ::Color(0.0f, 0.0f, 1.0f, 0.5f));

        void ConfigurePlanarView(
            const AZ::Color& planeColor = AZ::Color(0.0f, 1.0f, 0.0f, 0.5f),
            const AZ::Color& plane2Color = AZ::Color(0.0f, 0.0f, 1.0f, 0.5f));

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        float m_max = AZStd::numeric_limits<float>::max();
        float m_min = -AZStd::numeric_limits<float>::max();

        AZ::EntityComponentIdPair m_entityComponentIdPair;
        AZ::Vector3 m_resetPostion;
        AZ::Vector3 m_resetRotation;
        AngleLimitsFloatPair m_resetLimits;
        AZStd::string m_propertyName;
        AZStd::shared_ptr<AzToolsFramework::AngularManipulator> m_xRotationManipulator;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_yLinearManipulator;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_zLinearManipulator;
        AZStd::shared_ptr<AzToolsFramework::PlanarManipulator> m_yzPlanarManipulator;
    };
}
