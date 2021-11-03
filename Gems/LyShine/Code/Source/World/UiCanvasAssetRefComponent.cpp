/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasAssetRefComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/LyShineBus.h>
#include <CryCommon/LyShine/ILyShine.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiCanvasAssetRefNotificationBus Behavior context handler class
class UiCanvasAssetRefNotificationBusBehaviorHandler
    : public UiCanvasAssetRefNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiCanvasAssetRefNotificationBusBehaviorHandler, "{CA397C92-9C0B-436C-9C71-38A1918929EC}", AZ::SystemAllocator,
        OnCanvasLoadedIntoEntity);

    void OnCanvasLoadedIntoEntity(AZ::EntityId uiCanvasEntity) override
    {
        Call(FN_OnCanvasLoadedIntoEntity, uiCanvasEntity);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiCanvasRefNotificationBus Behavior context handler class
class UiCanvasRefNotificationBusBehaviorHandler
    : public UiCanvasRefNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiCanvasRefNotificationBusBehaviorHandler, "{728D7B02-D5D1-493A-8DD1-3AE5EA595A79}", AZ::SystemAllocator,
        OnCanvasRefChanged);

    void OnCanvasRefChanged(AZ::EntityId uiCanvasRefEntity, AZ::EntityId uiCanvasEntity) override
    {
        Call(FN_OnCanvasRefChanged, uiCanvasRefEntity, uiCanvasEntity);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasAssetRefComponent::UiCanvasAssetRefComponent()
    : m_isAutoLoad(false)
    , m_shouldLoadDisabled(false)
{}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasAssetRefComponent::GetCanvas()
{
    return m_canvasEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiCanvasAssetRefComponent::GetCanvasPathname()
{
    return m_canvasAssetRef.GetAssetPath();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasAssetRefComponent::SetCanvasPathname(const AZStd::string& pathname)
{
    m_canvasAssetRef.SetAssetPath(pathname.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasAssetRefComponent::GetIsAutoLoad()
{
    return m_isAutoLoad;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasAssetRefComponent::SetIsAutoLoad(bool isAutoLoad)
{
    m_isAutoLoad = isAutoLoad;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasAssetRefComponent::GetShouldLoadDisabled()
{
    return m_shouldLoadDisabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasAssetRefComponent::SetShouldLoadDisabled(bool shouldLoadDisabled)
{
    m_shouldLoadDisabled = shouldLoadDisabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasAssetRefComponent::LoadCanvas()
{
    AZStd::string canvasPath = m_canvasAssetRef.GetAssetPath();
    if (!canvasPath.empty())
    {
        // Check if we already have a referenced UI canvas, if so release it
        if (m_canvasEntityId.IsValid())
        {
            gEnv->pLyShine->ReleaseCanvasDeferred(m_canvasEntityId);
            m_canvasEntityId.SetInvalid();
        }

        m_canvasEntityId = gEnv->pLyShine->LoadCanvas(canvasPath.c_str());

        EBUS_EVENT_ID(GetEntityId(), UiCanvasAssetRefNotificationBus, OnCanvasLoadedIntoEntity, m_canvasEntityId);
        EBUS_EVENT_ID(GetEntityId(), UiCanvasRefNotificationBus, OnCanvasRefChanged, GetEntityId(), m_canvasEntityId);
    }

    return m_canvasEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasAssetRefComponent::UnloadCanvas()
{
    if (m_canvasEntityId.IsValid())
    {
        gEnv->pLyShine->ReleaseCanvasDeferred(m_canvasEntityId);
        m_canvasEntityId.SetInvalid();

        EBUS_EVENT_ID(GetEntityId(), UiCanvasRefNotificationBus, OnCanvasRefChanged, GetEntityId(), m_canvasEntityId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasAssetRefComponent::OnCanvasUnloaded(AZ::EntityId canvasEntityId)
{
    if (canvasEntityId == m_canvasEntityId)
    {
        // this canvas has been unloaded (e.g. from script), set our canvas entity ID to invalid
        // and tell anyone watching this assert ref that it changed
        m_canvasEntityId.SetInvalid();
        EBUS_EVENT_ID(GetEntityId(), UiCanvasRefNotificationBus, OnCanvasRefChanged, GetEntityId(), m_canvasEntityId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasAssetRefComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiCanvasAssetRefComponent, AZ::Component>()
            ->Version(1)
            ->Field("CanvasAssetRef", &UiCanvasAssetRefComponent::m_canvasAssetRef)
            ->Field("IsAutoLoad", &UiCanvasAssetRefComponent::m_isAutoLoad)
            ->Field("ShouldLoadDisabled", &UiCanvasAssetRefComponent::m_shouldLoadDisabled);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            auto editInfo = editContext->Class<UiCanvasAssetRefComponent>("UI Canvas Asset Ref", "The UI Canvas Asset Ref component allows you to associate a UI Canvas with an entity");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/UiCanvasAssetRef.svg")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/UiCanvasAssetRef.svg")
                ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/ui-canvas-asset-ref/")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c));

            editInfo->DataElement("SimpleAssetRef", &UiCanvasAssetRefComponent::m_canvasAssetRef,
                "Canvas pathname", "The pathname of the canvas.")
                ->Attribute("BrowseIcon", ":/stylesheet/img/UI20/browse-edit-select-files.svg")
                ->Attribute("EditButton", "")
                ->Attribute("EditDescription", "Open in UI Editor")
                ->Attribute("EditCallback", &UiCanvasAssetRefComponent::LaunchUIEditor);
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiCanvasAssetRefComponent::m_isAutoLoad,
                "Load automatically", "When checked, the canvas is loaded when this component is activated.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiCanvasAssetRefComponent::m_shouldLoadDisabled,
                "Load in disabled state", "When checked and loading automatically, the canvas is loaded in a disabled state.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiCanvasAssetRefComponent::m_isAutoLoad);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiCanvasAssetRefBus>("UiCanvasAssetRefBus")
            ->Event("LoadCanvas", &UiCanvasAssetRefBus::Events::LoadCanvas)
            ->Event("UnloadCanvas", &UiCanvasAssetRefBus::Events::UnloadCanvas);

        behaviorContext->EBus<UiCanvasRefBus>("UiCanvasRefBus")
            ->Event("GetCanvas", &UiCanvasRefBus::Events::GetCanvas);

        behaviorContext->EBus<UiCanvasAssetRefNotificationBus>("UiCanvasAssetRefNotificationBus")
            ->Handler<UiCanvasAssetRefNotificationBusBehaviorHandler>();

        behaviorContext->EBus<UiCanvasRefNotificationBus>("UiCanvasRefNotificationBus")
            ->Handler<UiCanvasRefNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasAssetRefComponent::LaunchUIEditor([[maybe_unused]] const AZ::Data::AssetId& assetId, const AZ::Data::AssetType&)
{
    LyShine::LyShineRequestBus::Broadcast(&LyShine::LyShineRequests::EditUICanvas, GetCanvasPathname());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasAssetRefComponent::Activate()
{
    if (!gEnv->IsDedicated())
    {
        UiCanvasRefBus::Handler::BusConnect(GetEntityId());
        UiCanvasAssetRefBus::Handler::BusConnect(GetEntityId());
        UiCanvasManagerNotificationBus::Handler::BusConnect();

        if (m_isAutoLoad)
        {
            LoadCanvas();

            if (m_shouldLoadDisabled)
            {
                EBUS_EVENT_ID(m_canvasEntityId, UiCanvasBus, SetEnabled, false);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasAssetRefComponent::Deactivate()
{
    if (!gEnv->IsDedicated())
    {
        if (m_canvasEntityId.IsValid())
        {
            gEnv->pLyShine->ReleaseCanvasDeferred(m_canvasEntityId);
            m_canvasEntityId.SetInvalid();
        }

        UiCanvasAssetRefBus::Handler::BusDisconnect();
        UiCanvasRefBus::Handler::BusDisconnect();
        UiCanvasManagerNotificationBus::Handler::BusDisconnect();
    }
}
