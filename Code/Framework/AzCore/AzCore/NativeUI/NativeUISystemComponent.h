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

namespace AZ::NativeUI
{
    class NativeUISystem
        : public NativeUIRequestBus::Handler
    {
    public:
        AZ_RTTI(NativeUISystem, "{FF534B2C-11BE-4DEA-A5B7-A4FA96FE1EDE}", NativeUIRequests);
        AZ_CLASS_ALLOCATOR(NativeUISystem, AZ::OSAllocator, 0);

        NativeUISystem();
        ~NativeUISystem() override;

        ////////////////////////////////////////////////////////////////////////
        // NativeUIRequestBus interface implementation
        AZStd::string DisplayBlockingDialog(const AZStd::string& title, const AZStd::string& message, const AZStd::vector<AZStd::string>& options) const override;
        AZStd::string DisplayOkDialog(const AZStd::string& title, const AZStd::string& message, bool showCancel) const override;
        AZStd::string DisplayYesNoDialog(const AZStd::string& title, const AZStd::string& message, bool showCancel) const override;
        AssertAction DisplayAssertDialog(const AZStd::string& message) const override;
        ////////////////////////////////////////////////////////////////////////
    };
} // namespace AZ::NativeUI
