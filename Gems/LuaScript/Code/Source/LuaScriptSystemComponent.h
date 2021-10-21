/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: LuaScriptSystemComponent.
 * Create: 2021-07-14
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerBus.h>

#include "Previewer/LuaScriptPreviewerFactory.h"

namespace LuaScript {
    class LuaScriptSystemComponent
        :public AZ::Component,
        protected AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler
    {
    public:
        AZ_COMPONENT(LuaScriptSystemComponent, "{775FFB5D-08E1-4971-BCE7-8CFCC46B6E1A}");
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler
        const AzToolsFramework::AssetBrowser::PreviewerFactory* GetPreviewerFactory(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;

    private:
        AZStd::unique_ptr<const LuaScriptPreviewerFactory> m_previewerFactory;
    };
    
}
