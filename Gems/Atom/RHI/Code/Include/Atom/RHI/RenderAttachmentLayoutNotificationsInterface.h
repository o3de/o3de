/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>

#include <Atom/RHI.Reflect/ScopeId.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>

namespace AZ::RHI
{
    //! This is an optional RHI interface. It is called by the RPI to notify the RHI about RenderAttachmentLayouts
    //! that are being used by a group of AZ::RPI::RasterPass identified by their ScopeId.
    //! In pragmatic terms, this interface should only
    //! be implemented by RHIs that support Subpasses(e.g. Vulkan).
    //! For more details:
    //! https://github.com/o3de/sig-graphics-audio/blob/main/rfcs/SubpassesSupportInRPI/RFC_SubpassesSupportInRPI.md
    class IRenderAttachmentLayoutNotifications
    {
    public:
        AZ_RTTI(IRenderAttachmentLayoutNotifications, "{0432D83C-6EE2-4086-BDB6-7C62BF39458A}");
        AZ_DISABLE_COPY_MOVE(IRenderAttachmentLayoutNotifications);

        IRenderAttachmentLayoutNotifications() = default;
        virtual ~IRenderAttachmentLayoutNotifications() = default;

        //! Reports a list of Subpasses that are using the same RenderAttachmentLayout.
        virtual void SetLayoutForSubpasses(const AZStd::vector<ScopeId>& scopeIds , const RHI::RenderAttachmentLayout& layout) = 0;
    };

    using RenderAttachmentLayoutNotificationsInterface = AZ::Interface<IRenderAttachmentLayoutNotifications>;
} //namespace AZ::RHI
