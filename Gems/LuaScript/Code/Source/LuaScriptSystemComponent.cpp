/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: LuaScriptSystemComponent.
 * Create: 2021-07-14
 */
#include <LuaScriptSystemComponent.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LuaScript {
    void LuaScriptSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LuaScriptSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    void LuaScriptSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LuaScriptService"));
    }

    void LuaScriptSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Enforce singleton behavior by forbidding further components
        // which provide this same service from being added to an entity.
        incompatible.push_back(AZ_CRC("LuaScriptService"));
    }

    void LuaScriptSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // This component does not depend upon any other services.
        (void)required;
    }

    void LuaScriptSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        // This component does not depend upon any other services.
        (void)dependent;
    }

    const AzToolsFramework::AssetBrowser::PreviewerFactory* LuaScriptSystemComponent::GetPreviewerFactory(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
    {
        return m_previewerFactory->IsEntrySupported(entry) ? m_previewerFactory.get() : nullptr;
    }

    void LuaScriptSystemComponent::Init()
    {
        m_previewerFactory.reset(new LuaScriptPreviewerFactory);
    }

    void LuaScriptSystemComponent::Activate()
    {
        AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler::BusConnect();
    }

    void LuaScriptSystemComponent::Deactivate()
    {
        AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler::BusDisconnect();
    }
}