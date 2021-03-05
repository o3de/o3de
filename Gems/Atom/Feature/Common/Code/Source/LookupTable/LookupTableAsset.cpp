/**
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/Feature/LookupTable/LookupTableAsset.h>

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
