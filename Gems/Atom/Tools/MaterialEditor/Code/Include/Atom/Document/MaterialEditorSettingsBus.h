/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/any.h>
#include <AzCore/Outcome/Outcome.h>

namespace MaterialEditor
{
    class MaterialEditorSettingsRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual AZ::Outcome<AZStd::any> GetProperty(AZStd::string_view name) const = 0;
        virtual AZ::Outcome<AZStd::string> GetStringProperty(AZStd::string_view name) const = 0;
        virtual AZ::Outcome<bool> GetBoolProperty(AZStd::string_view name) const = 0;

        virtual void SetProperty(AZStd::string_view name, const AZStd::any& value) = 0;
        virtual void SetStringProperty(AZStd::string_view name, AZStd::string_view stringValue) = 0;
        virtual void SetBoolProperty(AZStd::string_view name, bool boolValue) = 0;
    };
    using MaterialEditorSettingsRequestBus = AZ::EBus<MaterialEditorSettingsRequests>;

    class MaterialEditorSettingsNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnPropertyChanged(AZStd::string_view name, const AZStd::any& value) = 0;
    };
    using MaterialEditorSettingsNotificationBus = AZ::EBus<MaterialEditorSettingsNotifications>;

} // namespace MaterialEditor
