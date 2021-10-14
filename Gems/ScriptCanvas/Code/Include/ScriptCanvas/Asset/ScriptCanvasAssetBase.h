/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Editor/Include/ScriptCanvas/Assets/ScriptCanvasAssetBus.h>
#include <Editor/Include/ScriptCanvas/Assets/ScriptCanvasBaseAssetData.h>
#include <ScriptCanvas/Asset/AssetDescription.h>

namespace ScriptCanvas
{
    class ScriptCanvasAssetBase
        : public AZ::Data::AssetData
        , public AZStd::enable_shared_from_this<ScriptCanvasAssetBase>
        , ScriptCanvas::ScriptCanvasAssetBusRequestBus::Handler
    {

    public:
        AZ_RTTI(ScriptCanvasAssetBase, "{D07DBDE4-A169-4650-871B-FC75AFEEB03E}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetBase, AZ::SystemAllocator, 0);

        ScriptCanvasAssetBase(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom()),
            AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, status)
        {
            ScriptCanvas::ScriptCanvasAssetBusRequestBus::Handler::BusConnect(GetId());
        }

        virtual ~ScriptCanvasAssetBase()
        {
            delete m_data;
            ScriptCanvas::ScriptCanvasAssetBusRequestBus::Handler::BusDisconnect();
        }

        template <typename DataType>
        DataType* GetScriptCanvasDataAs() 
        {
            return azrtti_cast<DataType*>(m_data);
        }

        template <typename DataType>
        const DataType* GetScriptCanvasDataAs() const
        {
            return azrtti_cast<DataType*>(m_data);
        }

        virtual ScriptCanvasData& GetScriptCanvasData()
        {
            return *m_data;
        }

        virtual const ScriptCanvasData& GetScriptCanvasData() const
        {
            return *m_data;
        }

        AZ::Entity* GetScriptCanvasEntity() const
        {
            return m_data->m_scriptCanvasEntity.get();
        }

        virtual void SetScriptCanvasEntity(AZ::Entity* scriptCanvasEntity)
        {
            if (m_data->m_scriptCanvasEntity.get() != scriptCanvasEntity)
            {
                m_data->m_scriptCanvasEntity.reset(scriptCanvasEntity);
            }
        }

        virtual ScriptCanvas::AssetDescription GetAssetDescription() const = 0;

    protected:

        ScriptCanvasData* m_data;

        void SetAsNewAsset() override
        {
            m_status = AZ::Data::AssetData::AssetStatus::Ready;
        }

    };
}
