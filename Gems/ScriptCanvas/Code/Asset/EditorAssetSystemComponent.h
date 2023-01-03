/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include "EditorAssetConversionBus.h"
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <ScriptCanvas/Asset/AssetRegistry.h>
#include <ScriptCanvas/Translation/Translation.h>

namespace ScriptCanvasEditor
{
    class EditorAssetSystemComponent
        : public AZ::Component
        , public EditorAssetConversionBus::Handler
    {
    public:
        AZ_COMPONENT(EditorAssetSystemComponent, "{2FB1C848-B863-4562-9C4B-01E18BD61583}");

        EditorAssetSystemComponent() = default;
        ~EditorAssetSystemComponent() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component...
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorAssetConversionBus::Handler...
        AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> CreateRuntimeAsset(const SourceHandle& editAsset) override;
        AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> CreateLuaAsset(const SourceHandle& editAsset, AZStd::string_view graphPathForRawLuaFile) override;
        //////////////////////////////////////////////////////////////////////////
        
        ScriptCanvas::AssetRegistry& GetAssetRegistry();

    private:
        ScriptCanvas::AssetRegistry m_editorAssetRegistry;    
        EditorAssetSystemComponent(const EditorAssetSystemComponent&) = delete;
    };
}
