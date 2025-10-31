/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/System/AnyAsset.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void AnyAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }        

        bool AnyAssetCreator::CreateAnyAsset(const AZStd::any& anyData, const Data::AssetId& assetId, Data::Asset<AnyAsset>& result)
        {
            result = Data::AssetManager::Instance().CreateAsset<AnyAsset>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
            result->m_data = anyData;
            return true;
        }

        void AnyAssetCreator::SetAnyAssetData(const AZStd::any& anyData, AnyAsset& result)
        {
            result.m_data = anyData;
        }

        Data::AssetHandler::LoadResult AnyAssetHandler::LoadAssetData(
            const Data::Asset<Data::AssetData>& asset,
            AZStd::shared_ptr<Data::AssetDataStream> stream,
            const Data::AssetFilterCB& assetLoadFilterCB)
        {
            AnyAsset* assetData = asset.GetAs<AnyAsset>();

            AZ_Assert(assetData, "Asset is of the wrong type.");

            SerializeContext* context = m_serializeContext;
            if (!context)
            {
                ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationRequests::GetSerializeContext);
            }

            if (!context)
            {
                AZ_Assert(false, "No serialize context");
                return Data::AssetHandler::LoadResult::Error;
            }

            Uuid loadedClassId;
            void* loadedInstance = nullptr;
            bool success = ObjectStream::LoadBlocking(stream.get(), *context,
                [&loadedInstance, &loadedClassId](void* classPtr, const Uuid& classId, const SerializeContext*)
                {
                    loadedClassId = classId;
                    loadedInstance = classPtr;
                },
                AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB),
                ObjectStream::InplaceLoadRootInfoCB()
            );

            if (!success)
            {
                return Data::AssetHandler::LoadResult::Error;
            }

            // Create temporary one with the type id then we can get its type info to construct a new AZStd::any with new data
            auto tempAny = context->CreateAny(loadedClassId);
            AZStd::any::type_info typeInfo = tempAny.get_type_info();
            AZStd::any newAny(loadedInstance, typeInfo);
            assetData->m_data = newAny;

            // Release loadedInstance
            auto classData = context->FindClassData(loadedClassId);
            if (classData && classData->m_factory)
            {
                classData->m_factory->Destroy(loadedInstance);
            }
             
            return Data::AssetHandler::LoadResult::LoadComplete;
        }

        bool AnyAssetHandler::SaveAssetData(const Data::Asset<Data::AssetData>& asset, IO::GenericStream* stream)
        {
            AnyAsset* assetData = asset.GetAs<AnyAsset>();

            AZ_Assert(assetData, "Asset is of the wrong type.");

            SerializeContext* context = m_serializeContext;
            if (!context)
            {
                ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationRequests::GetSerializeContext);
            }

            if (!context)
            {
                AZ_Assert(false, "No serialize context");
                return false;
            }

            if (assetData)
            {
                return Utils::SaveObjectToStream(*stream, ObjectStream::ST_XML, assetData->GetDataAs<void>(),
                    assetData->m_data.get_type_info().m_id, context);
            }

            return false;
        }
    } // namespace RPI
} // namespace AZ
