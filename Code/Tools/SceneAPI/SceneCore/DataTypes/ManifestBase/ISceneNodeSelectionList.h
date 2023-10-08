#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <memory>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class DataObject;
        }
        namespace DataTypes
        {
            class ISceneNodeSelectionList
            {
            public:
                AZ_RTTI(ISceneNodeSelectionList, "{DC3F9996-E550-4780-A03B-80B0DDA1DA45}");

                //! Callback for enumerating through the list of selected or unselected nodes.
                //! @param name The node name for each node enumerated through.
                //! @return True if enumeration should continue, false if it should stop.
                using EnumerateNodesCallback = AZStd::function<bool(const AZStd::string& name)>;

                virtual ~ISceneNodeSelectionList() = default;
                virtual size_t GetSelectedNodeCount() const = 0;
                virtual void AddSelectedNode(const AZStd::string& name) = 0;
                virtual void AddSelectedNode(AZStd::string&& name) = 0;
                virtual void RemoveSelectedNode(const AZStd::string& name) = 0;
                virtual void ClearSelectedNodes() = 0;
                virtual void ClearUnselectedNodes() = 0;

                //! Check to see if the given name is a selected node.
                //! @return True if the name appears in the selected node list, false if it doesn't.
                virtual bool IsSelectedNode(const AZStd::string& name) const = 0;
                //! Enumerate through the list of selected nodes, calling the callback with each node name.
                //! @param callback The callback to call for each selected node.
                virtual void EnumerateSelectedNodes(const EnumerateNodesCallback& callback) const = 0;
                //! Enumerate through the list of unselected nodes, calling the callback with each node name.
                //! @param callback The callback to call for each unselected node.
                virtual void EnumerateUnselectedNodes(const EnumerateNodesCallback& callback) const = 0;

                virtual AZStd::unique_ptr<ISceneNodeSelectionList> Copy() const = 0;
                virtual void CopyTo(ISceneNodeSelectionList& other) const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
