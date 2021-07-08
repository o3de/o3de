/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include "ScriptCanvasBuilderWorker.h"

#include <ScriptCanvas/Grammar/GrammarContext.h>
#include <ScriptCanvas/Grammar/GrammarContextBus.h>
#include <ScriptCanvas/Translation/TranslationContext.h>
#include <ScriptCanvas/Translation/TranslationContextBus.h>

namespace ScriptCanvasBuilder
{
    //! ScriptCanvasBuilder is responsible for turning editor ScriptCanvas Assets into runtime script canvas assets
    class PluginComponent
        : public AZ::Component
        , public ScriptCanvas::Grammar::RequestBus::Handler
        , public ScriptCanvas::Translation::RequestBus::Handler
    {
    public:
        AZ_COMPONENT(PluginComponent, "{F8286B21-E751-4745-8BC4-512F190215FF}")
        static void Reflect(AZ::ReflectContext* context);

        PluginComponent() = default;
        ~PluginComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        // ScriptCanvas::Grammar::RequestBus::Handler
        ScriptCanvas::Grammar::Context* GetGrammarContext() override;

        // ScriptCanvas::Translation::RequestBus::Handler
        ScriptCanvas::Translation::Context* GetTranslationContext() override;

    private:
        PluginComponent(const PluginComponent&) = delete;

        SharedHandlers m_sharedHandlers;

        Worker m_scriptCanvasBuilder;
        ScriptCanvas::Translation::Context m_translationContext;
        ScriptCanvas::Grammar::Context m_grammarContext;
    };
}
