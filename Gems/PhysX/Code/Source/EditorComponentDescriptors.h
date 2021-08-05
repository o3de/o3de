/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/list.h>

namespace AZ
{
    class ComponentDescriptor;
}

namespace PhysX
{
    AZStd::list<AZ::ComponentDescriptor*> GetEditorDescriptors();
} // namespace PhysX
