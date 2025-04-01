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
            class ILodRule
                : public IRule
            {
            public:
                AZ_RTTI(ILodRule, "{D91C2CA1-B3F9-4819-8B58-AE2AEB98859A}", IRule);

                virtual ~ILodRule() override = default;

                virtual ISceneNodeSelectionList& GetSceneNodeSelectionList(size_t index) = 0;
                virtual const ISceneNodeSelectionList& GetSceneNodeSelectionList(size_t index) const = 0;
                virtual size_t GetLodCount() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
