/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SliceDependencyBrowserComponent.h"

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>


namespace AzToolsFramework
{
    //////////////////////////////////////////////////////////////////////////

    void SliceDependencyBrowserComponent::Activate()
    {
        AssetBrowser::AssetDatabaseLocationNotificationBus::Handler::BusConnect();
        SliceDependencyBrowserRequestsBus::Handler::BusConnect();
    }

    void SliceDependencyBrowserComponent::Deactivate()
    {
        SliceDependencyBrowserRequestsBus::Handler::BusConnect();
        AssetBrowser::AssetDatabaseLocationNotificationBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    void SliceDependencyBrowserComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SliceDependencyBrowserComponent, AZ::Component>()
                ;
        }
    }

    void SliceDependencyBrowserComponent::OnDatabaseInitialized()
    {
        m_databaseConnection->OpenDatabase();
    }

    void SliceDependencyBrowserComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        if (!m_allNodes.empty() && m_currentlyInspectedNode != nullptr)
        {
            bool refreshTree = true;
            AZStd::vector<AZStd::string> dependencies;
            AZStd::string relativePath(GetRelativeAssetPathByGuid(assetId.m_guid));

            if(AzFramework::StringFunc::Path::IsExtension(relativePath.c_str(), AzToolsFramework::SliceUtilities::GetSliceFileExtension().c_str(), false))
            {
                GetSliceDependenciesByRelativeAssetPath(relativePath, dependencies);

                for (const AZStd::string& dependency : dependencies)
                {
                    const auto& dependencyNodeIt = m_allNodes.find(AZ::Crc32(dependency.c_str(), dependency.size(), true));

                    if (dependencyNodeIt != m_allNodes.end())
                    {
                        refreshTree = true;
                        break;
                    }
                }

                if (refreshTree)
                {
                    StartNewGraph(m_currentlyInspectedNode->m_sliceRelativePath);
                    SliceDependencyBrowserNotificationsBus::Broadcast(&SliceDependencyBrowserNotifications::OnSliceRelationshipModelUpdated, m_currentlyInspectedNode);
                }
            }
        }
    }

    AZStd::shared_ptr<SliceRelationshipNode> SliceDependencyBrowserComponent::StartNewGraph(const AZStd::string& startingSlice)
    {
        m_allNodes.clear();

        SliceRelationshipNode::SliceRelationshipNodeSet nodesToVisit;

        AZStd::shared_ptr<SliceRelationshipNode> startingNode = AZStd::make_shared<SliceRelationshipNode>(SliceRelationshipNode(startingSlice));
        m_allNodes.insert(AZStd::pair<AZ::Crc32, AZStd::shared_ptr<AzToolsFramework::SliceRelationshipNode>>(startingNode->GetRelativePathCrc(),startingNode));
        nodesToVisit.insert(startingNode);
        m_currentlyInspectedNode = startingNode;

        AZStd::vector<AZStd::string> dependents;
        AZStd::vector<AZStd::string> dependencies;

        while (!nodesToVisit.empty())
        {
            const AZStd::shared_ptr<SliceRelationshipNode>& currentNode = *(nodesToVisit.begin());

            GetSliceRelationshipsByRelativeAssetPath(currentNode->m_sliceRelativePath, dependents, dependencies);

            for (const AZStd::string& dependent : dependents)
            {
                AddRelationship(dependent, currentNode->m_sliceRelativePath, nodesToVisit);
            }

            for (const AZStd::string& dependency : dependencies)
            {
                AddRelationship(currentNode->m_sliceRelativePath, dependency, nodesToVisit);
            }

            dependents.clear();
            dependencies.clear();
            nodesToVisit.erase(currentNode);
        }

        return startingNode;
    }

    AZStd::string SliceDependencyBrowserComponent::GetRelativeAssetPathByGuid(const AZ::Uuid & uuid) const
    {
        AZStd::string result;

        bool found = false;
        m_databaseConnection->QuerySourceBySourceGuid(uuid,[&](AssetDatabase::SourceDatabaseEntry& source)
        {
            found = true;
            result = AZStd::move(source.m_sourceName);
            return false;
        });

        return result;
    }

    void SliceDependencyBrowserComponent::AddRelationship(const AZStd::string& dependent, const AZStd::string& dependency, SliceRelationshipNode::SliceRelationshipNodeSet& nodesToVisit)
    {
        AZStd::shared_ptr<SliceRelationshipNode> dependentNode = nullptr;
        AZ::Crc32 dependentPathCrc(dependent.c_str(), dependent.size(), true);
        const auto& dependentNodeIt = m_allNodes.find(dependentPathCrc);
        if (dependentNodeIt != m_allNodes.end())
        {
            dependentNode = (dependentNodeIt->second);
        }
        else
        {
            dependentNode = AZStd::make_shared<AzToolsFramework::SliceRelationshipNode>(SliceRelationshipNode(dependent, dependentPathCrc));
            m_allNodes.insert(AZStd::pair<AZ::Crc32, AZStd::shared_ptr<AzToolsFramework::SliceRelationshipNode>>(dependentPathCrc,dependentNode));
            nodesToVisit.insert(dependentNode);
        }

        AZStd::shared_ptr<SliceRelationshipNode> dependencyNode = nullptr;
        AZ::Crc32 dependencyPathCrc(dependency.c_str(), dependency.size(), true);
        auto dependencyNodeIt = m_allNodes.find(dependencyPathCrc);
        if (dependencyNodeIt != m_allNodes.end())
        {
            dependencyNode = (dependencyNodeIt->second);
        }
        else
        {
            dependencyNode = AZStd::make_shared<SliceRelationshipNode>(SliceRelationshipNode(dependency, dependencyPathCrc));
            m_allNodes.insert(AZStd::pair<AZ::Crc32, AZStd::shared_ptr<AzToolsFramework::SliceRelationshipNode>>(dependencyPathCrc,dependencyNode));
            nodesToVisit.insert(dependencyNode);
        }

        dependentNode->AddDependency(dependencyNode);
        dependencyNode->AddDependent(dependentNode);
    }

    AZStd::shared_ptr<SliceRelationshipNode> SliceDependencyBrowserComponent::ReportSliceAssetDependenciesByPath(const AZStd::string& relativePath)
    {
        const auto& nodeIt = m_allNodes.find(AZ::Crc32(relativePath.c_str(), relativePath.size(), true));
        AZStd::shared_ptr<SliceRelationshipNode> focusNode = nullptr;
        if (nodeIt != m_allNodes.end())
        {
            m_currentlyInspectedNode = (*nodeIt).second;
            focusNode = (*nodeIt).second;
        }
        else
        {
            focusNode = StartNewGraph(relativePath);
        }

        return focusNode;
    }

    void SliceDependencyBrowserComponent::ClearCurrentlyReportedSlice()
    {
        m_allNodes.clear();
        m_currentlyInspectedNode = nullptr;
    }

    AZStd::shared_ptr<SliceRelationshipNode> SliceDependencyBrowserComponent::ReportSliceAssetDependenciesById(const AZ::Data::AssetId & sliceAssetId)
    {
        AZStd::string relativePath;
        relativePath = GetRelativeAssetPathByGuid(sliceAssetId.m_guid);

        return ReportSliceAssetDependenciesByPath(relativePath);
    }

    void SliceDependencyBrowserComponent::GetSliceRelationshipsByRelativeAssetPath(const AZStd::string& relativePath, AZStd::vector<AZStd::string>& dependents, AZStd::vector<AZStd::string>& dependencies) const
    {
        GetSliceDependenciesByRelativeAssetPath(relativePath, dependencies);
        GetSliceDependendentsByRelativeAssetPath(relativePath, dependents);
    }

    bool SliceDependencyBrowserComponent::GetSliceDependenciesByRelativeAssetPath(const AZStd::string& relativePath, AZStd::vector<AZStd::string>& dependencies) const
    {
        bool found = false;

        AZ::Uuid sourceUuid;
        m_databaseConnection->QuerySourceBySourceName(relativePath.c_str(), [&sourceUuid](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
        {
            sourceUuid = entry.m_sourceGuid;
            return false;
        });

        if (sourceUuid.IsNull())
        {
            return false;
        }

        bool succeeded = m_databaseConnection->QueryDependsOnSourceBySourceDependency(
            sourceUuid,
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceOrJob,
            [&](AssetDatabase::SourceFileDependencyEntry& entry)
            {
                AZStd::string dependencyName = entry.m_dependsOnSource.GetPath();

                if (entry.m_dependsOnSource.IsUuid())
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
                    m_databaseConnection->QuerySourceBySourceGuid(
                        entry.m_dependsOnSource.GetUuid(),
                        [&dependencyName](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                        {
                            dependencyName = entry.m_sourceName;
                            return false;
                        });
                }

                if (!dependencyName.ends_with(".slice"))
                {
                    // Since this tool is only meant to display slices, filter out non-slice dependencies
                    return true;
                }

                found = true;
                dependencies.push_back(AZStd::move(dependencyName));
                return true;
            });
        return found && succeeded;
    }

    bool SliceDependencyBrowserComponent::GetSliceDependendentsByRelativeAssetPath(const AZStd::string& relativePath, AZStd::vector<AZStd::string>& dependents) const
    {
        bool found = false;

        auto callback = [&](AssetDatabase::SourceFileDependencyEntry& entry)
        {
            AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
            m_databaseConnection->QuerySourceBySourceGuid(
                entry.m_sourceGuid,
                [&sourceEntry](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                {
                    sourceEntry = entry;
                    return false;
                });

            if (!sourceEntry.m_sourceName.ends_with(".slice"))
            {
                // Since this tool is only meant to display slices, filter out non-slice dependencies
                return true;
            }

            found = true;
            dependents.push_back(AZStd::move(sourceEntry.m_sourceName));
            return true;
        };

        AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
        m_databaseConnection->QuerySourceBySourceName(
            relativePath.c_str(),
            [&sourceEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
            {
                sourceEntry = entry;
                return false;
            });

        AZStd::string scanFolderPath;
        m_databaseConnection->QueryScanFolderByScanFolderID(
            sourceEntry.m_scanFolderPK,
            [&scanFolderPath](const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry)
            {
                scanFolderPath = entry.m_scanFolder;
                return false;
            });

        auto absolutePath = AZ::IO::Path(scanFolderPath) / sourceEntry.m_sourceName;

        bool succeeded = m_databaseConnection->QuerySourceDependencyByDependsOnSource(
            sourceEntry.m_sourceGuid,
            sourceEntry.m_sourceName.c_str(),
            absolutePath.FixedMaxPathStringAsPosix().c_str(),
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_SourceOrJob,
            callback);

        return found && succeeded;
    }
} // namespace AzToolsFramework

