/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CRC.h"

#include <AzCore/Math/Crc.h>

#include <Include/ScriptCanvas/Libraries/Math/CRC.generated.h>

namespace ScriptCanvas
{
    namespace CRCFunctions
    {
        Data::CRCType FromString(Data::StringType value)
        {
            return AZ::Crc32(value.data());
        }
    } // namespace CRCFunctions
} // namespace ScriptCanvas
