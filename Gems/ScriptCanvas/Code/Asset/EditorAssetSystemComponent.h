/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include "EditorAssetConversionBus.h"
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <ScriptCanvas/Asset/AssetRegistry.h>

#include <ScriptCanvas/Grammar/GrammarContext.h>
#include <ScriptCanvas/Grammar/GrammarContextBus.h>
#include <ScriptCanvas/Translation/TranslationContext.h>
#include <ScriptCanvas/Translation/TranslationContextBus.h>
#include <ScriptCanvas/Translation/Translation.h>

namespace ScriptCanvasEditor
{
    class ScriptCanvasAsset;

    class EditorAssetSystemComponent
        : public AZ::Component
        , public EditorAssetConversionBus::Handler
        , public ScriptCanvas::Grammar::RequestBus::Handler
        , public ScriptCanvas::Translation::RequestBus::Handler
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
        // AZ::Component...
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
        
        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler...
        void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUuid, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorAssetConversionBus::Handler...
        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> LoadAsset(AZStd::string_view graphPath) override;
        AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> CreateRuntimeAsset(const AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset>& editAsset) override;
        AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> CreateLuaAsset(const AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset>& editAsset, AZStd::string_view graphPathForRawLuaFile) override;
        //////////////////////////////////////////////////////////////////////////
        
        // ScriptCanvas::Grammar::RequestBus::Handler...
        ScriptCanvas::Grammar::Context* GetGrammarContext() override;
        
        // ScriptCanvas::Translation::RequestBus::Handler...
        ScriptCanvas::Translation::Context* GetTranslationContext() override;


        ScriptCanvas::AssetRegistry& GetAssetRegistry();

    private:
        ScriptCanvas::AssetRegistry m_editorAssetRegistry;
        ScriptCanvas::Translation::Context m_translationContext;
        ScriptCanvas::Grammar::Context m_grammarContext;
    
        EditorAssetSystemComponent(const EditorAssetSystemComponent&) = delete;
    };
}
