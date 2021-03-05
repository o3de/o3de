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

#include <AzCore/Component/Component.h>
#include <AzCore/NativeUI/NativeUIRequests.h>

namespace AZ
{
    namespace NativeUI
    {
        class NativeUISystemComponent
            : public AZ::Component
            , public NativeUIRequestBus::Handler
        {
        public:
            AZ_COMPONENT(NativeUISystemComponent, "{E996C058-4AFE-4C8C-816F-98D864D8576D}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            ////////////////////////////////////////////////////////////////////////
            // NativeUIRequestBus interface implementation
            AZStd::string DisplayBlockingDialog(const AZStd::string& title, const AZStd::string& message, const AZStd::vector<AZStd::string>& options) const override;
            AZStd::string DisplayOkDialog(const AZStd::string& title, const AZStd::string& message, bool showCancel) const override;
            AZStd::string DisplayYesNoDialog(const AZStd::string& title, const AZStd::string& message, bool showCancel) const override;
            AssertAction DisplayAssertDialog(const AZStd::string& message) const override;
            ////////////////////////////////////////////////////////////////////////

        protected:

            ////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////
        };
    }
}
