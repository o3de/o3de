#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Group
        {
            class IActorGroup
                : public AZ::SceneAPI::DataTypes::IGroup
            {
            public:
                AZ_RTTI(IActorGroup, "{C86945A8-AEE8-4CFC-8FBF-A20E9BC71348}", AZ::SceneAPI::DataTypes::IGroup);

                ~IActorGroup() override = default;

                virtual const AZStd::string& GetSelectedRootBone() const = 0;
                virtual void SetSelectedRootBone(const AZStd::string& selectedRootBone) = 0;
            };
        }
    }
}
