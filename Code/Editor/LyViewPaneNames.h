/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace LyViewPane
{
    static const char* const CategoryTools = "Tools";
    static const char* const CategoryOther = "Other";
    //----
    static const char* const CategoryEditor = "Editor";
    static const char* const CategoryViewport = "Viewport";


    // Putting these names here so that when view panes are opened
    // from other areas of Editor code, they still work when the name changes.
    static const char* const AssetBrowser = "Asset Browser";
    static const char* const AssetEditor = "Asset Editor";
    static const char* const AssetBrowserInspector = "Asset Browser Inspector";
    static const char* const EntityOutliner = "Entity Outliner";
    static const char* const Inspector = "Inspector";
    static const char* const EntityInspectorPinned = "Pinned Entity Inspector";
    static const char* const LevelInspector = "Level Inspector";
    static const char* const ProjectSettingsTool = "Edit Platform Settings...";
    static const char* const ErrorReport = "Error Report";
    static const char* const Console = "Console";
    static const char* const ConsoleMenuName = "&Console";
    static const char* const ConsoleVariables = "Console Variables";
    static const char* const TrackView = "Track View";
    static const char* const ScriptCanvas = "Script Canvas";
    static const char* const UiEditor = "UI Editor";

    static const char* const EditorSettingsManager = "Editor Settings Manager";
    static const char* const ParticleEditor = "Particle Editor";
    static const char* const AudioControlsEditor = "Audio Controls Editor";
    static const char* const SubstanceEditor = "Substance Editor";
    static const char* const LandscapeCanvas = "Landscape Canvas";
    static const char* const AnimationEditor = "EMotion FX Animation Editor";
    static const char* const PhysXConfigurationEditor = "PhysX Configuration";

    static const char* const SliceRelationships = "Slice Relationship View";

    const int NO_BUILTIN_ACTION = -1;
}

