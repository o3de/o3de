/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiDynamicScrollBoxBus.h>

#include <AzCore/Component/Component.h>

namespace LyShineExamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! This component associates dynamic data with the dynamic scroll box in the UiComponents
    //! level
    class UiTestScrollBoxDataProviderComponent
        : public AZ::Component
        , public UiDynamicScrollBoxDataBus::Handler
        , public UiDynamicScrollBoxElementNotificationBus::Handler
    {
    public: // member functions

        AZ_COMPONENT(UiTestScrollBoxDataProviderComponent, "{C66A6BBF-D715-4876-8302-D452CC6975C8}", AZ::Component);

        UiTestScrollBoxDataProviderComponent();
        ~UiTestScrollBoxDataProviderComponent() override;

        // UiDynamicScrollBoxDataInterface
        virtual int GetNumElements() override;
        // ~UiDynamicScrollBoxDataInterface

        // UiDynamicScrollBoxElementNotifications
        virtual void OnElementBecomingVisible(AZ::EntityId entityId, int index) override;
        // ~UiDynamicScrollBoxElementNotifications

    protected: // static member functions

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("UiDynamicContentProviderService", 0xe25f3f73));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("UiDynamicContentProviderService", 0xe25f3f73));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("UiDynamicScrollBoxService", 0x11112f1a));
            required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
            required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
        }

        static void Reflect(AZ::ReflectContext* context);

    protected: // member functions

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        // ~AZ::Component

        AZ_DISABLE_COPY_MOVE(UiTestScrollBoxDataProviderComponent);

    protected: // data
    };
}
