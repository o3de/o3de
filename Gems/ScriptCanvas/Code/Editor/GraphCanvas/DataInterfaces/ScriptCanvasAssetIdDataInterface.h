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

#include <GraphCanvas/Components/NodePropertyDisplay/AssetIdDataInterface.h>

#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasDataInterface.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Slice/SliceAsset.h>

#include <QWidget>
#include <QMenu>
#include <QAction>

namespace ScriptCanvasEditor
{
    class ScriptCanvasAssetIdDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::AssetIdDataInterface>
    {

    public:

        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetIdDataInterface, AZ::SystemAllocator, 0);
        ScriptCanvasAssetIdDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }

        ~ScriptCanvasAssetIdDataInterface() = default;

        // AssetIdDataInterface
        AZ::Data::AssetId GetAssetId() const override
        {
            const ScriptCanvas::Datum* object = GetSlotObject();

            if (object)
            {
                const AZ::Data::AssetId* retVal = object->GetAs<AZ::Data::AssetId>();

                if (retVal)
                {
                    return (*retVal);
                }
            }

            return AZ::Data::AssetId();
        }

        void SetAssetId(const AZ::Data::AssetId& assetId) override
        {
            ScriptCanvas::ModifiableDatumView datumView;
            ModifySlotObject(datumView);

            if (datumView.IsValid())
            {
                datumView.SetAs(assetId);

                PostUndoPoint();
                PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
            }
        }

        AZ::Data::AssetType GetAssetType() const override
        {
            return AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid(); //hardcoded for DynamicSliceAsset until the slot definitions can be queried for this
        }

        AZStd::string GetStringFilter() const override
        {
            return AZ::DynamicSliceAsset::GetFileFilter(); //hardcoded for DynamicSliceAsset until the slot definitions can be queried for this
        }
        ////
    };
}
