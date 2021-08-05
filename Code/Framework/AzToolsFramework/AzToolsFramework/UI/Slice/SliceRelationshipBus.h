/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    /**
    * Event bus for slice relationship widget.
    */
    class SliceRelationshipRequests
        : public AZ::EBusTraits
    {
    public:
        /// Notifies SliceRelationshipWidget when to change view to the requested asset id
        virtual void OnSliceRelationshipViewRequested(const AZ::Data::AssetId& /*assetId*/) = 0;
    };

    using SliceRelationshipRequestBus = AZ::EBus<SliceRelationshipRequests>;
}
