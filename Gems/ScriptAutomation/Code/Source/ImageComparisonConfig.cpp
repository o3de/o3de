/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageComparisonConfig.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::ScriptAutomation
{
    void ImageComparisonConfig::Reflect(AZ::ReflectContext* context)
    {
        ImageComparisonToleranceLevel::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ImageComparisonConfig>()
                ->Version(0)
                ->Field("toleranceLevels", &ImageComparisonConfig::m_toleranceLevels)
                ;
        }
    }

    void ImageComparisonToleranceLevel::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ImageComparisonToleranceLevel>()
                ->Version(0)
                ->Field("name", &ImageComparisonToleranceLevel::m_name)
                ->Field("threshold", &ImageComparisonToleranceLevel::m_threshold)
                ->Field("filterImperceptibleDiffs", &ImageComparisonToleranceLevel::m_filterImperceptibleDiffs)
                ;
        }
    }

    AZStd::string ImageComparisonToleranceLevel::ToString() const
    {
        return AZStd::string::format("'%s' (threshold %f%s)", m_name.c_str(), m_threshold, m_filterImperceptibleDiffs ? ", filtered" : "");
    }
} // namespace AZ::ScriptAutomation
