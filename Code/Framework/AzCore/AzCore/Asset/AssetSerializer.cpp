/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/IO/SystemFile.h>

namespace AZ {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    const Uuid& GetAssetClassId()
    {
        static Uuid s_typeId("{77A19D40-8731-4d3c-9041-1B43047366A4}");
        return s_typeId;
    }
    //-------------------------------------------------------------------------
    AssetSerializer AssetSerializer::s_serializer;
    //-------------------------------------------------------------------------

    size_t AssetSerializer::DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/)
    {
        (void)isDataBigEndian;
        const size_t dataSize = sizeof(Data::AssetId) + sizeof(Data::AssetType);
        AZ_Assert(in.GetLength() >= dataSize, "Invalid data in stream");
        (void)dataSize;

        Data::AssetId assetId;
        Data::AssetType assetType;
        Data::AssetLoadBehavior assetLoadBehavior;
        size_t hintSize = 0;
        AZStd::string assetHint;
        in.Read(sizeof(Data::AssetId), reinterpret_cast<void*>(&assetId));
        in.Read(sizeof(assetType), reinterpret_cast<void*>(&assetType));
        in.Read(sizeof(size_t), reinterpret_cast<void*>(&hintSize));
        AZ_SERIALIZE_SWAP_ENDIAN(assetId.m_subId, isDataBigEndian);
        AZ_SERIALIZE_SWAP_ENDIAN(hintSize, isDataBigEndian);
        assetHint.resize(hintSize);
        in.Read(hintSize, reinterpret_cast<void*>(assetHint.data()));
        in.Read(sizeof(assetLoadBehavior), reinterpret_cast<void*>(&assetLoadBehavior));
        AZ_SERIALIZE_SWAP_ENDIAN(assetLoadBehavior, isDataBigEndian);

        AZStd::string outText = AZStd::string::format("id=%s,type=%s,hint={%s},loadBehavior=%u",
            assetId.ToString<AZStd::string>().c_str(), assetType.ToString<AZStd::string>().c_str(), assetHint.c_str(),
            aznumeric_cast<u32>(assetLoadBehavior));
        return static_cast<size_t>(out.Write(outText.size(), outText.c_str()));
    }

    //-------------------------------------------------------------------------

    bool AssetSerializer::Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian)
    {
        AZ_Assert(classPtr, "AssetSerializer::Load received invalid data pointer.");

        using namespace AZ::Data;

        (void)isDataBigEndian;

        // version 0 just has asset Id and type
        size_t dataSize = sizeof(AssetId) + sizeof(AssetType);
        // version 1 adds asset hint
        if (version > 0)
        {
            dataSize += sizeof(IO::SizeType); // There must be at least enough room for the hint length
        }
        // version 2 adds asset auto load behavior
        if (version > 1)
        {
            dataSize += sizeof(AZ::Data::AssetLoadBehavior);
        }
        if (stream.GetLength() < dataSize)
        {
            return false;
        }

        AssetId assetId = AssetId();
        AssetType assetType = AssetType::CreateNull();
        Data::AssetLoadBehavior assetLoadBehavior = Data::AssetLoadBehavior::Default;
        AZStd::string assetHint;

        stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

        AZ::IO::SizeType bytesRead = 0;
        bytesRead += stream.Read(sizeof(assetId), reinterpret_cast<void*>(&assetId));
        AZ_SERIALIZE_SWAP_ENDIAN(assetId.m_subId, isDataBigEndian);
        bytesRead += stream.Read(sizeof(assetType), reinterpret_cast<void*>(&assetType));
        if (version > 0)
        {
            IO::SizeType hintSize = 0;
            bytesRead += stream.Read(sizeof(hintSize), reinterpret_cast<void*>(&hintSize));
            AZ_SERIALIZE_SWAP_ENDIAN(hintSize, isDataBigEndian);
            AZ_Warning("Asset", hintSize < AZ_MAX_PATH_LEN, "Invalid asset hint, will be truncated");
            hintSize = AZStd::min<size_t>(hintSize, AZ_MAX_PATH_LEN);
            assetHint.resize(hintSize);
            dataSize += hintSize;
            bytesRead += stream.Read(hintSize, reinterpret_cast<void*>(assetHint.data()));
        }
        if (version > 1)
        {
            bytesRead += stream.Read(sizeof(assetLoadBehavior), reinterpret_cast<void*>(&assetLoadBehavior));
            AZ_SERIALIZE_SWAP_ENDIAN(assetLoadBehavior, isDataBigEndian);
        }
        
        AZ_Assert(bytesRead == dataSize, "Invalid asset type/read");
        (void)bytesRead;


        Asset<AssetData>* asset = reinterpret_cast<Asset<AssetData>*>(classPtr);
        
        asset->m_assetId = assetId;
        asset->m_assetType = assetType;
        asset->m_assetHint = assetHint;

        // Only overwrite the AutoLoad Behavior if a saved value existed.  This preserves the behavior of letting the
        // asset class constructor set a default value at runtime if no written value exists.
        if (version > 1)
        {
            asset->SetAutoLoadBehavior(assetLoadBehavior);
        }
        asset->UpgradeAssetInfo();
   
        return true;
    }

    //-------------------------------------------------------------------------

    bool AssetSerializer::LoadWithFilter(void* classPtr, IO::GenericStream& stream, unsigned int version, const Data::AssetFilterCB& assetFilterCallback, bool isDataBigEndian)
    {
        if (Load(classPtr, stream, version, isDataBigEndian))
        {
            Data::Asset<Data::AssetData>* asset = reinterpret_cast<Data::Asset<Data::AssetData>*>(classPtr);
            return PostSerializeAssetReference(*asset, assetFilterCallback);
        }

        return false;
    }

    //-------------------------------------------------------------------------

    void AssetSerializer::Clone(const void* sourcePtr, void* destPtr)
    {
        AZ_Assert(sourcePtr, "AssetSerializer::Clone received invalid source pointer.");
        AZ_Assert(destPtr, "AssetSerializer::Clone received invalid destination pointer.");
            
        const Data::Asset<Data::AssetData>* sourceAsset = reinterpret_cast<const Data::Asset<Data::AssetData>*>(sourcePtr);
        Data::Asset<Data::AssetData>* destAsset = reinterpret_cast<Data::Asset<Data::AssetData>*>(destPtr);

        *destAsset = *sourceAsset;
    }

    //-------------------------------------------------------------------------

    size_t  AssetSerializer::TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian)
    {
        (void)isDataBigEndian;
        // Parse the asset id and type
        const char* idGuidStart = strchr(text, '{');
        AZ_Assert(idGuidStart, "Invalid asset guid data! %s", text);
        const char* idGuidEnd = strchr(idGuidStart, ':');
        AZ_Assert(idGuidEnd, "Invalid asset guid data! %s", idGuidStart);
        const char* idSubIdStart = idGuidEnd + 1;
        const char* idSubIdEnd = strchr(idSubIdStart, ',');
        AZ_Assert(idSubIdEnd, "Invalid asset subId data! %s", idSubIdStart);
        const char* idTypeStart = strchr(idSubIdEnd, '{');
        AZ_Assert(idTypeStart, "Invalid asset type data! %s", idSubIdEnd);
        const char* idTypeEnd = strchr(idTypeStart, '}');
        AZ_Assert(idTypeEnd, "Invalid asset type data! %s", idTypeStart);
        idTypeEnd++;

        AZStd::string assetHint;
        Data::AssetLoadBehavior assetLoadBehavior = Data::AssetLoadBehavior::PreLoad;

        // Read hint for version >= 1
        if (textVersion > 0)
        {
            const char* hintStart = strchr(idTypeEnd, '{');
            AZ_Assert(hintStart, "Invalid asset hint data! %s", idTypeEnd);
            const char* hintEnd = strchr(hintStart, '}');
            AZ_Assert(hintEnd, "Invalid asset hint data! %s", hintStart);
            assetHint.assign(hintStart+1, hintEnd);

            // Read loadBehavior for version >= 2
            if (textVersion > 1)
            {
                const char* loadBehaviorStart = strchr(hintEnd, '=');
                AZ_Assert(loadBehaviorStart, "Invalid asset load behavior data! %s", loadBehaviorStart);
                assetLoadBehavior = static_cast<Data::AssetLoadBehavior>(strtoul(loadBehaviorStart+1, nullptr, 16));
            }
        }
        
        Data::AssetId assetId;
        assetId.m_guid = Uuid::CreateString(idGuidStart, idGuidEnd - idGuidStart);
        assetId.m_subId = static_cast<u32>(strtoul(idSubIdStart, nullptr, 16));
        Data::AssetType assetType = Uuid::CreateString(idTypeStart, idTypeEnd - idTypeStart);

        Data::Asset<Data::AssetData> asset(assetId, assetType, assetHint);

        // Only overwrite the AutoLoad Behavior if a saved value existed.  This preserves the behavior of letting the
        // asset class constructor set a default value at runtime if no written value exists.
        if (textVersion > 1)
        {
            asset.SetAutoLoadBehavior(assetLoadBehavior);
        }

        return Save(&asset, stream, isDataBigEndian);
    }

    //-------------------------------------------------------------------------

    size_t AssetSerializer::Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian)
    {
        (void)isDataBigEndian;

        const Data::Asset<Data::AssetData>* asset = reinterpret_cast<const Data::Asset<Data::AssetData>*>(classPtr);

        AZ_Assert(asset->Get() == nullptr || asset->GetType() != AzTypeInfo<Data::AssetData>::Uuid(), 
            "Asset contains data, but does not have a valid asset type.");

        Data::AssetId assetId = asset->GetId();
        Data::AssetType assetType = asset->GetType();
        const AZStd::string& assetHint = asset->GetHint();
        IO::SizeType assetHintSize = assetHint.size();
        Data::AssetLoadBehavior assetLoadBehavior = asset->GetAutoLoadBehavior();

        AZ_SERIALIZE_SWAP_ENDIAN(assetId.m_subId, isDataBigEndian);
        AZ_SERIALIZE_SWAP_ENDIAN(assetHintSize, isDataBigEndian);
        AZ_SERIALIZE_SWAP_ENDIAN(assetLoadBehavior, isDataBigEndian);

        stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        size_t bytesWritten = static_cast<size_t>(stream.Write(sizeof(Data::AssetId), reinterpret_cast<void*>(&assetId)));
        bytesWritten += static_cast<size_t>(stream.Write(sizeof(Data::AssetType), reinterpret_cast<void*>(&assetType)));
        bytesWritten += static_cast<size_t>(stream.Write(sizeof(assetHintSize), reinterpret_cast<void*>(&assetHintSize)));
        bytesWritten += static_cast<size_t>(stream.Write(assetHint.size(), assetHint.c_str()));
        bytesWritten += static_cast<size_t>(stream.Write(sizeof(Data::AssetLoadBehavior), reinterpret_cast<void*>(&assetLoadBehavior)));
        return bytesWritten;
    }

    //-------------------------------------------------------------------------

    bool AssetSerializer::PostSerializeAssetReference(AZ::Data::Asset<AZ::Data::AssetData>& asset, const Data::AssetFilterCB& assetFilterCallback)
    {
        if (!asset.GetId().IsValid())
        {
            // The asset reference is null, so there's no additional processing required.
            return true;
        }
        
        if (assetFilterCallback && !assetFilterCallback(AZ::Data::AssetFilterInfo(asset)))
        {
            // This asset reference is filtered out for further processing/loading.
            // we are allowed to bind it to assets that are already loaded.
            Data::AssetId assetId = asset.GetId();

            if (assetId.IsValid() && asset.GetType() != Data::s_invalidAssetType)
            {
                // Valid populated asset pointer. If the asset has already been constructed and/or loaded, acquire a pointer.
                if (Data::AssetManager::IsReady())
                {
                    Data::Asset<Data::AssetData> existingAsset = Data::AssetManager::Instance().FindAsset(assetId, asset.GetAutoLoadBehavior());
                    if (existingAsset)
                    {
                        asset = existingAsset;
                    }
                }
            }

            return true;
        }

        RemapLegacyIds(asset);

        if (asset.Get())
        {
            // Asset reference is already fully populated.
            return true;
        }

        const Data::AssetLoadBehavior loadBehavior = asset.GetAutoLoadBehavior();

        if (loadBehavior == Data::AssetLoadBehavior::NoLoad)
        {
            // Asset reference is flagged to never load unless explicitly by user code.
            return true;
        }

        // Save this in case GetAsset() fails
        Data::AssetId assetId = asset.GetId();
        Data::AssetType assetType = asset.GetType();
        const bool blockingLoad = loadBehavior == Data::AssetLoadBehavior::PreLoad;

        // Get the asset and start loading
        asset = Data::AssetManager::Instance().GetAsset(assetId, assetType, loadBehavior, Data::AssetLoadParameters{ assetFilterCallback });

        if (!asset.GetId().IsValid()) // This will happen if there is no asset handler registered
        {
            AZ_Error("Serialization", false, "Dependent asset (%s) could not be loaded.", assetId.ToString<AZStd::string>().c_str());
            return false;
        }

        // If the asset is flagged to pre-load, kick off a blocking load.
        if (blockingLoad)
        {   
            asset.BlockUntilLoadComplete();

            if (asset.IsError())
            {
                AZ_Error("Serialization", false, "Dependent asset (%s:%s) could not be loaded.",
                    asset.GetId().ToString<AZStd::string>().c_str(),
                    asset.GetHint().c_str());
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void AssetSerializer::RemapLegacyIds(AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, asset.GetId());
        if (assetInfo.m_assetId.IsValid())
        {
            asset.m_assetId = assetInfo.m_assetId;
            asset.m_assetHint = assetInfo.m_relativePath;
        }
    }
    //-------------------------------------------------------------------------

    bool AssetSerializer::CompareValueData(const void* lhs, const void* rhs)
    {
        return SerializeContext::EqualityCompareHelper<Data::Asset<Data::AssetData>>::CompareValues(lhs, rhs);
    }

    //-------------------------------------------------------------------------
}   // namespace AZ
