/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Slice/SliceInstantiationTicket.h>

namespace AzToolsFramework
{
    //! Bus for making slice requests.
    class SliceRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~SliceRequests() {}

        //! EBusTraits overrides ...
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Return if a given slice by AssetId is dynamic
        virtual bool IsSliceDynamic(const AZ::Data::AssetId& assetId) = 0;

        //! Set if a slice asset is dynamic and re-saves the slice
        virtual void SetSliceDynamic(const AZ::Data::AssetId& assetId, bool isDynamic) = 0;

        //! Instantiate a slice by AssetId at the given transform
        virtual AzFramework::SliceInstantiationTicket InstantiateSliceFromAssetId(const AZ::Data::AssetId& assetId, const AZ::Transform& transform) = 0;

        //! Create a new slice asset from the given Entity
        virtual bool CreateNewSlice(const AZ::EntityId& entityId, const char* assetPath) = 0;

        virtual void ShowPushDialog(const EntityIdList& entityIds) = 0;
    };

    using SliceRequestBus = AZ::EBus<SliceRequests>;
} // namespace AzToolsFramework
