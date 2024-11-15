/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <LookupTable/LookupTableAsset.h>

namespace AZ
{
    namespace Render
    {
        void LookupTableAsset::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<LookupTableAsset>()
                    ->Version(1)
                    ->Field("Name", &LookupTableAsset::m_name)
                    ->Field("Intervals", &LookupTableAsset::m_intervals)
                    ->Field("Values", &LookupTableAsset::m_values)
                ;
            }
        }
    } // namespace RPI
} // namespace AZ
