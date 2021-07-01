#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
