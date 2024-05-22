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

        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetIdDataInterface, AZ::SystemAllocator);
        ScriptCanvasAssetIdDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
            m_assetType = AZ::Uuid::CreateNull();
            m_stringFilter = "*.*";
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
            return m_assetType;
        }

        AZStd::string GetStringFilter() const override
        {
            return m_stringFilter;
        }

        void SetAssetType(AZ::Data::AssetType assetType) override
        {
            m_assetType = assetType;
        }

        void SetStringFilter(const AZStd::string& stringFilter) override
        {
            m_stringFilter = stringFilter;
        }

        AZ::Data::AssetType m_assetType;
        AZStd::string m_stringFilter;
    };
}
