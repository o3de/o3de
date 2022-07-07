/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/ColorGrading/HDRColorGradingComponent.h>

namespace AZ
{
    namespace Render
    {
        HDRColorGradingComponent::HDRColorGradingComponent(const HDRColorGradingComponentConfig& config)
            : BaseClass(config)
        {
        }

        void HDRColorGradingComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<HDRColorGradingComponent, BaseClass>();
            }
        }
    } // namespace Render
} // namespace AZ
