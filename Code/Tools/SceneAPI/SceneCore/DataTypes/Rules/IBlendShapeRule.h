/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

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
