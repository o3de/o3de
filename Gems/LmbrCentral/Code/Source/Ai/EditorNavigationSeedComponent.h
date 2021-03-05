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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Ai/NavigationSeedBus.h>

namespace LmbrCentral
{
    class EditorNavigationSeedComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private NavigationSeedRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
        using Base = AzToolsFramework::Components::EditorComponentBase;

    public:
        AZ_EDITOR_COMPONENT(EditorNavigationSeedComponent, "{A836E9F7-0C5A-4397-AD01-523EBC1E41A5}");
        EditorNavigationSeedComponent() = default;

    protected:
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string m_agentType;

        void TriggerReachaibilityRecalculation() const;
        AZ::u32 OnAgentTypeChanged() const;

        // NavigationSeedRequestBus
        void RecalculateReachabilityAroundSelf() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
    };
} // namespace LmbrCentral