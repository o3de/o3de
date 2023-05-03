/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    class Entity;
    class Vector3;
} // namespace AZ

namespace UnitTest
{
    bool IsPointInside(const AZ::Entity& entity, const AZ::Vector3& point);
} // namespace UnitTest
