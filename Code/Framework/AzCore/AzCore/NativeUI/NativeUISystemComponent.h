/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(NativeUISystem, AZ::OSAllocator);

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
