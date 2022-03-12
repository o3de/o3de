/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    // Redirects writing of the AssetBundleManifest to an older version if the bundle version
    // is not set to the current version
    static void OldBundleManifestWriter(AZ::SerializeContext::EnumerateInstanceCallContext& callContext, const void* bundleManifestPointer,
        const AZ::SerializeContext::ClassData&, const AZ::SerializeContext::ClassElement* assetBundleManifestClassElement);

    static bool BundleManifestVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);

    const int AssetBundleManifest::CurrentBundleVersion = 3;
    const char AssetBundleManifest::s_manifestFileName[] = "manifest.xml";

    AssetBundleManifest::AssetBundleManifest() = default;
    AssetBundleManifest::~AssetBundleManifest() = default;

    void AssetBundleManifest::ReflectSerialize(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AssetBundleManifest>()
                ->Version(CurrentBundleVersion, &BundleManifestVersionConverter)
                ->Attribute(AZ::SerializeContextAttributes::ObjectStreamWriteElementOverride, &OldBundleManifestWriter)
                ->Field("BundleVersion", &AssetBundleManifest::m_bundleVersion)
                ->Field("CatalogName", &AssetBundleManifest::m_catalogName)
                ->Field("DependentBundleNames", &AssetBundleManifest::m_dependentBundleNames)
                ->Field("LevelNames", &AssetBundleManifest::m_levelDirs);

            // Make sure the AZStd::vector<AZStd::string> type is reflected so that it can be read
            // using DataElement::GetChildData
            serializeContext->RegisterGenericType<AZStd::vector<AZStd::string>>();
        }
    }

    bool BundleManifestVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            static constexpr AZ::u32 levelNamesCrc = AZ_CRC_CE("LevelNames");
            AZStd::vector<AZ::IO::Path> newLevelDirs;
            if (AZStd::vector<AZStd::string> oldLevelNames; rootElement.GetChildData(levelNamesCrc, oldLevelNames))
            {
                newLevelDirs.insert(newLevelDirs.end(),
                    AZStd::make_move_iterator(oldLevelNames.begin()), AZStd::make_move_iterator(oldLevelNames.end()));
            }
            else
            {
                AZ_Error("AssetBundleManifest", false, R"(Unable to read "levelNames" from AssetBundleManifest version %u )",
                    rootElement.GetVersion());
            }

            rootElement.RemoveElementByName(levelNamesCrc);
            rootElement.AddElementWithData(context, "LevelNames", newLevelDirs);
        }
        return true;
    }

    void OldBundleManifestWriter(AZ::SerializeContext::EnumerateInstanceCallContext& callContext, const void* bundleManifestPointer,
        const AZ::SerializeContext::ClassData&, const AZ::SerializeContext::ClassElement* assetBundleManifestClassElement)
    {
        // Copy the AssetBundleManifest current version instance to the AssetBundleManifest V2 instance
        auto assetBundleManifestCurrent = reinterpret_cast<const AssetBundleManifest*>(bundleManifestPointer);
        if (assetBundleManifestCurrent->GetBundleVersion() <= 2)
        {
            auto serializeContext = const_cast<AZ::SerializeContext*>(callContext.m_context);

            struct AssetBundleManifestV2
            {
                // Use the same ClassName and typeid as the AssetBundleManifest
                AZ_TYPE_INFO(AssetBundleManifest, azrtti_typeid<AssetBundleManifest>());
                AZStd::string m_catalogName;
                AZStd::vector<AZStd::string> m_dependentBundleNames;
                AZStd::vector<AZStd::string> m_levelDirs;
                int m_bundleVersion{};
            };
            auto ReflectAssetBundleManifestV2 = [](AZ::SerializeContext* serializeContext)
            {
                serializeContext->Class<AssetBundleManifestV2>()
                    ->Version(2)
                    ->Field("BundleVersion", &AssetBundleManifestV2::m_bundleVersion)
                    ->Field("CatalogName", &AssetBundleManifestV2::m_catalogName)
                    ->Field("DependentBundleNames", &AssetBundleManifestV2::m_dependentBundleNames)
                    ->Field("LevelNames", &AssetBundleManifestV2::m_levelDirs);
            };

            // Unreflect the AssetBundleManifest class at the version since it shares the same typeid
            // as the older version and Reflect the V2 AssetBundlerManifest
            serializeContext->EnableRemoveReflection();
            AssetBundleManifest::ReflectSerialize(serializeContext);
            serializeContext->DisableRemoveReflection();
            ReflectAssetBundleManifestV2(serializeContext);

            // Use the Current AssetBundleManifest instance to make a Version 2 AssetBundleManifest
            AssetBundleManifestV2 assetBundleManifestV2;
            assetBundleManifestV2.m_catalogName = assetBundleManifestCurrent->GetCatalogName();
            assetBundleManifestV2.m_dependentBundleNames = assetBundleManifestCurrent->GetDependentBundleNames();
            assetBundleManifestV2.m_bundleVersion = assetBundleManifestCurrent->GetBundleVersion();
            for (const AZ::IO::Path& levelDir : assetBundleManifestCurrent->GetLevelDirectories())
            {
                assetBundleManifestV2.m_levelDirs.emplace_back(levelDir.Native());
            }

            const AZ::TypeId& assetBundlerManifestTypeId = azrtti_typeid<AssetBundleManifestV2>();
            const auto assetBundleManifestV2ClassData = serializeContext->FindClassData(assetBundlerManifestTypeId);

            // Create an AssetBundleManifest Version 2 Class Eleemnt
            // It will copy over the name and nameCrc values of the current AssetBundleManifestelemnt
            auto CreateAssetBundleManifestV2ClassElement = [&assetBundlerManifestTypeId](
                const AZ::SerializeContext::ClassElement* currentVersionElement) -> AZ::SerializeContext::ClassElement
            {
                AZ::SerializeContext::ClassElement v2ClassElement;
                // Copy over the name of he current
                if (currentVersionElement)
                {
                    v2ClassElement.m_name = currentVersionElement->m_name;
                    v2ClassElement.m_nameCrc = currentVersionElement->m_nameCrc;
                }
                v2ClassElement.m_dataSize = sizeof(AssetBundleManifest);
                v2ClassElement.m_azRtti = AZ::GetRttiHelper<AssetBundleManifestV2>();
                v2ClassElement.m_genericClassInfo = nullptr;
                v2ClassElement.m_typeId = assetBundlerManifestTypeId;
                v2ClassElement.m_editData = nullptr;
                v2ClassElement.m_attributeOwnership = AZ::SerializeContext::ClassElement::AttributeOwnership::Self;
                return v2ClassElement;
            };
            const auto assetBundleManifestV2ClassElement = CreateAssetBundleManifestV2ClassElement(assetBundleManifestClassElement);

            serializeContext->EnumerateInstanceConst(&callContext, &assetBundleManifestV2, assetBundlerManifestTypeId,
                assetBundleManifestV2ClassData, assetBundleManifestClassElement ? &assetBundleManifestV2ClassElement : nullptr);

            // Unreflect the V2 AssetBundleManifest and Re-reflect the AssetBundleManifest class at the current version
            serializeContext->EnableRemoveReflection();
            ReflectAssetBundleManifestV2(serializeContext);
            serializeContext->DisableRemoveReflection();
            AssetBundleManifest::ReflectSerialize(serializeContext);
        }
    }

    const AZStd::vector<AZ::IO::Path>& AssetBundleManifest::GetLevelDirectories() const
    {
        return m_levelDirs;
    }
    void AssetBundleManifest::SetLevelsDirectory(const AZStd::vector<AZ::IO::Path>& levelDirs)
    {
        m_levelDirs = levelDirs;
    }
} // namespace AzFramework
