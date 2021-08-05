/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
