/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>

namespace AZ
{
    namespace RPI
    {
        bool ShaderReloadDebugTracker::s_enabled = false;
        int ShaderReloadDebugTracker::s_indent = 0;

        bool ShaderReloadDebugTracker::IsEnabled()
        {
#ifdef AZ_ENABLE_SHADER_RELOAD_DEBUG_TRACKER
            // Set this to true in the debugger to turn on hot reload tracing.
            // If needed, we could hook this up to a CVar.
            return s_enabled;
#else
            return false;
#endif
        }

        ShaderReloadDebugTracker::ScopedSection::~ScopedSection()
        {
            ShaderReloadDebugTracker::EndSection("%s", m_sectionName.c_str());
        }

    } // namespace RPI
} // namespace AZ
