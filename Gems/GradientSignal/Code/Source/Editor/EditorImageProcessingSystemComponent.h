/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

namespace GradientSignal
{
    class EditorImageProcessingSystemComponent
        : public AZ::Component
        , protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorImageProcessingSystemComponent, "{3AF5AB01-161C-4762-A73F-BBDD2B878F6A}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        void AddContextMenuActions(QWidget* /*caller*/, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        bool HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry) const;
    };
} // namespace GradientSignal
