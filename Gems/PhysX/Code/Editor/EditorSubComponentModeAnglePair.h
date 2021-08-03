
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Editor/EditorSubComponentModeBase.h>
#include <PhysX/EditorJointBus.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>

namespace PhysX
{
    class EditorSubComponentModeAnglePair
        : public PhysX::EditorSubComponentModeBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        EditorSubComponentModeAnglePair(
            const AZ::EntityComponentIdPair& entityComponentIdPair
            , const AZ::Uuid& componentType
            , const AZStd::string& name
            , const AZ::Vector3& axis
            , float firstMax
            , float firstMin
            , float secondMax
            , float secondMin);
        ~EditorSubComponentModeAnglePair();

        // PhysX::EditorSubComponentModeBase
        void Refresh() override;

    private:
        struct SharedRotationState
        {
            AZ::Vector3 m_axis;
            AZ::Quaternion m_savedOrientation = AZ::Quaternion::CreateIdentity();
            AngleLimitsFloatPair m_valuePair;
        };

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        float MouseMove(AZStd::shared_ptr<SharedRotationState>& sharedRotationState
            , const AzToolsFramework::AngularManipulator::Action& action
            , bool isFirstValue
            , float& angleDelta
            , AZ::Quaternion& manipulatorOrientation);

        AZStd::shared_ptr<AzToolsFramework::AngularManipulator> m_firstManipulator;
        AZStd::shared_ptr<AzToolsFramework::AngularManipulator> m_secondManipulator;

        AZ::Vector3 m_axis = AZ::Vector3::CreateAxisX();
        float m_firstMax = FLT_MAX;
        float m_firstMin = -FLT_MAX;
        float m_secondMax = FLT_MAX;
        float m_secondMin = -FLT_MAX;
    };
} // namespace PhysX
