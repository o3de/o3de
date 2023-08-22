/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <gmock/gmock.h>

#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class MockISceneNodeSelectionList
                : public ISceneNodeSelectionList
            {
            public:
                AZ_RTTI(MockISceneNodeSelectionList, "{6054C5D9-4719-4483-9E45-94F8BF7E614C}", ISceneNodeSelectionList);

                // IMeshGroup
                MOCK_CONST_METHOD0(GetSelectedNodeCount,
                    size_t());
                MOCK_METHOD1(AddSelectedNode,
                    void(const AZStd::string & name));
                void AddSelectedNode(AZStd::string&& name)
                {
                    // Passing in AZStd::move(Name) directly into AddSelectedNode() causes compile errors on Linux as it
                    // thinks this has infinite recursion. Storing in a temp variable disambiguates the AddSelectedNode() call.
                    const AZStd::string& movedName = AZStd::move(name);
                    AddSelectedNode(movedName);
                }
                MOCK_METHOD1(RemoveSelectedNode,
                    void(const AZStd::string & name));
                MOCK_METHOD0(ClearSelectedNodes,
                    void());
                MOCK_CONST_METHOD1(IsSelectedNode, bool(const AZStd::string& name));
                MOCK_CONST_METHOD1(EnumerateSelectedNodes, void(const EnumerateNodesCallback& callback));

                MOCK_CONST_METHOD0(GetUnselectedNodeCount,
                    size_t());
                MOCK_METHOD0(ClearUnselectedNodes,
                    void());
                MOCK_CONST_METHOD1(EnumerateUnselectedNodes, void(const EnumerateNodesCallback& callback));

                MOCK_CONST_METHOD0(CopyProxy,
                    ISceneNodeSelectionList*());
                AZStd::unique_ptr<ISceneNodeSelectionList> Copy() const
                {
                    return AZStd::unique_ptr<ISceneNodeSelectionList>(CopyProxy());
                }
                MOCK_CONST_METHOD1(CopyTo, 
                    void(ISceneNodeSelectionList& other));
            };
        }  // namespace DataTypes
    }  // namespace SceneAPI
}  // namespace AZ
