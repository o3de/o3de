/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceBus.h>

#include <AzToolsFramework/Slice/SliceRequestBus.h>

namespace AzToolsFramework
{
    //! Component in charge of listening for SliceRequestBus requests
    class SliceRequestComponent
        : public AZ::Component
        , protected SliceRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SliceRequestComponent, "{7E3DFACD-DC40-45EE-9B66-DBE73A8553CF}");

        static void Reflect(AZ::ReflectContext* context);

    protected:
        //! Component overrides
        void Activate() override;
        void Deactivate() override;

        //! AzToolsFramework::SliceRequestBus overrides ...
        bool IsSliceDynamic(const AZ::Data::AssetId& assetId) override;
        void SetSliceDynamic(const AZ::Data::AssetId& assetId, bool isDynamic) override;
    };
} // namespace AzToolsFramework
