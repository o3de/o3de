/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Tests/SystemComponentFixture.h>
#include <Tests/UI/UIFixture.h>

#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>

#include <QString>

namespace EMotionFX
{

    class AnimGraphUIFixture
        : public UIFixture
    {
    public:
        void SetUp() override;

        EMotionFX::AnimGraph* CreateAnimGraph() const;
        EMStudio::NodeGraph* GetActiveNodeGraph() const;
        EMotionFX::AnimGraphNode* CreateAnimGraphNode(const AZStd::string& type, const AZStd::string& args="", const AnimGraph* animGraph = nullptr) const;

        AnimGraphNode* AddNodeToAnimGraph(EMotionFX::AnimGraph*  animGraph, const QString& nodeTypeName);
    protected:
        EMStudio::AnimGraphPlugin* m_animGraphPlugin;
        EMStudio::BlendGraphWidget* m_blendGraphWidget;
    };
} // end namespace EMotionFX
