/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${Name}Settings.h    (variant: PostVolume)
// -----------------------------------------------------------------------------
// Per-area artist-tunable parameters for ${Name}Pass.
//
// This is intentionally a SMALL POD. The wizard ships it with three demo
// fields (intensity, tint, exposure) so you have something to drag in the
// Inspector immediately. Add or remove fields freely; just remember:
//   1. Reflect new fields in Reflect() so they save and show in Edit Context.
//   2. Add matching constants to PassSrg in ${Name}.azsl.
//   3. Cache the SRG indices in ${Name}Pass::InitializeInternal.
//   4. Push the values in ${Name}Pass::FrameBeginInternal.
//
// SCOPE NOTE:
//   This is a deliberately simplified take on Atom's full PostProcess Volume
//   system. Atom's real BloomSettings / DepthOfFieldSettings / etc. blend
//   between multiple settings sources by priority and falloff radius. That
//   pattern needs about 5 more files (ConfigClass, Controller, runtime +
//   editor components, FeatureProcessor settings storage). The wizard's
//   "PostVolume" mode gives you the artist-facing surface (a component you
//   drop on an entity, sliders, an enabled checkbox) using a singleton
//   provider pattern -- the FIRST active ${Name}SettingsComponent in the
//   level wins. Graduate to Atom's full pattern when you actually need
//   priority-based blending.
// =============================================================================

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace ${GemName}
{
    struct ${SanitizedCppName}Settings
    {
        AZ_TYPE_INFO(${GemName}::${SanitizedCppName}Settings, "{${Random_Uuid}}");

        static void Reflect(AZ::ReflectContext* context);

        //! Master enable for the effect. When false, the Pass's FrameBegin
        //! pushes a zero intensity so the shader is a pass-through. Toggling
        //! this in the editor should be the first thing you try when verifying
        //! the wiring.
        bool m_enabled = true;

        //! 0.0 -> no effect, 1.0 -> full strength. Bound to the shader as a
        //! float you multiply your computed delta by:
        //!   final = lerp(src, effect, m_intensity)
        float m_intensity = 1.0f;

        //! Demo field: per-channel multiplicative tint. Replace with whatever
        //! controls the effect needs.
        AZ::Color m_tint = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);

        //! Demo field: stop-style exposure adjustment. 0 = no change.
        float m_exposureEv = 0.0f;
    };

} // namespace ${GemName}
