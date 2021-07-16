/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <MessagePopup/MessagePopupBus.h>
#include <MessagePopupManager.h>

namespace MessagePopup
{
    class MessagePopupSystemComponent
        : public AZ::Component
        , protected MessagePopupRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MessagePopupSystemComponent, "{C950D60D-4673-4262-A44D-6A0A1A4DB341}");

        MessagePopupSystemComponent();
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // MessagePopupRequestBus interface implementation
        AZ::u32 ShowPopup(const AZStd::string& _message, EPopupButtons _buttons) override;
        AZ::u32 ShowPopupWithCallback(
            const AZStd::string& _message,
            EPopupButtons _buttons,
            AZStd::function<void(int _button)> _callback) override;
        AZ::u32 ShowToasterPopup(const AZStd::string& _message, float _showTime) override;
        AZ::u32 ShowToasterPopupWithCallback(
            const AZStd::string& _message,
            float _showTime,
            AZStd::function<void(int _button)> _callback) override;
        bool HidePopup(AZ::u32 _popupID, int _buttonPressed) override;
        AZ::u32 GetNumActivePopups() const override;

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        AZ::u32 InternalShowPopup(const AZStd::string& _message, EPopupButtons _buttons, EPopupKind _kind, AZStd::function<void(int _button)> _callback, float _showTime);
        bool ShowNativePopup(const AZStd::string& _message, MessagePopup::EPopupButtons _buttons, MessagePopup::EPopupKind _kind, AZStd::function<void(int _button)> _callback);

        MessagePopupManager m_PopupsManager;
    };
}
