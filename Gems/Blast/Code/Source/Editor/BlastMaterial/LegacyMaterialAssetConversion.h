/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    class ReflectContext;
}

namespace Blast
{
    // Reflection of legacy blast material classes.
    // Used when converting old material asset to new one.
    void ReflectLegacyMaterialClasses(AZ::ReflectContext* context);
} // namespace Blast
