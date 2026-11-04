/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Translation/TranslationDef.h>

namespace LyViewPane
{
    // Note: QT_TRANSLATE_NOOP marks these strings for translation extraction by lupdate,
    // but does NOT change their runtime values. The constants remain plain English strings
    // and continue to serve as pane identifiers for registration, lookup, and layout persistence.
    // To display the translated version, use:
    //   QCoreApplication::translate("LyViewPane", LyViewPane::XXX)

    static const char* const CategoryTools = QT_TRANSLATE_NOOP("LyViewPane", "Tools");
    static const char* const CategoryOther = QT_TRANSLATE_NOOP("LyViewPane", "Other");
    //----
    static const char* const CategoryEditor = QT_TRANSLATE_NOOP("LyViewPane", "Editor");
    static const char* const CategoryViewport = QT_TRANSLATE_NOOP("LyViewPane", "Viewport");


    // Putting these names here so that when view panes are opened
    // from other areas of Editor code, they still work when the name changes.
    static const char* const AssetBrowser = QT_TRANSLATE_NOOP("LyViewPane", "Asset Browser");
    static const char* const AssetEditor = QT_TRANSLATE_NOOP("LyViewPane", "Asset Editor");
    static const char* const AssetBrowserInspector = QT_TRANSLATE_NOOP("LyViewPane", "Asset Browser Inspector");
    static const char* const EntityOutliner = QT_TRANSLATE_NOOP("LyViewPane", "Entity Outliner");
    static const char* const Inspector = QT_TRANSLATE_NOOP("LyViewPane", "Inspector");
    static const char* const EntityInspectorPinned = QT_TRANSLATE_NOOP("LyViewPane", "Pinned Entity Inspector");
    static const char* const LevelInspector = QT_TRANSLATE_NOOP("LyViewPane", "Level Inspector");
    static const char* const ProjectSettingsTool = QT_TRANSLATE_NOOP("LyViewPane", "Edit Platform Settings...");
    static const char* const ErrorReport = QT_TRANSLATE_NOOP("LyViewPane", "Error Report");
    static const char* const Console = QT_TRANSLATE_NOOP("LyViewPane", "Console");
    static const char* const ConsoleMenuName = QT_TRANSLATE_NOOP("LyViewPane", "&Console");
    static const char* const ConsoleVariables = QT_TRANSLATE_NOOP("LyViewPane", "Console Variables");
    static const char* const TrackView = QT_TRANSLATE_NOOP("LyViewPane", "Track View");
    static const char* const ScriptCanvas = QT_TRANSLATE_NOOP("LyViewPane", "Script Canvas");
    static const char* const UiEditor = QT_TRANSLATE_NOOP("LyViewPane", "UI Editor");

    static const char* const EditorSettingsManager = QT_TRANSLATE_NOOP("LyViewPane", "Editor Settings Manager");
    static const char* const AudioControlsEditor = QT_TRANSLATE_NOOP("LyViewPane", "Audio Controls Editor");
    static const char* const SubstanceEditor = QT_TRANSLATE_NOOP("LyViewPane", "Substance Editor");
    static const char* const LandscapeCanvas = QT_TRANSLATE_NOOP("LyViewPane", "Landscape Canvas");
    static const char* const AnimationEditor = QT_TRANSLATE_NOOP("LyViewPane", "EMotion FX Animation Editor");
    static const char* const PhysXConfigurationEditor = QT_TRANSLATE_NOOP("LyViewPane", "PhysX Configuration");

    static const char* const SliceRelationships = QT_TRANSLATE_NOOP("LyViewPane", "Slice Relationship View");

    const int NO_BUILTIN_ACTION = -1;
}

