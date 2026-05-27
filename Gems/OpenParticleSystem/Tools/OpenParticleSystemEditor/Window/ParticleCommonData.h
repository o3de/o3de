/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace OpenParticleSystemEditor
{
    enum LineWidgetIndex : AZ::u8
    {
        WIDGET_LINE_EMITTER,
        WIDGET_LINE_SPAWN,
        WIDGET_LINE_PARTICLES,
        WIDGET_LINE_LOCATION,
        WIDGET_LINE_VELOCITY,
        WIDGET_LINE_COLOR,
        WIDGET_LINE_SIZE,
        WIDGET_LINE_FORCE,
        WIDGET_LINE_LIGHT,
        WIDGET_LINE_SUBUV,
        WIDGET_LINE_EVENT,
        WIDGET_LINE_RENDERER,
        WIDGET_LINE_TITLE,
        WIDGET_LINE_BLANK
    };

    enum RendererIndex : AZ::u8
    {
        NA_RENDERER_INDEX,
        SPRITE_RENDERER_INDEX,
        MESH_RENDERER_INDEX,
        RIBBON_RENDERER_INDEX
    };

    constexpr AZ::u8 SKELETON_INDEX = 6;
    constexpr AZ::u8 PARTICLE_COUNT = 12;
 
    const AZStd::string NOT_AVAILABLE = "NA";
    const AZStd::string PARTICLE_LINE_NAMES[PARTICLE_COUNT] = { "Emitter", "Spawn", "Particles", "Shape", "Velocity", "Color",
                                                  "Size", "Force", "Light", "SubUV", "Event", "Renderer" };
} // namespace OpenParticleSystemEditor
