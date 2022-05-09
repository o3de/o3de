/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <Editor/EditorBlastChunksAssetHandler.h>

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
            provided.push_back(AZ_CRC("BlastEditorService", 0xeddfed0d));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("BlastService", 0x46927a9f));
        }

        AZStd::unique_ptr<EditorBlastChunksAssetHandler> m_editorBlastChunksAssetHandler;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EditorEvents
        void NotifyRegisterViews() override;
    };
} // namespace Blast
