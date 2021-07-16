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
                MOCK_CONST_METHOD1(GetSelectedNode,
                    const AZStd::string & (size_t index));
                MOCK_METHOD1(AddSelectedNode,
                    size_t(const AZStd::string & name));
                size_t AddSelectedNode(AZStd::string&& name)
                {
                    return AddSelectedNode(name);
                }
                MOCK_METHOD1(RemoveSelectedNode,
                    void(size_t index));
                MOCK_METHOD1(RemoveSelectedNode,
                    void(const AZStd::string & name));
                MOCK_METHOD0(ClearSelectedNodes,
                    void());

                MOCK_CONST_METHOD0(GetUnselectedNodeCount,
                    size_t());
                MOCK_CONST_METHOD1(GetUnselectedNode,
                    const AZStd::string & (size_t index));
                MOCK_METHOD0(ClearUnselectedNodes,
                    void());

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
