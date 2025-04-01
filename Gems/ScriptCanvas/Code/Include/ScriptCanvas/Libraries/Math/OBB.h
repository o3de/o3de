/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace OBBFunctions
    {
        using namespace Data;

        OBBType FromAabb(const AABBType& source);

        OBBType FromPositionRotationAndHalfLengths(Vector3Type position, QuaternionType rotation, Vector3Type halfLengths);

        BooleanType IsFinite(const OBBType& source);

        Vector3Type GetAxisX(const OBBType& source);

        Vector3Type GetAxisY(const OBBType& source);

        Vector3Type GetAxisZ(const OBBType& source);

        Vector3Type GetPosition(const OBBType& source);
    } // namespace OBBFunctions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/OBB.generated.h>
