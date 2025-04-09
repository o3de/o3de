/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Pipeline/HeightFieldAssetHandler.h>

#include <Pipeline/HeightFieldAssetHandler.h>
#include <Pipeline/StreamWrapper.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <PhysX/HeightFieldAsset.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/ComponentTypeIds.h>
#include <PxPhysicsAPI.h>

#include <extensions/PxSerialization.h>
#include <extensions/PxDefaultStreams.h>

namespace PhysX::Pipeline
{
    const char* HeightFieldAssetHandler::s_assetFileExtension = "pxheightfield";

    HeightFieldAssetHandler::HeightFieldAssetHandler()
    {
        Register();
    }

    HeightFieldAssetHandler::~HeightFieldAssetHandler()
    {
        Unregister();
    }

    void HeightFieldAssetHandler::Register()
    {
        bool assetManagerReady = AZ::Data::AssetManager::IsReady();
        AZ_Error("PhysX HeightField Asset", assetManagerReady, "Asset manager isn't ready.");
        if (assetManagerReady)
        {
            AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<HeightFieldAsset>::Uuid());
        }

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<HeightFieldAsset>::Uuid());
    }

    void HeightFieldAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect();

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    // AZ::AssetTypeInfoBus
    AZ::Data::AssetType HeightFieldAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<HeightFieldAsset>::Uuid();
    }

    void HeightFieldAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(HeightFieldAssetHandler::s_assetFileExtension);
    }

    const char* HeightFieldAssetHandler::GetAssetTypeDisplayName() const
    {
        return "PhysX HeightField Mesh";
    }

    const char* HeightFieldAssetHandler::GetBrowserIcon() const
    {
        return "Icons/Components/PhysXHeightfieldCollider.svg";
    }

    const char* HeightFieldAssetHandler::GetGroup() const
    {
        return "Physics";
    }

    AZ::Uuid HeightFieldAssetHandler::GetComponentTypeId() const
    {
        return PhysX::EditorTerrainComponentTypeId;
    }

    // AZ::Data::AssetHandler
    AZ::Data::AssetPtr HeightFieldAssetHandler::CreateAsset([[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        if (type == AZ::AzTypeInfo<HeightFieldAsset>::Uuid())
        {
            return aznew HeightFieldAsset();
        }

        AZ_Error("PhysX HeightField Asset", false, "This handler deals only with PhysXHeightFieldAsset type.");
        return nullptr;
    }

    AZ::Data::AssetHandler::LoadResult HeightFieldAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        AZ_PROFILE_FUNCTION(Physics);

        HeightFieldAsset* physXHeightFieldAsset = asset.GetAs<HeightFieldAsset>();
        if (!physXHeightFieldAsset)
        {
            AZ_Error("PhysX HeightField Asset", false, "This should be a PhysX HeightField Asset, as this is the only type we process.");
            return AZ::Data::AssetHandler::LoadResult::Error;
        }

        // Wrap az stream behind physx interface
        PhysX::AssetDataStreamWrapper readerStream(stream);

        // Read the file header
        HeightFieldAssetHeader header;
        readerStream.read(&header, sizeof(header));

        // Parse the asset versions
        if (header.m_assetVersion >= 1)
        {
            if (header.m_assetDataSize > 0)
            {
                readerStream.read(&physXHeightFieldAsset->m_minHeight, sizeof(float));
                readerStream.read(&physXHeightFieldAsset->m_maxHeight, sizeof(float));

                // Create heightfield from cooked file
                physx::PxPhysics& physx = PxGetPhysics();
                physXHeightFieldAsset->SetHeightField(physx.createHeightField(readerStream));

                AZ_Error("PhysX HeightField Asset", physXHeightFieldAsset->m_heightField != nullptr, "Failed to construct PhysX mesh from the cooked data. Possible data corruption.");
                return (physXHeightFieldAsset->m_heightField != nullptr) ?
                    AZ::Data::AssetHandler::LoadResult::LoadComplete :
                    AZ::Data::AssetHandler::LoadResult::Error;
            }
            else
            {
                AZ_Warning("HeightFieldAssetHandler", false, "Empty heightfield file. Try resaving your level");
            }
        }
        else
        {
            AZ_Warning("HeightFieldAssetHandler", false, "Unsupported asset version");
        }

        return AZ::Data::AssetHandler::LoadResult::Error;
    }

    bool HeightFieldAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        AZ_PROFILE_FUNCTION(Physics);

        HeightFieldAsset* physXHeightFieldAsset = asset.GetAs<HeightFieldAsset>();
        if (!physXHeightFieldAsset)
        {
            AZ_Error("PhysX HeightField Asset", false, "This should be a PhysX HeightField Asset. HeightFieldAssetHandler doesn't handle any other asset type.");
            return false;
        }

        physx::PxHeightField* heightField = physXHeightFieldAsset->GetHeightField();
        if (!heightField)
        {
            AZ_Warning("PhysX HeightField Asset", false, "There is no heightfield to save.");
            return false;
        }

        HeightFieldAssetHeader header;
        physx::PxCooking* cooking = nullptr;
        SystemRequestsBus::BroadcastResult(cooking, &SystemRequests::GetCooking);

        // Read samples from heightfield
        AZStd::vector<physx::PxHeightFieldSample> samples;
        samples.resize(heightField->getNbColumns() * heightField->getNbRows());
        heightField->saveCells(samples.data(), (physx::PxU32)samples.size() * heightField->getSampleStride());

        // Read description from heightfield
        physx::PxHeightFieldDesc heightFieldDesc;
        heightFieldDesc.format = heightField->getFormat();
        heightFieldDesc.nbColumns = heightField->getNbColumns();
        heightFieldDesc.nbRows = heightField->getNbRows();
        heightFieldDesc.samples.data = samples.data();
        heightFieldDesc.samples.stride = heightField->getSampleStride();

        // Cook description to file
        physx::PxDefaultMemoryOutputStream writer;
        bool success = cooking->cookHeightField(heightFieldDesc, writer);
        header.m_assetDataSize = writer.getSize() + 2 * sizeof(float);

        PhysX::StreamWrapper writerStream(stream);
        writerStream.write(&header, sizeof(header));
        writerStream.write(&physXHeightFieldAsset->m_minHeight, sizeof(physXHeightFieldAsset->m_minHeight));
        writerStream.write(&physXHeightFieldAsset->m_maxHeight, sizeof(physXHeightFieldAsset->m_maxHeight));
        writerStream.write(writer.getData(), writer.getSize());

        return success;
    }

    void HeightFieldAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void HeightFieldAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<HeightFieldAsset>::Uuid());
    }
} // namespace PhysX::Pipeline
