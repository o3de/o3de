/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiTestScrollBoxDataProviderComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiImageBus.h>

#include "UiDynamicContentDatabase.h"
#include "LyShineExamplesInternalBus.h"

namespace LyShineExamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PUBLIC MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    UiTestScrollBoxDataProviderComponent::UiTestScrollBoxDataProviderComponent()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    UiTestScrollBoxDataProviderComponent::~UiTestScrollBoxDataProviderComponent()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    int UiTestScrollBoxDataProviderComponent::GetNumElements()
    {
        UiDynamicContentDatabase *uiDynamicContentDB = nullptr;
        EBUS_EVENT_RESULT(uiDynamicContentDB, LyShineExamplesInternalBus, GetUiDynamicContentDatabase);
        if (uiDynamicContentDB)
        {
            return uiDynamicContentDB->GetNumColors(UiDynamicContentDatabaseInterface::ColorType::PaidColors);
        }

        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiTestScrollBoxDataProviderComponent::OnElementBecomingVisible(AZ::EntityId entityId, int index)
    {
        UiDynamicContentDatabase *uiDynamicContentDB = nullptr;
        EBUS_EVENT_RESULT(uiDynamicContentDB, LyShineExamplesInternalBus, GetUiDynamicContentDatabase);
        if (uiDynamicContentDB)
        {
            if ((index >= 0) && (index < uiDynamicContentDB->GetNumColors(UiDynamicContentDatabaseInterface::ColorType::PaidColors)))
            {
                AZ::Entity* entity = nullptr;

                EBUS_EVENT_ID_RESULT(entity, entityId, UiElementBus, FindChildByName, "Name");
                if (entity)
                {
                    AZStd::string text = uiDynamicContentDB->GetColorName(UiDynamicContentDatabaseInterface::ColorType::PaidColors, index);
                    EBUS_EVENT_ID(entity->GetId(), UiTextBus, SetText, text.c_str());
                }

                entity = nullptr;
                EBUS_EVENT_ID_RESULT(entity, entityId, UiElementBus, FindChildByName, "Price");
                if (entity)
                {
                    AZStd::string text = uiDynamicContentDB->GetColorPrice(UiDynamicContentDatabaseInterface::ColorType::PaidColors, index);
                    EBUS_EVENT_ID(entity->GetId(), UiTextBus, SetText, text.c_str());
                }

                entity = nullptr;
                EBUS_EVENT_ID_RESULT(entity, entityId, UiElementBus, FindChildByName, "Icon");
                if (entity)
                {
                    AZ::Color color = uiDynamicContentDB->GetColor(UiDynamicContentDatabaseInterface::ColorType::PaidColors, index);
                    EBUS_EVENT_ID(entity->GetId(), UiImageBus, SetColor, color);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PROTECTED STATIC MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiTestScrollBoxDataProviderComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<UiTestScrollBoxDataProviderComponent, AZ::Component>()
                ->Version(1);

            AZ::EditContext* ec = serializeContext->GetEditContext();
            if (ec)
            {
                auto editInfo = ec->Class<UiTestScrollBoxDataProviderComponent>("TestScrollBoxDataProvider",
                    "Associates dynamic data with a dynamic scroll box");

                editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiTestScrollBoxDataProvider.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiTestScrollBoxDataProvider.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PROTECTED MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiTestScrollBoxDataProviderComponent::Activate()
    {
        UiDynamicScrollBoxDataBus::Handler::BusConnect(GetEntityId());
        UiDynamicScrollBoxElementNotificationBus::Handler::BusConnect(GetEntityId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiTestScrollBoxDataProviderComponent::Deactivate()
    {
        UiDynamicScrollBoxDataBus::Handler::BusDisconnect();
        UiDynamicScrollBoxElementNotificationBus::Handler::BusDisconnect();
    }
}
