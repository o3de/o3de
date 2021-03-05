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

#include <AzCore/Component/ComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Notification bus for edge related changes
    class EditorWhiteBoxEdgeModifierNotifications : public AZ::EntityComponentBus
    {
    public:
        //! Was the polygon handle changed/updated by the polygon modifier.
        //! @note This will happen during an append/extrusion.
        virtual void OnEdgeModifierUpdatedEdgeHandle(
            [[maybe_unused]] Api::EdgeHandle previousEdgeHandle, [[maybe_unused]] Api::EdgeHandle nextEdgeHandle)
        {
        }

    protected:
        ~EditorWhiteBoxEdgeModifierNotifications() = default;
    };

    using EditorWhiteBoxEdgeModifierNotificationBus = AZ::EBus<EditorWhiteBoxEdgeModifierNotifications>;
} // namespace WhiteBox
