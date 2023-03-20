/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <FeatureSchema.h>

namespace EMotionFX::MotionMatching
{
    struct DefaultFeatureSchemaInitSettings
    {
        AZStd::string m_rootJointName;
        AZStd::string m_leftFootJointName;
        AZStd::string m_rightFootJointName;
        AZStd::string m_pelvisJointName;
    };
    void DefaultFeatureSchema(FeatureSchema& featureSchema, DefaultFeatureSchemaInitSettings settings);
} // namespace EMotionFX::MotionMatching
