/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <MessagePopupSystemComponent.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/EBus/EBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace MessagePopup
{
    // BehaviorContext MessagePopupNotificationsBus forwarder
    class MessagePopupNotificationsBusHandler : public MessagePopupNotificationsBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(MessagePopupNotificationsBusHandler, "{7AEDC591-41AB-4E3B-87D2-0334615427AA}", AZ::SystemAllocator,
            OnHide);

        void OnHide(AZ::u32 popupID, int buttonPressed) override
        {
            Call(FN_OnHide, popupID, buttonPressed);
        }
    };

    //-------------------------------------------------------------------------
    MessagePopupSystemComponent::MessagePopupSystemComponent()
    {
    }

    //-------------------------------------------------------------------------
    void MessagePopupSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MessagePopupSystemComponent, AZ::Component>()
                ->Version(1);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MessagePopupSystemComponent>("MessagePopup", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<MessagePopupRequestBus>("MessagePopupRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay")
                ->Event("ShowToasterPopup", &MessagePopupRequests::ShowToasterPopup,
                    { {
                        { "Message", "The message to display. Localization ID can be used as well." },
                        { "Duration", "Number of seconds before closing the window" }
                    } }
                    )
                ->Attribute(AZ::Script::Attributes::ToolTip, "Show a information message window on bottom right of the screen for a short period of time.")
                ->Event("ShowPopup", &MessagePopupRequests::ShowPopup,
                    { {
                        { "Message", "The message to display. Localization ID can be used as well." },
                        { "Button kind", "0:OK, 1:Yes/No 2:no buttons" }
                    } }
                    )
                ->Event("HidePopup", &MessagePopupRequests::HidePopup,
                    { {
                        { "Popup ID", "The ID of the popup you get from the Result of a ShowPopup" },
                        { "Button Pressed", "Which button to simulate pressing?" },
                    } }
                );

            behaviorContext->EBus<MessagePopupNotificationsBus>("MessagePopupNotificationsBus")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay")
                ->Handler<MessagePopupNotificationsBusHandler>()
                ;
        }
    }
    
    //-------------------------------------------------------------------------
    void MessagePopupSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MessagePopupSystemComponentService"));
    }

    //-------------------------------------------------------------------------
    void MessagePopupSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MessagePopupSystemComponentService"));
    }

    //-------------------------------------------------------------------------
    void MessagePopupSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    //-------------------------------------------------------------------------
    void MessagePopupSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    //-------------------------------------------------------------------------
    void MessagePopupSystemComponent::Init()
    {
    }

    //-------------------------------------------------------------------------
    void MessagePopupSystemComponent::Activate()
    {
        MessagePopupRequestBus::Handler::BusConnect();
    }

    //-------------------------------------------------------------------------
    void MessagePopupSystemComponent::Deactivate()
    {
        MessagePopupRequestBus::Handler::BusDisconnect();
    }

    //-------------------------------------------------------------------------
    AZ::u32 MessagePopupSystemComponent::ShowPopup(const AZStd::string& _message, EPopupButtons _buttons)
    {
        return InternalShowPopup(_message, _buttons, EPopupKind_Generic, nullptr, 0.0f);
    }

    //-------------------------------------------------------------------------
    AZ::u32 MessagePopupSystemComponent::ShowPopupWithCallback(const AZStd::string& _message, EPopupButtons _buttons, AZStd::function<void(int _button)> _callback)
    {
        return InternalShowPopup(_message, _buttons, EPopupKind_Generic, _callback, 0.0f);
    }

    //-------------------------------------------------------------------------
    AZ::u32 MessagePopupSystemComponent::ShowToasterPopup(const AZStd::string& _message, float _showTime)
    {
        return InternalShowPopup(_message, EPopupButtons_NoButtons, EPopupKind_Toaster, nullptr, _showTime);
    }

    //-------------------------------------------------------------------------
    AZ::u32 MessagePopupSystemComponent::ShowToasterPopupWithCallback(const AZStd::string& _message, float _showTime, AZStd::function<void(int _button)> _callback)
    {
        return InternalShowPopup(_message, EPopupButtons_NoButtons, EPopupKind_Toaster, _callback, _showTime);
    }

    //-------------------------------------------------------------------------
    AZ::u32 MessagePopupSystemComponent::InternalShowPopup(const AZStd::string& _message, EPopupButtons _buttons, EPopupKind _kind, AZStd::function<void(int _button)> _callback, float _showTime)
    {
        void *clientData = nullptr;
        
        AZ::u32 popupID = m_PopupsManager.CreatePopup();

        // First try to send the request to a valid MessagePopup implementation.
        // If none consumed it and return the clientID then use the platform specific default popup
        MessagePopup::MessagePopupImplBus::Broadcast(&MessagePopup::MessagePopupImplBus::Events::OnShowPopup, popupID, _message, _buttons, _kind, _callback, &clientData);
        
        if (clientData != nullptr)
        {
            m_PopupsManager.SetPopupData(popupID, clientData, _callback, _showTime);
        }
        else
        {
            bool res = ShowNativePopup(_message, _buttons, _kind, _callback);
            
            if (res)
            {
                m_PopupsManager.SetPopupData(popupID, nullptr, _callback, _showTime);
            }
            else
            {
                m_PopupsManager.RemovePopup(popupID);
                return InvalidId;
            }
        }
        return popupID;
    }

    //-------------------------------------------------------------------------
    bool MessagePopupSystemComponent::ShowNativePopup(const AZStd::string& _message, MessagePopup::EPopupButtons _buttons, [[maybe_unused]] MessagePopup::EPopupKind _kind, AZStd::function<void(int _button)> _callback)
    {
        switch (_buttons)
        {
        case MessagePopup::EPopupButtons_NoButtons:
            AZ_Assert(false, "Invalid option since we cannot Hide a native Popup");
            return false;
        case MessagePopup::EPopupButtons_Confirm:
            AZ::NativeUI::NativeUIRequestBus::Broadcast(&AZ::NativeUI::NativeUIRequestBus::Events::DisplayOkDialog, "", _message.c_str(), true);
            if (_callback)
            {
                _callback(0);
            }
            return true;
        case MessagePopup::EPopupButtons_YesNo:
        {
            AZStd::string resultStr;
            AZ::NativeUI::NativeUIRequestBus::BroadcastResult(resultStr, &AZ::NativeUI::NativeUIRequestBus::Events::DisplayYesNoDialog, "", _message.c_str(), false);
            if (_callback)
            {
                if (resultStr == "Yes")
                    _callback(0);
                else
                    _callback(1);
            }
            return true;
        }
        }
        // return true means we consumed it
        return false;
    }

    //-------------------------------------------------------------------------
    bool MessagePopupSystemComponent::HidePopup(AZ::u32 _popupID, int _buttonPressed)
    {
        MessagePopupInfo* popupInfo = m_PopupsManager.GetPopupInfo(_popupID);
        if (popupInfo == nullptr)
        {
            return false;
        }
        void *clientData = m_PopupsManager.GetPopupClientData(_popupID);

        // First try to send the request to a valid MessagePopup implementation.
        // If none consumed it and return the clientID then use the platform specific default popup
        MessagePopup::MessagePopupImplBus::Broadcast(&MessagePopup::MessagePopupImplBus::Events::OnHidePopup, *popupInfo);
        if (clientData == nullptr)
        {
            // No way to remove the native UI 
            AZ_Assert(false, "Invalid option since we cannot Hide a native Popup.");
        }

        // Process the callback function
        if (popupInfo->IsValid() && popupInfo->m_callback)
        {
            popupInfo->m_callback(_buttonPressed);
        }

        m_PopupsManager.RemovePopup(_popupID);

        return true;
    }

    //-------------------------------------------------------------------------
    AZ::u32 MessagePopupSystemComponent::GetNumActivePopups() const
    {
        return m_PopupsManager.GetNumActivePopups();
    }
}
