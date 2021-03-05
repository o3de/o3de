/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <MessagePopup/MessagePopupBus.h>

namespace MessagePopup
{
    class LyShineMessagePopup
        : public AZ::Component
        , protected MessagePopup::MessagePopupImplBus::Handler
        , public UiCanvasNotificationBus::MultiHandler
    {
    public:
        AZ_COMPONENT(LyShineMessagePopup, "{C950D60D-4673-4262-A44D-6A0A1A4DB342}");

        LyShineMessagePopup();
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // MessagePopupImplBus interface implementation
        virtual void OnShowPopup(AZ::u32 _popupID, const AZStd::string& _message, EPopupButtons _buttons, EPopupKind _kind, AZStd::function<void(int _button)> _callback, void** _popupClientID) override;
        virtual void OnHidePopup(const MessagePopupInfo& _popupInfo) override;

        //UiCanvasNotificationBus interface implementation
        void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override;

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        AZStd::unordered_map<AZ::EntityId, AZ::u32> m_activePopupIdsByCanvasId;
    };
}
