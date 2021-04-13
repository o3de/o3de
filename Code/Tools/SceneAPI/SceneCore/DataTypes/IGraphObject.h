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

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/Utilities/DebugOutput.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IGraphObject
            {
            public:
                AZ_RTTI(IGraphObject, "{9A72266A-B308-4A74-9500-EF5C7B60AD8A}");

                virtual ~IGraphObject() = 0;

                // Copy object-level attributes
                // These are attributes that would be the same at different optimization levels of a node. For
                // instance, a decimated version of a mesh node is still going to have the same unit size.
                virtual void CloneAttributesFrom(const IGraphObject* sourceObject) = 0;

                // When requested, the scene graph can be dumped to a file to help with debugging.
                virtual void GetDebugOutput([[maybe_unused]] AZ::SceneAPI::Utilities::DebugOutput& output) const {}
            };

            inline IGraphObject::~IGraphObject()
            {
            }
        }  //namespace DataTypes
    }  //namespace SceneAPI
}  //namespace AZ
