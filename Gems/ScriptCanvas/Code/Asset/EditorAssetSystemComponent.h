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

#include "EditorAssetConversionBus.h"
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <ScriptCanvas/Asset/AssetRegistry.h>

namespace ScriptCanvasEditor
{
    class EditorAssetSystemComponent
        : public AZ::Component
        , public EditorAssetConversionBus::Handler
        , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorAssetSystemComponent, "{2FB1C848-B863-4562-9C4B-01E18BD61583}");

        EditorAssetSystemComponent() = default;
        ~EditorAssetSystemComponent() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
        
        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler overrides
        void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUuid, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorAssetConversionBus::Handler overrides
        AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> CreateRuntimeAsset(AZStd::string_view graphPath) override;
        //////////////////////////////////////////////////////////////////////////
        
        ScriptCanvas::AssetRegistry& GetAssetRegistry();

    private:
        EditorAssetSystemComponent(const EditorAssetSystemComponent&) = delete;

        ScriptCanvas::AssetRegistry m_editorAssetRegistry;
    };
}
