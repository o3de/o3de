/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        class MotionExtractionRequests
            : public AZ::ComponentBus
        {
        public:
            virtual void ExtractMotion(const AZ::Vector3& deltaPosition, float deltaTime) = 0;
        };
        using MotionExtractionRequestBus = AZ::EBus<MotionExtractionRequests>;
    }
}
