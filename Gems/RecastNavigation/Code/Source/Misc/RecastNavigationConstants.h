/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>

namespace RecastNavigation
{
    inline constexpr AZ::TypeId RecastNavigationMeshComponentTypeId{ "{a281f314-a525-4c05-876d-17eb632f14b4}" };
    inline constexpr AZ::TypeId EditorRecastNavigationMeshComponentTypeId{ "{22D516D4-C98D-4783-85A4-1ABE23CAB4D4}" };

    inline constexpr AZ::TypeId RecastNavigationPhysXProviderComponentTypeId{ "{4bc92ce5-e179-4985-b0b1-f22bff6006dd}" };
    inline constexpr AZ::TypeId EditorRecastNavigationPhysXProviderComponentTypeId{ "{F1E57D0B-11A1-46C2-876D-720DD40CB14D}" };
} // namespace RecastNavigation
