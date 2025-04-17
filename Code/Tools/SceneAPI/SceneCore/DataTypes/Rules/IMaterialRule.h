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

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMaterialRule
                : public IRule
            {
            public:
                AZ_RTTI(IMaterialRule, "{428C9752-6EDF-4FA2-9BDF-DBDFCEB4CC0F}", IRule);

                ~IMaterialRule() override = default;

                virtual bool RemoveUnusedMaterials() const = 0;
                virtual bool UpdateMaterials() const = 0;

            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
