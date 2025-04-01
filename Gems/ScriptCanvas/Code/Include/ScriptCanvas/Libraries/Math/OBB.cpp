/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OBB.h"

#include <Include/ScriptCanvas/Libraries/Math/OBB.generated.h>

namespace ScriptCanvas
{
    namespace OBBFunctions
    {
        using namespace Data;

        OBBType FromAabb(const AABBType& source)
        {
            return OBBType::CreateFromAabb(source);
        }

        OBBType FromPositionRotationAndHalfLengths(Vector3Type position, QuaternionType rotation, Vector3Type halfLengths)
        {
            return OBBType::CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths);
        }

        BooleanType IsFinite(const OBBType& source)
        {
            return source.IsFinite();
        }

        Vector3Type GetAxisX(const OBBType& source)
        {
            return source.GetAxisX();
        }

        Vector3Type GetAxisY(const OBBType& source)
        {
            return source.GetAxisY();
        }

        Vector3Type GetAxisZ(const OBBType& source)
        {
            return source.GetAxisZ();
        }

        Vector3Type GetPosition(const OBBType& source)
        {
            return source.GetPosition();
        }
    } // namespace OBBFunctions
} // namespace ScriptCanvas
