/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>

namespace ScriptCanvas
{
    namespace EntityFunctions
    {
        AZ::Vector3 GetEntityRight(AZ::EntityId entityId, double scale);

        AZ::Vector3 GetEntityForward(AZ::EntityId entityId, double scale);

        AZ::Vector3 GetEntityUp(AZ::EntityId entityId, double scale);

        void Rotate(const AZ::EntityId& entityId, const AZ::Vector3& angles);

        bool IsActive(const AZ::EntityId& entityId);

        bool IsValid(const AZ::EntityId& source);

        AZStd::string ToString(const AZ::EntityId& source);
    } // namespace EntityFunctions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Entity/EntityFunctions.generated.h>
