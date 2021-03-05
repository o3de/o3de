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

#include <AzCore/EBus/EBus.h>
#include "SliceRelationshipNode.h"

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }
}

namespace AzToolsFramework
{
    class SliceDependencyBrowserRequests
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        /**
        * \brief If needed, generates a slice relationship graph and returns the node for the slice indicated by 'relativePath'
        * \param const AZStd::string & relativePath Relative path of the slice that is to be the starting node for this graph
        * \return AZStd::shared_ptr<AzToolsFramework::SliceRelationshipNode> Node in the slice relationship graph that corresponds to 'relativePath'
        */
        virtual AZStd::shared_ptr<SliceRelationshipNode> ReportSliceAssetDependenciesByPath(const AZStd::string& relativePath) = 0;

        /**
        * \brief If needed, generates a slice relationship graph and returns the node for the slice indicated by 'assetId'
        * \param const AZ::Data::AssetId & assetId Asset Id of the slice that is to be the starting node for this graph
        * \return AZStd::shared_ptr<AzToolsFramework::SliceRelationshipNode> Node in the slice relationship graph that corresponds to 'assetId'
        */
        virtual AZStd::shared_ptr<SliceRelationshipNode> ReportSliceAssetDependenciesById(const AZ::Data::AssetId& assetId) = 0;

        /**
        * \brief Indicates that no slice is being viewed so that the component can disconnect itself from the Asset catalog bus
        * \return void
        */
        virtual void ClearCurrentlyReportedSlice() = 0;
    };

    using SliceDependencyBrowserRequestsBus = AZ::EBus<SliceDependencyBrowserRequests>;

    class SliceDependencyBrowserNotifications
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        /**
        * \brief Notifies handlers of an update in the slice relationship model
        * \param const AZStd::shared_ptr<SliceRelationshipNode> & focusNode Node that any view should focus on
        */
        virtual void OnSliceRelationshipModelUpdated(const AZStd::shared_ptr<SliceRelationshipNode>& focusNode) = 0;
    };

    using SliceDependencyBrowserNotificationsBus = AZ::EBus<SliceDependencyBrowserNotifications>;
}