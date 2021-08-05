/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Display Settings definition.


#ifndef CRYINCLUDE_EDITOR_DISPLAYSETTINGS_H
#define CRYINCLUDE_EDITOR_DISPLAYSETTINGS_H

#pragma once

//////////////////////////////////////////////////////////////////////////
enum EDisplayRenderFlags
{
    RENDER_FLAG_BBOX            = 1 << 0,
    RENDER_FLAG_ROADS           = 1 << 1,
    RENDER_FLAG_DECALS      = 1 << 2,
    RENDER_FLAG_DETAILTEX   = 1 << 3,
    RENDER_FLAG_FOG             = 1 << 4,
    RENDER_FLAG_INDOORS     = 1 << 5,
    RENDER_FLAG_LIVINGOBJ   = 1 << 6,
    RENDER_FLAG_STATICOBJ   = 1 << 7,
    RENDER_FLAG_SHADOWMAPS = 1 << 8,
    RENDER_FLAG_SKYBOX      = 1 << 9,
    RENDER_FLAG_TERRAIN     = 1 << 10,
    RENDER_FLAG_WATER           = 1 << 11,
    RENDER_FLAG_DETAILOBJ   = 1 << 12,

    RENDER_FLAG_PARTICLES = 1 << 14,
    RENDER_FLAG_VOXELS    = 1 << 15,
    RENDER_FLAG_CLOUDS    = 1 << 16,
    RENDER_FLAG_IMPOSTERS = 1 << 17,
    RENDER_FLAG_BEAMS     = 1 << 18,
    RENDER_FLAG_GI              = 1 << 19,
    RENDER_FLAG_ALPHABLEND = 1 << 20,

    //update this!
    RENDER_FLAG_LAST_ONE = 1 << 21

        // Debugging options.
        //RENDER_FLAG_PROFILE = 1<<22,
        //RENDER_FLAG_AIDEBUGDRAW = 1<<23,
        //RENDER_FLAG_MEMINFO = 1<<24,
};

//////////////////////////////////////////////////////////////////////////
enum EDisplaySettingsFlags
{
    SETTINGS_NOCOLLISION = 0x01,//!< Disable collision with terrain.
    SETTINGS_NOLABELS = 0x02,       //!< Do not draw labels.
    SETTINGS_PHYSICS = 0x04,        //!< Physics simulation is enabled.
    SETTINGS_HIDE_TRACKS = 0x08, //!< Enable displaying of animation tracks in views.
    SETTINGS_HIDE_LINKS = 0x10, //!< Enable displaying of links between objects.
    SETTINGS_HIDE_HELPERS = 0x20, //!< Disable displaying of all object helpers.
    SETTINGS_SHOW_DIMENSIONFIGURES = 0x40, //!< Enable displaying of dimension figures.

    SETTINGS_SERIALIZABLE_FLAGS_MASK = ~(SETTINGS_PHYSICS),
};

//////////////////////////////////////////////////////////////////////////
enum EDebugSettingsFlags
{
    DBG_MEMINFO                     = 0x002,
    DBG_MEMSTATS                    = 0x004,
    DBG_TEXTURE_MEMINFO     = 0x008,
    DBG_AI_DEBUGDRAW            = 0x010,
    DBG_PHYSICS_DEBUGDRAW   = 0x020,
    DBG_RENDERER_PROFILE    = 0x040,
    DBG_RENDERER_PROFILESHADERS = 0x080,
    DBG_RENDERER_OVERDRAW   = 0x100,
    DBG_RENDERER_RESOURCES  = 0x200,
    DBG_FRAMEPROFILE                = 0x400,
    DBG_DEBUG_LIGHTS                = 0x800,
    DBG_BUDGET_MONITORING       = 0x1000,
    DBG_HIGHLIGHT_BREAKABLE = 0x2000,
    DBG_HIGHLIGHT_MISSING_SURFACE_TYPE = 0x4000
};

/*!
 *  CDisplaySettings is a collection of information about how to display current views.
 */
class SANDBOX_API CDisplaySettings
{
public:
    CDisplaySettings();
    ~CDisplaySettings();

    void PostInitApply();

    void SetSettings(int flags) { m_flags = flags; };
    int GetSettings() const { return m_flags; };

    void SetObjectHideMask(int m_objectHideMask);
    int GetObjectHideMask() const { return m_objectHideMask; }

    void SetRenderFlags(int flags);
    int GetRenderFlags() const { return m_renderFlags; }

    void SetDebugFlags(int flags);
    int GetDebugFlags() const { return m_debugFlags; }

    void DisplayLabels(bool bEnable);
    bool IsDisplayLabels() const { return (m_flags & SETTINGS_NOLABELS) == 0; };

    void DisplayTracks(bool bEnable);
    bool IsDisplayTracks() const { return (m_flags & SETTINGS_HIDE_TRACKS) == 0; };

    void DisplayLinks(bool bEnable);
    bool IsDisplayLinks() const { return (m_flags & SETTINGS_HIDE_LINKS) == 0; };

    void DisplayHelpers(bool bEnable);
    bool IsDisplayHelpers() const { return (m_flags & SETTINGS_HIDE_HELPERS) == 0; };

    void DisplayDimensionFigures(bool bEnable);
    bool IsDisplayDimensionFigures() const {    return m_flags & SETTINGS_SHOW_DIMENSIONFIGURES;  };

    void SetLabelsDistance(float dist) { m_labelsDistance = dist; };
    float GetLabelsDistance() const { return m_labelsDistance; };

    bool IsHighlightBreakable() const { return (m_debugFlags & DBG_HIGHLIGHT_BREAKABLE) != 0; }
    void SetHighlightBreakable(bool bEnable);

    bool IsHighlightMissingSurfaceType() const { return (m_debugFlags & DBG_HIGHLIGHT_MISSING_SURFACE_TYPE) != 0; }
    void SetHighlightMissingSurfaceType(bool bEnable);

    void SaveRegistry();
    void LoadRegistry();

private:
    // Restrict access.
    CDisplaySettings(const CDisplaySettings&) {}
    void operator=(const CDisplaySettings&) {}

    void SetCVar(const char* cvar, bool val);
    void SetCVarInt(const char* cvar, int val);
    void SaveValue(const char* sSection, const char* sKey, int value);
    void LoadValue(const char* sSection, const char* sKey, int& value);
    void LoadValue(const char* sSection, const char* sKey, bool& value);

    int m_objectHideMask;
    int m_renderFlags;
    int m_flags;

    //! Debug/profile settings.
    //! @see EDebugSettingsFlags.
    int m_debugFlags;
    float m_labelsDistance;
};

#endif // CRYINCLUDE_EDITOR_DISPLAYSETTINGS_H
