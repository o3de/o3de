/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StarsComponent.h"

namespace AZ::Render 
{
    StarsComponent::StarsComponent(const StarsComponentConfig& config)
        : BaseClass(config)
    {
    }

    void StarsComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (AZ::SerializeContext * serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<StarsComponent, BaseClass>();
        }
    }
} // namespace AtomSampleViewer
