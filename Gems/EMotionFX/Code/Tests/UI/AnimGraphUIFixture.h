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

#include <Tests/SystemComponentFixture.h>
#include <Tests/UI/UIFixture.h>

#include <AzCore/Memory/MemoryComponent.h>
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
