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
