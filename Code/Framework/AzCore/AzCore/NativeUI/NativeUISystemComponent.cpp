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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/NativeUI/NativeUISystemComponent.h>

namespace AZ
{
    using namespace AZ::NativeUI;

    void NativeUISystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<NativeUISystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<NativeUISystemComponent>("NativeUI", "Adds basic support for native (platform specific) UI dialog boxes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void NativeUISystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("NativeUIService", 0x8ec25f87));
    }

    void NativeUISystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("NativeUIService", 0x8ec25f87));
    }

    void NativeUISystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void NativeUISystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    AssertAction NativeUISystemComponent::DisplayAssertDialog(const AZStd::string& message) const
    {
        static const char* buttonNames[3] = { "Ignore", "Ignore All", "Break" };
        AZStd::vector<AZStd::string> options;
        options.push_back(buttonNames[0]);
#if AZ_TRAIT_SHOW_IGNORE_ALL_ASSERTS_OPTION
        options.push_back(buttonNames[1]);
#endif
        options.push_back(buttonNames[2]);
        AZStd::string result;
        result = DisplayBlockingDialog("Assert Failed!", message, options);

        if (result.compare(buttonNames[0]) == 0)
            return AssertAction::IGNORE_ASSERT;
        else if (result.compare(buttonNames[1]) == 0)
            return AssertAction::IGNORE_ALL_ASSERTS;
        else if (result.compare(buttonNames[2]) == 0)
            return AssertAction::BREAK;

        return AssertAction::NONE;
    }

    AZStd::string NativeUISystemComponent::DisplayOkDialog(const AZStd::string& title, const AZStd::string& message, bool showCancel) const
    {
        AZStd::vector<AZStd::string> options;

        options.push_back("OK");
        if (showCancel)
        {
            options.push_back("Cancel");
        }

        return DisplayBlockingDialog(title, message, options);
    }

    AZStd::string NativeUISystemComponent::DisplayYesNoDialog(const AZStd::string& title, const AZStd::string& message, bool showCancel) const
    {
        AZStd::vector<AZStd::string> options;

        options.push_back("Yes");
        options.push_back("No");
        if (showCancel)
        {
            options.push_back("Cancel");
        }

        return DisplayBlockingDialog(title, message, options);
    }

    void NativeUISystemComponent::Init()
    {
    }

    void NativeUISystemComponent::Activate()
    {
        NativeUIRequestBus::Handler::BusConnect();
    }

    void NativeUISystemComponent::Deactivate()
    {
        NativeUIRequestBus::Handler::BusDisconnect();
    }
}
