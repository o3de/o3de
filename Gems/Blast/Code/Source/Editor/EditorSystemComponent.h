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
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <Editor/EditorBlastSliceAssetHandler.h>

namespace Blast
{
    ///
    /// System component for Blast editor
    ///
    class EditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        AZ_COMPONENT(EditorSystemComponent, "{D29CF1A3-5E2C-4385-8541-F7CE78D5FFF8}");
        static void Reflect(AZ::ReflectContext* context);

        EditorSystemComponent() = default;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("BlastEditorService", 0x0a61cda5));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("BlastService", 0x75beae2d));
        }

        AZStd::unique_ptr<EditorBlastSliceAssetHandler> m_editorBlastSliceAssetHandler;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EditorEvents
        void NotifyRegisterViews() override;
    };
} // namespace Blast
