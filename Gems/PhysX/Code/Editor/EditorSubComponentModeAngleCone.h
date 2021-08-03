
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Editor/EditorSubComponentModeBase.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    class AngularManipulator;
    class LinearManipulator;
    class PlanarManipulator;
}

namespace PhysX
{
    class EditorSubComponentModeAngleCone
        : public PhysX::EditorSubComponentModeBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        EditorSubComponentModeAngleCone(
            const AZ::EntityComponentIdPair& entityComponentIdPair
            , const AZ::Uuid& componentType
            , const AZStd::string& name
            , float max
            , float min);
        ~EditorSubComponentModeAngleCone();

        // PhysX::EditorSubComponentModeBase
        void Refresh() override;

    private:
        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        void ConfigureLinearView(
            float axisLength,
            const AZ::Color& axis1Color, const AZ::Color& axis2Color,
            const AZ::Color& axis3Color = AZ::Color(0.0f, 0.0f, 1.0f, 0.5f));

        void ConfigurePlanarView(const AZ::Color& planeColor = AZ::Color(0.0f, 1.0f, 0.0f, 0.5f)
            , const AZ::Color& plane2Color = AZ::Color(0.0f, 0.0f, 1.0f, 0.5f));

        AZStd::shared_ptr<AzToolsFramework::AngularManipulator> m_xRotationManipulator;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_yLinearManipulator;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_zLinearManipulator;
        AZStd::shared_ptr<AzToolsFramework::PlanarManipulator> m_yzPlanarManipulator;

        float m_max = FLT_MAX;
        float m_min = 0.0f;
    };
} // namespace PhysX
