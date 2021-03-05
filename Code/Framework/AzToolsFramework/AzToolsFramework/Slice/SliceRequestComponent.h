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
        AzFramework::SliceInstantiationTicket InstantiateSliceFromAssetId(const AZ::Data::AssetId& assetId, const AZ::Transform& transform) override;
        bool CreateNewSlice(const AZ::EntityId& entityId, const char* assetPath) override;
        void ShowPushDialog(const EntityIdList& entityIds) override;
    };
} // namespace AzToolsFramework
