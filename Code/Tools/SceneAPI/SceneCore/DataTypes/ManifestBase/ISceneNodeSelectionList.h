#pragma once

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
                
                virtual ~ ISceneNodeSelectionList() = default;
                virtual size_t GetSelectedNodeCount() const = 0;
                virtual const AZStd::string& GetSelectedNode(size_t index) const = 0;
                virtual size_t AddSelectedNode(const AZStd::string& name) = 0;
                virtual size_t AddSelectedNode(AZStd::string&& name) = 0;
                virtual void RemoveSelectedNode(size_t index) = 0;
                virtual void RemoveSelectedNode(const AZStd::string& name) = 0;
                virtual void ClearSelectedNodes() = 0;

                virtual size_t GetUnselectedNodeCount() const = 0;
                virtual const AZStd::string& GetUnselectedNode(size_t index) const = 0;
                virtual void ClearUnselectedNodes() = 0;

                virtual AZStd::unique_ptr<ISceneNodeSelectionList> Copy() const = 0;
                virtual void CopyTo(ISceneNodeSelectionList& other) const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
