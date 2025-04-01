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
            class ISkeletonProxyRule
                : public IRule
            {
            public:
                AZ_RTTI(ISkeletonProxyRule, "{0F1D03FA-E6A8-4E9D-8EF8-3AB426360C45}", IRule);

                ~ISkeletonProxyRule() override = default;


                virtual size_t GetProxyGroupCount() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
