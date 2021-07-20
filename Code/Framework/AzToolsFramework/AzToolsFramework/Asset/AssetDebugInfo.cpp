/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Asset/AssetDebugInfo.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogBus.h>
#include <cinttypes>


const char AssetListDebugFileExtension[] = "assetlistdebug";

namespace AzToolsFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////
    // DependencyNode
    ////////////////////////////////////////////////////////////////////////////////////////////

    struct DependencyNode
    {
        ~DependencyNode()
        {
            for (AZStd::map<AZ::Data::AssetId, DependencyNode*>::iterator leaf = m_leaves.begin(); leaf != m_leaves.end(); ++leaf)
            {
                delete leaf->second;
            }
        }

        //! Creates a string that displays the hierarchy of how a product dependency relates back to a Seed in a Seed List file.
        //! Example string:
            //! fonts/vera-bold.font - {13AFF67C-6673-58CD-8FC8-265447B71259}:0 - 282 bytes
            //!   fonts/vera.fontfamily - {6B62FA48-7032-5B8D-88DB-4EDCF0D500AE}:0
            //!     ui/canvas/start.uicanvas - {E7B190F9-3507-5524-BE05-35C6941B8E26}:0
            //!       [SEED] levels/testdependencieslevel/level.pak - {A89A2DCA-0C7B-5919-B3E0-3F67BFD8AA40}:0
        void BuildHumanReadableString(const AZStd::string& tabString, AssetFileDebugInfoList& infoList)
        {
            AZStd::string fileName = infoList.m_fileDebugInfoList[m_assetId].m_assetRelativePath;

            // Only print size on the top asset, not the graph below it.
            uint64_t filesize = infoList.m_fileDebugInfoList[m_assetId].m_fileSize;
            AZStd::string sizeString = tabString.empty() ? AZStd::string::format(" - %" PRIu64 " bytes", filesize) : "";

            infoList.m_humanReadableString += tabString;

            if (m_isCyclicalDependency)
            {
                infoList.m_humanReadableString += AZStd::string("[CYCLICAL DEPENDENCY] ");
            }
            else if (m_leaves.empty())
            {
                infoList.m_humanReadableString += AZStd::string("[SEED] ");
            }

            infoList.m_humanReadableString += AZStd::string::format("%s - %s%s\n",
                fileName.c_str(),
                m_assetId.ToString<AZStd::string>().c_str(),
                sizeString.c_str());

            for (AZStd::map<AZ::Data::AssetId, DependencyNode*>::iterator leaf = m_leaves.begin(); leaf != m_leaves.end(); ++leaf)
            {
                leaf->second->BuildHumanReadableString(tabString + "  ", infoList);
            }
        }

        AZ::Data::AssetId m_assetId;
        DependencyNode* m_parent = nullptr;
        AZStd::map<AZ::Data::AssetId, DependencyNode*> m_leaves;
        bool m_isCyclicalDependency = false;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////
    // AssetFileDebugInfo
    ////////////////////////////////////////////////////////////////////////////////////////////

    void AssetFileDebugInfo::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetFileDebugInfo>()
                ->Version(1)
                ->Field("assetId", &AssetFileDebugInfo::m_assetId)
                ->Field("assetRelativePath", &AssetFileDebugInfo::m_assetRelativePath)
                ->Field("fileSize", &AssetFileDebugInfo::m_fileSize)
                ->Field("filesThatReferenceMe", &AssetFileDebugInfo::m_filesThatReferenceMe);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    // AssetFileDebugInfoList
    ////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetFileDebugInfoList::GetAllProductDependenciesDebug(
        const AZ::Data::AssetId& assetId,
        const AzFramework::PlatformId& platformIndex,
        AssetFileDebugInfoList* debugList,
        AZStd::unordered_set<AZ::Data::AssetId>* cyclicalDependencySet,
        const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
        const AZStd::vector<AZStd::string>& wildcardPatternExclusionList)
    {
        using namespace AzToolsFramework::AssetCatalog;

        if (!cyclicalDependencySet)
        {
            // A failure means there were no dependencies, and not that the call failed.
            return AZ::Success(AZStd::vector<AZ::Data::ProductDependency>());
        }

        // A core piece of debug info is the tree from seed(s) that reference an asset, to the asset itself.
        // It can be useful to know that someLevel\level.pak results in someTexture.dds being included, it's more useful to know
        // that someLevel\level.pak references someMesh.cgf, which references SomeMaterial.mtl, which references someTexture.dds.
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> currentDependenciesResult = AZ::Failure(AZStd::string());
        PlatformAddressedAssetCatalogRequestBus::EventResult(currentDependenciesResult, platformIndex, &PlatformAddressedAssetCatalogRequestBus::Events::GetDirectProductDependencies, assetId);
        if (!currentDependenciesResult.IsSuccess())
        {
            // A failure means there were no dependencies, and not that the call failed.
            return AZ::Success(AZStd::vector<AZ::Data::ProductDependency>());
        }

        AZStd::vector<AZ::Data::ProductDependency> entries = currentDependenciesResult.TakeValue();
        AZStd::vector<AZ::Data::ProductDependency> allFoundProducts;

        allFoundProducts.reserve(allFoundProducts.size());

        cyclicalDependencySet->insert(assetId);

        for (const AZ::Data::ProductDependency& productDependency : entries)
        {
            if (!productDependency.m_assetId.IsValid())
            {
                continue;
            }

            if (exclusionList.find(productDependency.m_assetId) != exclusionList.end())
            {
                continue;
            }

            bool wildcardPatternMatch = false;
            for (const AZStd::string& wildcardPattern : wildcardPatternExclusionList)
            {
                PlatformAddressedAssetCatalogRequestBus::EventResult(wildcardPatternMatch, platformIndex, &PlatformAddressedAssetCatalogRequestBus::Events::DoesAssetIdMatchWildcardPattern, assetId, wildcardPattern);
                if (wildcardPatternMatch)
                {
                    break;
                }
            }
            if (wildcardPatternMatch)
            {
                continue;
            }

            allFoundProducts.push_back(productDependency);

            // Cyclical Dependency detection
            if (cyclicalDependencySet->find(productDependency.m_assetId) != cyclicalDependencySet->end())
            {
                continue;
            }

            if (debugList->m_fileDebugInfoList.find(productDependency.m_assetId) == debugList->m_fileDebugInfoList.end())
            {
                debugList->m_fileDebugInfoList[productDependency.m_assetId].m_assetId = productDependency.m_assetId;
            }
            debugList->m_fileDebugInfoList[productDependency.m_assetId].m_filesThatReferenceMe.insert(assetId);

            // Recurse
            auto recursiveResult = GetAllProductDependenciesDebug(productDependency.m_assetId, platformIndex, debugList, cyclicalDependencySet, exclusionList);
            if (!recursiveResult.IsSuccess())
            {
                return recursiveResult;
            }
            AZStd::vector<AZ::Data::ProductDependency> recursiveValues = recursiveResult.TakeValue();
            allFoundProducts.insert(allFoundProducts.end(), recursiveValues.begin(), recursiveValues.end());
        }

        cyclicalDependencySet->erase(assetId);

        return AZ::Success(allFoundProducts);
    }

    const char* AssetFileDebugInfoList::GetAssetListDebugFileExtension()
    {
        return AssetListDebugFileExtension;
    }

    void AssetFileDebugInfoList::BuildHumanReadableString()
    {
        // Start with a newline to separate it from AZ serialization output.
        m_humanReadableString = "\n\n";
        for (AZStd::map<AZ::Data::AssetId, AssetFileDebugInfo>::iterator assetDebugInfo = m_fileDebugInfoList.begin();
            assetDebugInfo != m_fileDebugInfoList.end();
            ++assetDebugInfo)
        {
            DependencyNode root;
            root.m_assetId = assetDebugInfo->second.m_assetId;
            BuildNodeTree(assetDebugInfo->second.m_assetId, &root);
            root.BuildHumanReadableString("", *this);
            m_humanReadableString += AZStd::string("\n");
        }
    }

    void AssetFileDebugInfoList::BuildNodeTree(AZ::Data::AssetId assetId, DependencyNode* parent)
    {
        if (parent->m_isCyclicalDependency)
        {
            return;
        }

        for (const AZ::Data::AssetId& referenceId : m_fileDebugInfoList[assetId].m_filesThatReferenceMe)
        {
            bool foundLoop = false;
            for (DependencyNode* hierarchyWalk = parent; hierarchyWalk != nullptr; hierarchyWalk = hierarchyWalk->m_parent)
            {
                if (hierarchyWalk->m_assetId == referenceId)
                {
                    foundLoop = true;
                    break;
                }
            }

            DependencyNode* leaf = new DependencyNode();
            leaf->m_parent = parent;
            leaf->m_assetId = referenceId;
            leaf->m_isCyclicalDependency = foundLoop;

            BuildNodeTree(referenceId, leaf);

            parent->m_leaves[referenceId] = leaf;
        }
    }

    void AssetFileDebugInfoList::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetFileDebugInfoList>()
                ->Version(1)
                // We are only reflecting the human readable string here because we do not plan on loading this file type
                // into memory at this time, and this is the easiest way for our customers to read this info
                ->Field("humanReadableString", &AssetFileDebugInfoList::m_humanReadableString);
        }
    }

} // namespace AzToolsFramework
