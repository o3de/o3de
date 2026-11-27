/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/BehaviorContext.h>

namespace OpenXRVk
{
    //! Reflects OpenXR related AZ::Interfaces as static classes
    //! in the behavior context.
    void OpenXRBehaviorReflect(AZ::BehaviorContext& context);
}

