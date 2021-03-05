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
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IBlendShapeRule
                : public IRule
            {
            public:
                AZ_RTTI(IBlendShapeRule, "{C801EEE7-934B-4F5E-A20B-C394BEC992E2}", IRule);

                ~IBlendShapeRule() override = default;
                virtual ISceneNodeSelectionList& GetSceneNodeSelectionList() = 0;
                virtual const ISceneNodeSelectionList& GetSceneNodeSelectionList() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
