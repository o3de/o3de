/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <ISystem.h>
#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiCursorBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShineMessagePopup.h>

namespace MessagePopup
{
    //-------------------------------------------------------------------------
    LyShineMessagePopup::LyShineMessagePopup()
    {
    }

    //-------------------------------------------------------------------------
    void LyShineMessagePopup::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LyShineMessagePopup, AZ::Component>()
                ->Version(1);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LyShineMessagePopup>("MessagePopup", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    //-------------------------------------------------------------------------
    void LyShineMessagePopup::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MessagePopupService"));
    }

    //-------------------------------------------------------------------------
    void LyShineMessagePopup::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MessagePopupService"));
    }

    //-------------------------------------------------------------------------
    void LyShineMessagePopup::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    //-------------------------------------------------------------------------
    void LyShineMessagePopup::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    //-------------------------------------------------------------------------
    void LyShineMessagePopup::Init()
    {
    }

    //-------------------------------------------------------------------------
    void LyShineMessagePopup::Activate()
    {
        MessagePopup::MessagePopupImplBus::Handler::BusConnect();
    }

    //-------------------------------------------------------------------------
    void LyShineMessagePopup::Deactivate()
    {
        MessagePopup::MessagePopupImplBus::Handler::BusConnect();
    }

    //-----------------------------------------------------------------------------
    void LyShineMessagePopup::OnAction([[maybe_unused]] AZ::EntityId entityId, const LyShine::ActionName& actionName)
    {
        const AZ::EntityId* canvasId = UiCanvasNotificationBus::GetCurrentBusId();
        if (!canvasId)
        {
            return;
        }

        const auto it = m_activePopupIdsByCanvasId.find(*canvasId);
        if (it == m_activePopupIdsByCanvasId.end())
        {
            return;
        }

        const AZ::u32 popupId = it->second;
        if (actionName == "OnButton1")
        {
            MessagePopup::MessagePopupRequestBus::Broadcast(&MessagePopup::MessagePopupRequestBus::Events::HidePopup, popupId, 0);
            MessagePopupNotificationsBus::Broadcast(&MessagePopupNotificationsBus::Events::OnHide, popupId, 0);
        }
        else if (actionName == "OnButton2")
        {
            MessagePopup::MessagePopupRequestBus::Broadcast(&MessagePopup::MessagePopupRequestBus::Events::HidePopup, popupId, 1);
            MessagePopupNotificationsBus::Broadcast(&MessagePopupNotificationsBus::Events::OnHide, popupId, 1);
        }
        else if (actionName == "OnButton3")
        {
            MessagePopup::MessagePopupRequestBus::Broadcast(&MessagePopup::MessagePopupRequestBus::Events::HidePopup, popupId, 2);
            MessagePopupNotificationsBus::Broadcast(&MessagePopupNotificationsBus::Events::OnHide, popupId, 2);
        }
    }

    //-----------------------------------------------------------------------------
    void LyShineMessagePopup::OnShowPopup(AZ::u32 _popupID, const AZStd::string& _message, EPopupButtons _buttons, EPopupKind _kind, AZStd::function<void(int _button)> _callback, void** _popupClientID)
    {
        AZ::EntityId canvasEntityId;
        bool isNavigationSupported = false;
        if (_kind == EPopupKind_Generic)
        {
            switch (_buttons)
            {
            case EPopupButtons::EPopupButtons_NoButtons:
                canvasEntityId = AZ::Interface<ILyShine>::Get()->LoadCanvas("@products@/ui/canvases/defaultmessagepopup.uicanvas");
                break;
            case EPopupButtons::EPopupButtons_Confirm:
                canvasEntityId = AZ::Interface<ILyShine>::Get()->LoadCanvas("@products@/ui/canvases/defaultmessagepopup_confirm.uicanvas");
                isNavigationSupported = true;
                break;
            case EPopupButtons::EPopupButtons_YesNo:
                canvasEntityId = AZ::Interface<ILyShine>::Get()->LoadCanvas("@products@/ui/canvases/defaultmessagepopup_yesno.uicanvas");
                isNavigationSupported = true;
                break;
            }
        }
        else if (_kind == EPopupKind_Toaster)
        {
            canvasEntityId = AZ::Interface<ILyShine>::Get()->LoadCanvas("@products@/ui/canvases/toaster.uicanvas");
        }

        if (canvasEntityId.IsValid())
        {
            // Get the canvasID to send to the MessagePopupManager
            LyShine::CanvasId canvasId = 0;
            UiCanvasBus::EventResult(canvasId, canvasEntityId, &UiCanvasBus::Events::GetCanvasId);

            *_popupClientID = (void*)(AZ::u64)canvasId;

            // Enable the popup
            AZ::Entity* instructionsTextElement = nullptr;
            UiCanvasBus::Event(canvasEntityId, &UiCanvasBus::Events::SetEnabled, true);
            UiCanvasBus::Event(canvasEntityId, &UiCanvasBus::Events::SetKeepLoadedOnLevelUnload, true);

            // Set the text
            UiCanvasBus::EventResult(instructionsTextElement, canvasEntityId, &UiCanvasBus::Events::FindElementByName, "Text");
            if (instructionsTextElement != nullptr && instructionsTextElement->GetId().IsValid())
            {
                UiTextBus::Event(instructionsTextElement->GetId(), &UiTextBus::Events::SetText, _message.c_str());
            }

            // Set whether navigation is supported, and show the cursor if so
            UiCanvasBus::Event(canvasEntityId, &UiCanvasInterface::SetIsNavigationSupported, isNavigationSupported);
            if (isNavigationSupported)
            {
                UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);
            }

            m_activePopupIdsByCanvasId[canvasEntityId] = _popupID;

            UiCanvasNotificationBus::MultiHandler::BusConnect(canvasEntityId);
        }
        // Having an invalid canvas will rollback to the platform messagepopup
    }

    //-----------------------------------------------------------------------------
    void LyShineMessagePopup::OnHidePopup(const MessagePopupInfo& _popupInfo)
    {
        // get the canvas ID in the clientdata
        LyShine::CanvasId canvasId = *((LyShine::CanvasId*)&_popupInfo.m_clientData);
        AZ::EntityId canvasEntityId = AZ::Interface<ILyShine>::Get()->FindCanvasById(canvasId);
        if (canvasEntityId.IsValid())
        {
            // Hide the cursor if it was shown in LyShineMessagePopup::OnShowPopup
            bool isNavigationSupported = false;
            UiCanvasBus::EventResult(isNavigationSupported, canvasEntityId, &UiCanvasInterface::GetIsNavigationSupported);
            if (isNavigationSupported)
            {
                UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);
            }

            // Disable the popup
            UiCanvasBus::Event(canvasEntityId, &UiCanvasBus::Events::SetEnabled, false);

            AZ::Interface<ILyShine>::Get()->ReleaseCanvas(canvasEntityId, false);

            UiCanvasNotificationBus::MultiHandler::BusDisconnect(canvasEntityId);
            m_activePopupIdsByCanvasId.erase(canvasEntityId);
        }
    }
}
