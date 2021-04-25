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
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISkeletonGroup
                : public IGroup
            {
            public:
                AZ_RTTI(ISkeletonGroup, "{419ECE00-3CC5-4CAB-A451-453BF3FEA665}", IGroup);

                ~ISkeletonGroup() override = default;
                virtual const AZStd::string& GetSelectedRootBone() const = 0;
                virtual void SetSelectedRootBone(const AZStd::string& selectedRootBone) = 0;
            };
        }
    }
}
