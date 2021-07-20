#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
