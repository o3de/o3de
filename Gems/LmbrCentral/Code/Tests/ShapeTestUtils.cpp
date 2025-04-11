/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ShapeTestUtils.h>
#include <AzCore/Component/Entity.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace UnitTest
{
    bool IsPointInside(const AZ::Entity& entity, const AZ::Vector3& point)
    {
        bool inside = false;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, point);
        return inside;
    }
} // namespace UnitTest
