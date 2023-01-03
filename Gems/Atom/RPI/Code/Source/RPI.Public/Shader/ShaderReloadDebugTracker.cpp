/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Module/Environment.h>

namespace AZ
{
    namespace RPI
    {
        namespace ShaderReloadDebugTrackerInternal
        {
            static constexpr char EnabledVariableName[] = "ShaderReloadDebugTracker enabled";
            static constexpr char IndentVariableName[] = "ShaderReloadDebugTracker indent";

            static EnvironmentVariable<bool> s_enabled;
            static EnvironmentVariable<int> s_indent;
        }

        void ShaderReloadDebugTracker::Init()
        {
            MakeReady();
        }

        void ShaderReloadDebugTracker::Shutdown()
        {
            ShaderReloadDebugTrackerInternal::s_enabled.Reset();
            ShaderReloadDebugTrackerInternal::s_indent.Reset();
        }

        void ShaderReloadDebugTracker::MakeReady()
        {
            if (!ShaderReloadDebugTrackerInternal::s_enabled)
            {
                ShaderReloadDebugTrackerInternal::s_enabled = AZ::Environment::CreateVariable<bool>(AZ::Crc32(ShaderReloadDebugTrackerInternal::EnabledVariableName), false);
                ShaderReloadDebugTrackerInternal::s_indent = AZ::Environment::CreateVariable<int>(AZ::Crc32(ShaderReloadDebugTrackerInternal::IndentVariableName), 0);
            }
        }

        bool ShaderReloadDebugTracker::IsEnabled()
        {
#ifdef AZ_ENABLE_SHADER_RELOAD_DEBUG_TRACKER
            MakeReady();

            // Set this to true in the debugger to turn on hot reload tracing.
            // If needed, we could hook this up to a CVar.
            return ShaderReloadDebugTrackerInternal::s_enabled.Get();
#else
            return false;
#endif
        }

        void ShaderReloadDebugTracker::AddIndent()
        {
            MakeReady();
            ShaderReloadDebugTrackerInternal::s_indent.Get() += IndentSpaces;
        }

        void ShaderReloadDebugTracker::RemoveIndent()
        {
            MakeReady();
            ShaderReloadDebugTrackerInternal::s_indent.Get() -= IndentSpaces;
        }

        int ShaderReloadDebugTracker::GetIndent()
        {
            MakeReady();
            return ShaderReloadDebugTrackerInternal::s_indent.Get();
        }

        ShaderReloadDebugTracker::ScopedSection::~ScopedSection()
        {
#ifdef AZ_ENABLE_SHADER_RELOAD_DEBUG_TRACKER
            if (m_shouldEndSection)
            {
                ShaderReloadDebugTracker::EndSection("%s", m_sectionName.c_str());
            }
#endif
        }

    } // namespace RPI
} // namespace AZ
