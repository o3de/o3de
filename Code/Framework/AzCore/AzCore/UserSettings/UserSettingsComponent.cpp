/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    //-----------------------------------------------------------------------------
    UserSettingsComponent::UserSettingsComponent(u32 providerId)
        : m_providerId(providerId)
    {
    }

    //-----------------------------------------------------------------------------
    UserSettingsComponent::~UserSettingsComponent()
    {
    }

    //-----------------------------------------------------------------------------
    void UserSettingsComponent::Activate()
    {
        Load();
        m_provider.Activate(m_providerId);

        UserSettingsComponentRequestBus::Handler::BusConnect();
    }

    //-----------------------------------------------------------------------------
    void UserSettingsComponent::Deactivate()
    {
        UserSettingsComponentRequestBus::Handler::BusDisconnect();
        // We specifically avoid auto-saving user settings here because that could cause crashes on shutdown since the module that created the user settings may be unloaded at this point
        m_provider.Deactivate();
    }

    //-----------------------------------------------------------------------------
    void UserSettingsComponent::Load()
    {
        AZStd::string settingsPath;
        UserSettingsFileLocatorBus::BroadcastResult(settingsPath, &UserSettingsFileLocatorBus::Events::ResolveFilePath, m_providerId);
        SerializeContext* serializeContext = nullptr;
        ComponentApplicationBus::BroadcastResult(serializeContext, &ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Warning("UserSettings", serializeContext != nullptr, "Failed to retrieve the serialization context. User settings cannot be loaded.");
        if (!settingsPath.empty() && serializeContext != nullptr)
        {
            m_provider.Load(settingsPath.c_str(), serializeContext);
        }
    }

    //-----------------------------------------------------------------------------
    void UserSettingsComponent::Save()
    {
        AZStd::string settingsPath;
        UserSettingsFileLocatorBus::BroadcastResult(settingsPath, &UserSettingsFileLocatorBus::Events::ResolveFilePath, m_providerId);
        SerializeContext* serializeContext = nullptr;
        ComponentApplicationBus::BroadcastResult(serializeContext, &ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Warning("UserSettings", serializeContext != nullptr, "Failed to retrieve the serialization context. User settings cannot be stored.");
        if (!settingsPath.empty() && serializeContext != nullptr)
        {
            m_provider.Save(settingsPath.c_str(), serializeContext);
        }
    }

    //-----------------------------------------------------------------------------
    void UserSettingsComponent::Finalize()
    {
        if (m_saveOnFinalize)
        {
            Save();
        }
        m_provider.Deactivate();
    }

    //-----------------------------------------------------------------------------
    void UserSettingsComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UserSettingsService"));
    }

    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    void UserSettingsComponent::Reflect(ReflectContext* context)
    {
        UserSettings::Reflect(context);
        UserSettingsProvider::Reflect(context);

        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<UserSettingsComponent, AZ::Component>()
                ->Version(3)
                ->Field("ProviderId", &UserSettingsComponent::m_providerId)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<UserSettingsComponent>(
                    "User Settings", "Provides userdata storage for all system components")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &UserSettingsComponent::m_providerId, "ProviderId", "The settings group this provider with handle.")
                        ->EnumAttribute(UserSettings::CT_LOCAL, "Local")
                        ->EnumAttribute(UserSettings::CT_GLOBAL, "Global")
                    ;
            }
        }
    }
    //-----------------------------------------------------------------------------
}   // namespace AZ
