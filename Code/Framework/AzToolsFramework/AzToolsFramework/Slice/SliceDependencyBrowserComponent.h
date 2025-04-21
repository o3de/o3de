/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#include <AzCore/Component/Component.h>

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

#include "SliceDependencyBrowserBus.h"

namespace AzToolsFramework
{
    class SliceDependencyBrowserComponent
        : public AZ::Component
        , private AssetBrowser::AssetDatabaseLocationNotificationBus::Handler
        , private SliceDependencyBrowserRequestsBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
    {

    public:
        AZ_COMPONENT(SliceDependencyBrowserComponent, "{D5D7D1BB-CACB-4B42-8FDA-F6C46F52418A}");

        SliceDependencyBrowserComponent()
            : m_databaseConnection(aznew AssetDatabase::AssetDatabaseConnection())
            , m_currentlyInspectedNode(nullptr)
        {}

        ~SliceDependencyBrowserComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        //////////////////////////////////////////////////////////////////////////
        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AssetDatabaseLocationNotificationBus
        //////////////////////////////////////////////////////////////////////////
        void OnDatabaseInitialized() override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetCatalogEventBus::Handler
        //////////////////////////////////////////////////////////////////////////
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;

        //////////////////////////////////////////////////////////////////////////
        // SliceDependencyBrowserRequestsBus::Handler
        //////////////////////////////////////////////////////////////////////////
        AZStd::shared_ptr<SliceRelationshipNode> ReportSliceAssetDependenciesByPath(const AZStd::string& relativePath) override;
        AZStd::shared_ptr<SliceRelationshipNode> ReportSliceAssetDependenciesById(const AZ::Data::AssetId& assetId) override;
        void ClearCurrentlyReportedSlice() override;

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SliceDependencyBrowserService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SliceDependencyBrowserService"));
        }
    private:

        //! Connection to the asset Database
        AZStd::shared_ptr<AssetDatabase::AssetDatabaseConnection> m_databaseConnection;

        //! All nodes that make up the slice Relationship Graph
        AZStd::unordered_map<AZ::Crc32, AZStd::shared_ptr<SliceRelationshipNode>> m_allNodes;

        //! Node that is being currently inspected
        AZStd::shared_ptr<SliceRelationshipNode> m_currentlyInspectedNode;

        /**
        * \brief Adds a relationship between the dependent and the dependency
        * \param const AZStd::string & startingSlice Relative path to the slice that the graph expands out from
        */
        AZStd::shared_ptr<SliceRelationshipNode> StartNewGraph(const AZStd::string& startingSlice);

        /**
        * \brief Adds a relationship between the dependent and the dependency
        * \param const AZStd::string& dependent Relative path to the slice that is dependent on dependency
        * \param const AZStd::string& dependency Relative path to the slice that is a dependency of the dependent
        * \param SliceRelationshipNode::SliceRelationshipNodeSet& [OUT] List of nodes that are to be visited while a graph is being constructed; When a relationship is added if the dependent or dependency nodes are
        *                                                               newly created, then they get added to this set.
        */
        void AddRelationship(const AZStd::string& dependent, const AZStd::string& dependency, SliceRelationshipNode::SliceRelationshipNodeSet& nodesToVisit);

        /**
        * \brief Communicates with the Asset database to find the relative asset path for a given uuid
        * \param const AZ::Uuid & uuid uuid of the asset whose relative asset path is to be found
        * \return AZStd::string The relative asset path for the indicated uuid
        */
        AZStd::string GetRelativeAssetPathByGuid(const AZ::Uuid& uuid) const;

        /**
        * \brief Gets slice relationships for the slice asset at the indicated relative path
        * \param const AZStd::string& relativePath path of the slice whose dependencies are being requested
        * \param AZStd::vector<AZStd::string> & dependents [OUT] All dependent slices for indicated slice
        * \param AZStd::vector<AZStd::string> & dependencies [OUT] All slices that indicated slice depends on
        */
        void GetSliceRelationshipsByRelativeAssetPath(const AZStd::string& relativePath, AZStd::vector<AZStd::string>& dependents, AZStd::vector<AZStd::string>& dependencies) const;

        /**
        * \brief Gets slice dependencies for the slice asset at the indicated relative path
        * \param const AZStd::string & relativePath relativePath path of the slice whose dependencies are being requested
        * \param AZStd::vector<AZStd::string> & dependencies [OUT] All slices that indicated slice depends on
        * \return bool True if any dependencies were found false otherwise
        */
        bool GetSliceDependenciesByRelativeAssetPath(const AZStd::string& relativePath, AZStd::vector<AZStd::string>& dependencies) const;

        /**
        * \brief Gets slice dependents for the slice asset at the indicated relative path
        * \param const AZStd::string & relativePath relativePath path of the slice whose dependents are being requested
        * \param AZStd::vector<AZStd::string> & dependents [OUT] All dependent slices for indicated slice
        * \return bool True if any dependents were found false otherwise
        */
        bool GetSliceDependendentsByRelativeAssetPath(const AZStd::string& relativePath, AZStd::vector<AZStd::string>& dependents) const;
    };
}
