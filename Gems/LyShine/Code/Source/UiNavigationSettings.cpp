/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiNavigationSettings.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiNavigationSettings::UiNavigationSettings()
    : m_navigationMode(NavigationMode::Automatic)
    , m_onUpEntity()
    , m_onDownEntity()
    , m_onLeftEntity()
    , m_onRightEntity()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiNavigationSettings::~UiNavigationSettings()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiNavigationInterface::NavigationMode UiNavigationSettings::GetNavigationMode()
{
    return m_navigationMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiNavigationSettings::SetNavigationMode(NavigationMode navigationMode)
{
    m_navigationMode = navigationMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiNavigationSettings::GetOnUpEntity()
{
    return m_onUpEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiNavigationSettings::SetOnUpEntity(AZ::EntityId entityId)
{
    m_onUpEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiNavigationSettings::GetOnDownEntity()
{
    return m_onDownEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiNavigationSettings::SetOnDownEntity(AZ::EntityId entityId)
{
    m_onDownEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiNavigationSettings::GetOnLeftEntity()
{
    return m_onLeftEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiNavigationSettings::SetOnLeftEntity(AZ::EntityId entityId)
{
    m_onLeftEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiNavigationSettings::GetOnRightEntity()
{
    return m_onRightEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiNavigationSettings::SetOnRightEntity(AZ::EntityId entityId)
{
    m_onRightEntity = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiNavigationSettings::Activate(AZ::EntityId entityId, GetNavigableEntitiesFn getNavigableFn)
{
    m_entityId = entityId;
    m_getNavigableEntitiesFunction = getNavigableFn;
    UiNavigationBus::Handler::BusConnect(entityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiNavigationSettings::Deactivate()
{
    UiNavigationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiNavigationSettings::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiNavigationSettings>()
            ->Version(1)
            ->Field("NavigationMode", &UiNavigationSettings::m_navigationMode)
            ->Field("OnUpEntity", &UiNavigationSettings::m_onUpEntity)
            ->Field("OnDownEntity", &UiNavigationSettings::m_onDownEntity)
            ->Field("OnLeftEntity", &UiNavigationSettings::m_onLeftEntity)
            ->Field("OnRightEntity", &UiNavigationSettings::m_onRightEntity);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiNavigationSettings>("Navigation", "Navigation settings");

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiNavigationSettings::m_navigationMode, "Mode",
                "Determines how the next element to receive focus is chosen when a navigation event occurs")
                ->EnumAttribute(NavigationMode::Automatic, "Automatic")
                ->EnumAttribute(NavigationMode::Custom, "Custom")
                ->EnumAttribute(NavigationMode::None, "None")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiNavigationSettings::m_onUpEntity, "Up Element", "The element to receive focus on an up event")
                ->Attribute(AZ::Edit::Attributes::EnumValues, &UiNavigationSettings::PopulateNavigableEntityList)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiNavigationSettings::IsNavigationModeCustom);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiNavigationSettings::m_onDownEntity, "Down Element", "The element to receive focus on a down event")
                ->Attribute(AZ::Edit::Attributes::EnumValues, &UiNavigationSettings::PopulateNavigableEntityList)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiNavigationSettings::IsNavigationModeCustom);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiNavigationSettings::m_onLeftEntity, "Left Element", "The element to receive focus on a left event")
                ->Attribute(AZ::Edit::Attributes::EnumValues, &UiNavigationSettings::PopulateNavigableEntityList)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiNavigationSettings::IsNavigationModeCustom);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiNavigationSettings::m_onRightEntity, "Right Element", "The element to receive focus on a right event")
                ->Attribute(AZ::Edit::Attributes::EnumValues, &UiNavigationSettings::PopulateNavigableEntityList)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiNavigationSettings::IsNavigationModeCustom);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Enum<(int)UiNavigationInterface::NavigationMode::Automatic>("eUiNavigationMode_Automatic")
            ->Enum<(int)UiNavigationInterface::NavigationMode::Custom>("eUiNavigationMode_Custom")
            ->Enum<(int)UiNavigationInterface::NavigationMode::None>("eUiNavigationMode_None");

        behaviorContext->EBus<UiNavigationBus>("UiNavigationBus")
            ->Event("GetNavigationMode", &UiNavigationBus::Events::GetNavigationMode)
            ->Event("SetNavigationMode", &UiNavigationBus::Events::SetNavigationMode)
            ->Event("GetOnUpEntity", &UiNavigationBus::Events::GetOnUpEntity)
            ->Event("SetOnUpEntity", &UiNavigationBus::Events::SetOnUpEntity)
            ->Event("GetOnDownEntity", &UiNavigationBus::Events::GetOnDownEntity)
            ->Event("SetOnDownEntity", &UiNavigationBus::Events::SetOnDownEntity)
            ->Event("GetOnLeftEntity", &UiNavigationBus::Events::GetOnLeftEntity)
            ->Event("SetOnLeftEntity", &UiNavigationBus::Events::SetOnLeftEntity)
            ->Event("GetOnRightEntity", &UiNavigationBus::Events::GetOnRightEntity)
            ->Event("SetOnRightEntity", &UiNavigationBus::Events::SetOnRightEntity);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiNavigationSettings::EntityComboBoxVec UiNavigationSettings::PopulateNavigableEntityList()
{
    EntityComboBoxVec result;

    // Add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(), "<None>"));

    // Get a list of all navigable elements using callback function
    LyShine::EntityArray navigableElements = m_getNavigableEntitiesFunction(m_entityId);

    // Sort the elements by name
    AZStd::sort(navigableElements.begin(), navigableElements.end(),
        [](const AZ::Entity* e1, const AZ::Entity* e2) { return e1->GetName() < e2->GetName(); });

    // Add their names to the StringList and their IDs to the id list
    for (auto navigableEntity : navigableElements)
    {
        result.push_back(AZStd::make_pair(navigableEntity->GetId(), navigableEntity->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiNavigationSettings::IsNavigationModeCustom() const
{
    return (m_navigationMode == NavigationMode::Custom);
}
