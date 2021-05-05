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

#include "LmbrCentral_precompiled.h"

#include <AzCore/IO/GenericStreams.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzCore/XML/rapidxml.h>

#include <AzFramework/Archive/IArchive.h>

#include <LmbrCentral/Rendering/LensFlareAsset.h>
#include "LensFlareAssetHandler.h"

#include <ISystem.h>

#define LENS_FLARE_EXT "xml"

namespace LmbrCentral
{
    LensFlareAssetHandler::~LensFlareAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr LensFlareAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;
        AZ_Assert(type == AZ::AzTypeInfo<LensFlareAsset>::Uuid(), "Invalid asset type! We handle only 'LensFlareAsset'");

        if (!CanHandleAsset(id))
        {
            return nullptr;
        }

        return aznew LensFlareAsset;
    }

    AZ::Data::AssetHandler::LoadResult LensFlareAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        (void)assetLoadFilterCB;

        // Load from preloaded stream.
        AZ_Assert(asset.GetType() == AZ::AzTypeInfo<LensFlareAsset>::Uuid(), "Invalid asset type! We handle only 'LensFlareAsset'");
        if (stream)
        {
            LensFlareAsset* data = asset.GetAs<LensFlareAsset>();

            const size_t sizeBytes = static_cast<size_t>(stream->GetLength());
            char* buffer = new char[sizeBytes];
            stream->Read(sizeBytes, buffer);

            bool loaded = LoadFromBuffer(data, buffer, sizeBytes);

            delete[] buffer;

            return loaded ? AZ::Data::AssetHandler::LoadResult::LoadComplete : AZ::Data::AssetHandler::LoadResult::Error;
        }

        return AZ::Data::AssetHandler::LoadResult::Error;
    }

    void LensFlareAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void LensFlareAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<LensFlareAsset>::Uuid());
    }

    void LensFlareAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<LensFlareAsset>::Uuid());

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<LensFlareAsset>::Uuid());
    }

    void LensFlareAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect();

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType LensFlareAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<LensFlareAsset>::Uuid();
    }

    const char* LensFlareAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Lens Flare";
    }

    const char* LensFlareAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/LensFlare.svg";
    }

    void LensFlareAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(LENS_FLARE_EXT);
    }

    bool LensFlareAssetHandler::CanHandleAsset(const AZ::Data::AssetId& id) const
    {
        // Look up the asset path to ensure it's actually a lens flare library.
        AZStd::string assetPath;
        EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, id);

        bool isLensFlareLibrary = false;
        if (!assetPath.empty() && strstr(assetPath.c_str(), "." LENS_FLARE_EXT))
        {
            //check whether the file includes "LensFlareLibrary" string
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "FileIO is not initialized.");
            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            const AZ::IO::Result result = fileIO->Open(assetPath.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle);
            if (result)
            {
                AZ::u64 fileSize = 0;
                if (fileIO->Size(fileHandle, fileSize) && fileSize)
                {
                    AZStd::vector<char> memoryBuffer;
                    memoryBuffer.resize_no_construct(fileSize);
                    if (fileIO->Read(fileHandle, memoryBuffer.begin(), fileSize, true))
                    {
                        //compare string instead of parse the entire xml because it's faster. 
                        isLensFlareLibrary = (nullptr != strstr(memoryBuffer.begin(), "LensFlareLibrary"));
                    }
                }

                fileIO->Close(fileHandle);
            }
        }
        return isLensFlareLibrary;
    }

    bool LensFlareAssetHandler::LoadFromBuffer(LensFlareAsset* data, char* buffer, [[maybe_unused]] size_t bufferSize)
    {
        using namespace AZ::rapidxml;

        xml_document<char> xmlDoc;
        xmlDoc.parse<parse_no_data_nodes>(buffer);

        xml_node<char>* root = xmlDoc.first_node();
        if (root)
        {
            //get the name of this lens flare library
            xml_attribute<char>* nameAttribute = root->first_attribute("Name");
            if (nameAttribute)
            {
                const char* libName = nameAttribute->value();
                if (libName && libName[0])
                {
                    xml_node<char>* flareNode = root->first_node("FlareItem");
                    while (flareNode)
                    {
                        //for each of FlareItem add their path, combined with lib name, to LensFlareAsset data. 
                        xml_attribute<char>* flareNameAttribute = flareNode->first_attribute("Name");
                        if (flareNameAttribute)
                        {
                            const char* effectName = flareNameAttribute->value();
                            if (effectName && effectName[0])
                            {
                                AZStd::string flarePath = AZStd::string::format("%s.%s", libName, effectName);
                                data->AddPath(std::move(flarePath));
                            }
                        }

                        flareNode = flareNode->next_sibling("FlareItem");
                    }
                    return true;
                }
            }
        }

        return false;
    }
} // namespace LmbrCentral

