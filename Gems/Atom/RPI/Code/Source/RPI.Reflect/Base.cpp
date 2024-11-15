/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Base.h>

namespace AZ
{
    namespace RPI
    {
        bool Validation::s_isEnabled = RHI::BuildOptions::IsDebugBuild || RHI::BuildOptions::IsProfileBuild;

        void Validation::SetEnabled(bool enabled)
        {
            s_isEnabled = enabled;
        }
    } // namespace RPI
} // namespace AZ
