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
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>
#include <PhysX/EditorJointBus.h>

namespace AZ
{
    class Vector3;
} // namespace AZ

namespace PhysX
{
    namespace AngleComponentModes
    {
        struct SharedRotationState;
    } // namespace AngleComponentModes

    class JointsSubComponentModeAnglePair final
        : public PhysXSubComponentModeBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;

        JointsSubComponentModeAnglePair(const AZStd::string& propertyName, const AZ::Vector3& axis, float max, float min);

        // PhysXSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        float MouseMove(
            AZStd::shared_ptr<JointsComponentModeCommon::SubComponentModes::AngleModesSharedRotationState>& sharedRotationState,
            const AzToolsFramework::AngularManipulator::Action& action,
            bool isFirstValue,
            float& angleDelta,
            AZ::Quaternion& manipulatorOrientation);

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        AZ::Vector3 m_axis = AZ::Vector3::CreateAxisX();
        float m_firstMax = AZStd::numeric_limits<float>::max();
        float m_firstMin = -AZStd::numeric_limits<float>::max();
        float m_secondMax = AZStd::numeric_limits<float>::max();
        float m_secondMin = -AZStd::numeric_limits<float>::max();
        AZStd::shared_ptr<JointsComponentModeCommon::SubComponentModes::AngleModesSharedRotationState> m_sharedRotationState;
        AngleLimitsFloatPair m_resetValue;
        AZ::EntityComponentIdPair m_entityComponentIdPair;
        
        AZStd::string m_propertyName;
        AZStd::shared_ptr<AzToolsFramework::AngularManipulator> m_firstManipulator;
        AZStd::shared_ptr<AzToolsFramework::AngularManipulator> m_secondManipulator;
    };
}
