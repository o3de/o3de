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

#include <AzCore/Asset/AssetCommon.h>
#include <Editor/Include/ScriptCanvas/Assets/ScriptCanvasAssetBus.h>
#include <Editor/Include/ScriptCanvas/Assets/ScriptCanvasBaseAssetData.h>

#include <ScriptCanvas/Asset/AssetDescription.h>

namespace ScriptCanvas
{
    class ScriptCanvasAssetBase
        : public AZ::Data::AssetData
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


} // namespace ScriptCanvas
