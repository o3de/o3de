/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "Settings.h"
#include "EditorViewportSettings.h"

// Qt
#include <QGuiApplication>
#include <QOperatingSystemVersion>
#include <QScreen>

// AzCore
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Console/IConsole.h>

// AzFramework
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

// AzToolsFramework
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

// Editor
#include "CryEdit.h"
#include "MainWindow.h"


//////////////////////////////////////////////////////////////////////////
// Global Instance of Editor settings.
//////////////////////////////////////////////////////////////////////////
SANDBOX_API SEditorSettings gSettings;

Q_GLOBAL_STATIC(QSettings, s_editorSettings);

const QString kDefaultColumnsForAssetBrowserList = "Filename,Path,LODs,Triangles,Submeshes,Filesize,Textures,Materials,Tags";
const int EditorSettingsVersion = 2; // bump this up on every substantial settings change

void KeepEditorActiveChanged(ICVar* keepEditorActive)
{
    const int iCVarKeepEditorActive = keepEditorActive->GetIVal();
    CCryEditApp::instance()->KeepEditorActive(iCVarKeepEditorActive);
}

void ToolbarIconSizeChanged(ICVar* toolbarIconSize)
{
    MainWindow::instance()->AdjustToolBarIconSize(static_cast<AzQtComponents::ToolBar::ToolBarIconSize>(toolbarIconSize->GetIVal()));
}

class SettingsGroup
{
public:
    explicit SettingsGroup(const QString& group)
        : m_group(group)
    {
        for (auto g : m_group.split('\\'))
        {
            s_editorSettings()->beginGroup(g);
        }
    }
    ~SettingsGroup()
    {
        for (auto g : m_group.split('\\'))
        {
            s_editorSettings()->endGroup();
        }
    }

private:
    const QString m_group;
};

namespace
{
    class QtApplicationListener
        : public AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        QtApplicationListener()
        {
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        }

        void NotifyQtApplicationAvailable(QApplication* application) override
        {
            gSettings.viewports.nDragSquareSize = application->startDragDistance();
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
            delete this;
        }
    };
}

//////////////////////////////////////////////////////////////////////////
SEditorSettings::SEditorSettings()
{
    bSettingsManagerMode = false;

    undoLevels = 50;
    m_undoSliceOverrideSaveValue = false;
    bShowDashboardAtStartup = true;
    m_showCircularDependencyError = true;
    bAutoloadLastLevelAtStartup = false;
    bMuteAudio = false;

    objectHideMask = 0;
    objectSelectMask = 0xFFFFFFFF; // Initially all selectable.

    autoBackupEnabled = false;
    autoBackupTime = 10;
    autoBackupMaxCount = 3;
    autoRemindTime = 0;

    bAutoSaveTagPoints = false;

    bNavigationContinuousUpdate = false;
    bNavigationShowAreas = true;
    bNavigationDebugDisplay = false;
    bVisualizeNavigationAccessibility = false;
    navigationDebugAgentType = 0;

    editorConfigSpec = CONFIG_VERYHIGH_SPEC;  //arbitrary choice, but lets assume that we want things to initially look as good as possible in the editor.

    viewports.bAlwaysShowRadiuses = false;
    viewports.bSync2DViews = false;
    viewports.fDefaultAspectRatio = 800.0f / 600.0f;
    viewports.fDefaultFov = DEG2RAD(60); // 60 degrees (to fit with current game)
    viewports.bShowSafeFrame = false;
    viewports.bHighlightSelectedGeometry = false;
    viewports.bHighlightSelectedVegetation = true;
    viewports.bHighlightMouseOverGeometry = true;
    viewports.bShowMeshStatsOnMouseOver = false;
    viewports.bDrawEntityLabels = false;
    viewports.bShowTriggerBounds = false;
    viewports.bShowIcons = true;
    viewports.bDistanceScaleIcons = true;
    viewports.bShowSizeBasedIcons = false;
    viewports.nShowFrozenHelpers = true;
    viewports.bFillSelectedShapes = false;
    viewports.nTopMapTextureResolution = 512;
    viewports.bTopMapSwapXY = false;
    viewports.bShowGridGuide = true;
    viewports.bHideMouseCursorWhenCaptured = true;
    viewports.nDragSquareSize = 0; // We must initialize this after the Qt application object is available; see QtApplicationListener
    viewports.bEnableContextMenu = true;
    viewports.fWarningIconsDrawDistance = 50.0f;
    viewports.bShowScaleWarnings = false;
    viewports.bShowRotationWarnings = false;

    cameraMoveSpeed = 1;
    cameraRotateSpeed = 1;
    cameraFastMoveSpeed = 2;
    stylusMode = false;
    restoreViewportCamera = true;
    wheelZoomSpeed = 1;
    invertYRotation = false;
    invertPan = false;
    fBrMultiplier = 2;
    bPreviewGeometryWindow = true;
    bBackupOnSave = true;
    backupOnSaveMaxCount = 3;
    bApplyConfigSpecInEditor = true;
    showErrorDialogOnLoad = 1;

    consoleBackgroundColorTheme = AzToolsFramework::ConsoleColorTheme::Dark;
    bShowTimeInConsole = false;
    clearConsoleOnGameModeStart = false;

    enableSceneInspector = false;

    strStandardTempDirectory = "Temp";

    // Init source safe params.
    enableSourceControl = true;

#if AZ_TRAIT_OS_PLATFORM_APPLE
    textEditorForScript = "TextEdit";
    textEditorForShaders = "TextEdit";
    textEditorForBspaces = "TextEdit";
    textureEditor = "Photoshop";
#elif defined(AZ_PLATFORM_WINDOWS)
    textEditorForScript = "notepad++.exe";
    textEditorForShaders = "notepad++.exe";
    textEditorForBspaces = "notepad++.exe";
    textureEditor = "Photoshop.exe";
#else
    textEditorForScript = "";
    textEditorForShaders = "";
    textEditorForBspaces = "";
    textureEditor = "";
#endif
    animEditor = "";

    terrainTextureExport = "";

    sTextureBrowserSettings.nCellSize = 128;

    // Experimental features settings
    sExperimentalFeaturesSettings.bTotalIlluminationEnabled = false;

    //
    // Asset Browser settings init
    //
    sAssetBrowserSettings.nThumbSize = 128;
    sAssetBrowserSettings.bShowLoadedInLevel = false;
    sAssetBrowserSettings.bShowUsedInLevel = false;
    sAssetBrowserSettings.bAutoSaveFilterPreset = true;
    sAssetBrowserSettings.bShowFavorites = false;
    sAssetBrowserSettings.bHideLods = false;
    sAssetBrowserSettings.bAutoChangeViewportSelection = false;
    sAssetBrowserSettings.bAutoFilterFromViewportSelection = false;

    smartOpenSettings.rect = QRect();

    //////////////////////////////////////////////////////////////////////////
    // Initialize GUI settings.
    //////////////////////////////////////////////////////////////////////////
    gui.bWindowsVista = QOperatingSystemVersion::current() >= QOperatingSystemVersion(QOperatingSystemVersion::Windows7);

    gui.nToolbarIconSize = static_cast<int>(AzQtComponents::ToolBar::ToolBarIconSize::Default);

    int lfHeight = 8;// -MulDiv(8, GetDeviceCaps(GetDC(nullptr), LOGPIXELSY), 72);
    gui.nDefaultFontHieght = lfHeight;
    gui.hSystemFont = QFont("Ms Shell Dlg 2", lfHeight, QFont::Normal);
    gui.hSystemFontBold = QFont("Ms Shell Dlg 2", lfHeight, QFont::Bold);
    gui.hSystemFontItalic = QFont("Ms Shell Dlg 2", lfHeight, QFont::Normal, true);

    backgroundUpdatePeriod = 0;
    g_TemporaryLevelName = nullptr;

    sliceSettings.dynamicByDefault = false;
    levelSaveSettings.saveAllPrefabsPreference = AzToolsFramework::Prefab::SaveAllPrefabsPreference::AskEveryTime;
}

void SEditorSettings::Connect()
{
    new QtApplicationListener(); // Deletes itself when it's done.
    AzToolsFramework::EditorSettingsAPIBus::Handler::BusConnect();
}

void SEditorSettings::Disconnect()
{
    AzToolsFramework::EditorSettingsAPIBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::SaveValue(const char* sSection, const char* sKey, int value)
{
    const SettingsGroup sg(sSection);
    s_editorSettings()->setValue(sKey, value);

    if (!bSettingsManagerMode)
    {
        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->SaveSetting(sSection, sKey, value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::SaveValue(const char* sSection, const char* sKey, const QColor& value)
{
    const SettingsGroup sg(sSection);
    s_editorSettings()->setValue(sKey, QVariant::fromValue<int>(RGB(value.red(), value.green(), value.blue())));

    if (!bSettingsManagerMode)
    {
        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->SaveSetting(sSection, sKey, value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::SaveValue(const char* sSection, const char* sKey, float value)
{
    const SettingsGroup sg(sSection);
    s_editorSettings()->setValue(sKey, QString::number(value));

    if (!bSettingsManagerMode)
    {
        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->SaveSetting(sSection, sKey, value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::SaveValue(const char* sSection, const char* sKey, const QString& value)
{
    const SettingsGroup sg(sSection);
    s_editorSettings()->setValue(sKey, value);

    if (!bSettingsManagerMode)
    {
        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->SaveSetting(sSection, sKey, value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue(const char* sSection, const char* sKey, int& value)
{
    if (bSettingsManagerMode)
    {
        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->LoadSetting(sSection, sKey, value);
        }

        SaveValue(sSection, sKey, value);
    }
    else
    {
        const SettingsGroup sg(sSection);
        value = s_editorSettings()->value(sKey, value).toInt();

        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->SaveSetting(sSection, sKey, value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue(const char* sSection, const char* sKey, QColor& value)
{
    if (bSettingsManagerMode)
    {
        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->LoadSetting(sSection, sKey, value);
        }

        SaveValue(sSection, sKey, value);
    }
    else
    {
        const SettingsGroup sg(sSection);
        int defaultValue = RGB(value.red(), value.green(), value.blue());
        int v = s_editorSettings()->value(sKey, QVariant::fromValue<int>(defaultValue)).toInt();
        value = QColor(GetRValue(v), GetGValue(v), GetBValue(v));

        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->SaveSetting(sSection, sKey, value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue(const char* sSection, const char* sKey, float& value)
{
    if (bSettingsManagerMode)
    {
        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->LoadSetting(sSection, sKey, value);
        }

        SaveValue(sSection, sKey, value);
    }
    else
    {
        const SettingsGroup sg(sSection);
        const QString defaultVal = s_editorSettings()->value(sKey, QString::number(value)).toString();
        value = defaultVal.toFloat();

        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->SaveSetting(sSection, sKey, value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue(const char* sSection, const char* sKey, bool& value)
{
    if (bSettingsManagerMode)
    {
        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->LoadSetting(sSection, sKey, value);
        }

        SaveValue(sSection, sKey, value);
    }
    else
    {
        const SettingsGroup sg(sSection);
        value = s_editorSettings()->value(sKey, value).toInt();

        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->SaveSetting(sSection, sKey, value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue(const char* sSection, const char* sKey, QString& value)
{
    if (bSettingsManagerMode)
    {
        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->LoadSetting(sSection, sKey, value);
        }

        SaveValue(sSection, sKey, value);
    }
    else
    {
        const SettingsGroup sg(sSection);
        value = s_editorSettings()->value(sKey, value).toString();

        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->SaveSetting(sSection, sKey, value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::LoadValue(const char* sSection, const char* sKey, ESystemConfigSpec& value)
{
    if (bSettingsManagerMode)
    {
        int valueCheck = 0;

        if (GetIEditor()->GetSettingsManager())
        {
            GetIEditor()->GetSettingsManager()->LoadSetting(sSection, sKey, valueCheck);
        }

        if (valueCheck >= CONFIG_AUTO_SPEC && valueCheck < END_CONFIG_SPEC_ENUM)
        {
            value = (ESystemConfigSpec)valueCheck;
            SaveValue(sSection, sKey, value);
        }
    }
    else
    {
        const SettingsGroup sg(sSection);
        auto valuecheck = static_cast<ESystemConfigSpec>(s_editorSettings()->value(sKey, QVariant::fromValue<int>(value)).toInt());
        if (valuecheck >= CONFIG_AUTO_SPEC && valuecheck < END_CONFIG_SPEC_ENUM)
        {
            value = valuecheck;

            if (GetIEditor()->GetSettingsManager())
            {
                GetIEditor()->GetSettingsManager()->SaveSetting(sSection, sKey, value);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::Save(bool isEditorClosing)
{
    QString strStringPlaceholder;

    // Save settings to registry.
    SaveValue("Settings", "UndoLevels", undoLevels);
    SaveValue("Settings", "UndoSliceOverrideSaveValue", m_undoSliceOverrideSaveValue);
    SaveValue("Settings", "ShowWelcomeScreenAtStartup", bShowDashboardAtStartup);
    SaveValue("Settings", "ShowCircularDependencyError", m_showCircularDependencyError);
    SaveValue("Settings", "LoadLastLevelAtStartup", bAutoloadLastLevelAtStartup);
    SaveValue("Settings", "MuteAudio", bMuteAudio);
    SaveValue("Settings", "AutoBackup", autoBackupEnabled);
    SaveValue("Settings", "AutoBackupTime", autoBackupTime);
    SaveValue("Settings", "AutoBackupMaxCount", autoBackupMaxCount);
    SaveValue("Settings", "AutoRemindTime", autoRemindTime);
    SaveValue("Settings", "CameraMoveSpeed", cameraMoveSpeed);
    SaveValue("Settings", "CameraRotateSpeed", cameraRotateSpeed);
    SaveValue("Settings", "StylusMode", stylusMode);
    SaveValue("Settings", "RestoreViewportCamera", restoreViewportCamera);
    SaveValue("Settings", "WheelZoomSpeed", wheelZoomSpeed);
    SaveValue("Settings", "InvertYRotation", invertYRotation);
    SaveValue("Settings", "InvertPan", invertPan);
    SaveValue("Settings", "BrMultiplier", fBrMultiplier);
    SaveValue("Settings", "CameraFastMoveSpeed", cameraFastMoveSpeed);
    SaveValue("Settings", "PreviewGeometryWindow", bPreviewGeometryWindow);
    SaveValue("Settings", "AutoSaveTagPoints", bAutoSaveTagPoints);

    SaveValue("Settings\\Navigation", "NavigationContinuousUpdate", bNavigationContinuousUpdate);
    SaveValue("Settings\\Navigation", "NavigationShowAreas", bNavigationShowAreas);
    SaveValue("Settings\\Navigation", "NavigationDebugDisplay", bNavigationDebugDisplay);
    SaveValue("Settings\\Navigation", "NavigationDebugAgentType", navigationDebugAgentType);
    SaveValue("Settings\\Navigation", "VisualizeNavigationAccessibility", bVisualizeNavigationAccessibility);

    SaveValue("Settings", "BackupOnSave", bBackupOnSave);
    SaveValue("Settings", "SaveBackupMaxCount", backupOnSaveMaxCount);
    SaveValue("Settings", "ApplyConfigSpecInEditor", bApplyConfigSpecInEditor);

    SaveValue("Settings", "editorConfigSpec", editorConfigSpec);

    SaveValue("Settings", "TemporaryDirectory", strStandardTempDirectory);

    SaveValue("Settings", "ConsoleBackgroundColorThemeV2", (int)consoleBackgroundColorTheme);

    SaveValue("Settings", "ClearConsoleOnGameModeStart", clearConsoleOnGameModeStart);

    SaveValue("Settings", "ShowTimeInConsole", bShowTimeInConsole);

    SaveValue("Settings", "EnableSceneInspector", enableSceneInspector);

    //////////////////////////////////////////////////////////////////////////
    // Viewport settings.
    //////////////////////////////////////////////////////////////////////////
    SaveValue("Settings", "AlwaysShowRadiuses", viewports.bAlwaysShowRadiuses);
    SaveValue("Settings", "Sync2DViews", viewports.bSync2DViews);
    SaveValue("Settings", "DefaultFov", viewports.fDefaultFov);
    SaveValue("Settings", "AspectRatio", viewports.fDefaultAspectRatio);
    SaveValue("Settings", "ShowSafeFrame", viewports.bShowSafeFrame);
    SaveValue("Settings", "HighlightSelectedGeometry", viewports.bHighlightSelectedGeometry);
    SaveValue("Settings", "HighlightSelectedVegetation", viewports.bHighlightSelectedVegetation);
    SaveValue("Settings", "HighlightMouseOverGeometry", viewports.bHighlightMouseOverGeometry);
    SaveValue("Settings", "ShowMeshStatsOnMouseOver", viewports.bShowMeshStatsOnMouseOver);
    SaveValue("Settings", "DrawEntityLabels", viewports.bDrawEntityLabels);
    SaveValue("Settings", "ShowTriggerBounds", viewports.bShowTriggerBounds);
    SaveValue("Settings", "ShowIcons", viewports.bShowIcons);
    SaveValue("Settings", "ShowSizeBasedIcons", viewports.bShowSizeBasedIcons);
    SaveValue("Settings", "ShowFrozenHelpers", viewports.nShowFrozenHelpers);
    SaveValue("Settings", "FillSelectedShapes", viewports.bFillSelectedShapes);
    SaveValue("Settings", "MapTextureResolution", viewports.nTopMapTextureResolution);
    SaveValue("Settings", "MapSwapXY", viewports.bTopMapSwapXY);
    SaveValue("Settings", "ShowGridGuide", viewports.bShowGridGuide);
    SaveValue("Settings", "HideMouseCursorOnCapture", viewports.bHideMouseCursorWhenCaptured);
    SaveValue("Settings", "DragSquareSize", viewports.nDragSquareSize);
    SaveValue("Settings", "EnableContextMenu", viewports.bEnableContextMenu);
    SaveValue("Settings", "ToolbarIconSizeV2", gui.nToolbarIconSize);
    SaveValue("Settings", "WarningIconsDrawDistance", viewports.fWarningIconsDrawDistance);
    SaveValue("Settings", "ShowScaleWarnings", viewports.bShowScaleWarnings);
    SaveValue("Settings", "ShowRotationWarnings", viewports.bShowRotationWarnings);

    SaveValue("Settings", "TextEditorScript", textEditorForScript);
    SaveValue("Settings", "TextEditorShaders", textEditorForShaders);
    SaveValue("Settings", "TextEditorBSpaces", textEditorForBspaces);
    SaveValue("Settings", "TextureEditor", textureEditor);
    SaveValue("Settings", "AnimationEditor", animEditor);

    SaveEnableSourceControlFlag(true);

    //////////////////////////////////////////////////////////////////////////
    // Snapping Settings.
    SaveValue("Settings\\Snap", "ConstructPlaneSize", snap.constructPlaneSize);
    SaveValue("Settings\\Snap", "ConstructPlaneDisplay", snap.constructPlaneDisplay);
    SaveValue("Settings\\Snap", "SnapMarkerDisplay", snap.markerDisplay);
    SaveValue("Settings\\Snap", "SnapMarkerColor", snap.markerColor);
    SaveValue("Settings\\Snap", "SnapMarkerSize", snap.markerSize);
    SaveValue("Settings\\Snap", "GridUserDefined", snap.bGridUserDefined);
    SaveValue("Settings\\Snap", "GridGetFromSelected", snap.bGridGetFromSelected);
    //////////////////////////////////////////////////////////////////////////

    SaveValue("Settings", "TerrainTextureExport", terrainTextureExport);

    //////////////////////////////////////////////////////////////////////////
    // Texture browser settings
    //////////////////////////////////////////////////////////////////////////
    SaveValue("Settings\\TextureBrowser", "Cell Size", sTextureBrowserSettings.nCellSize);

    //////////////////////////////////////////////////////////////////////////
    // Experimental features settings
    //////////////////////////////////////////////////////////////////////////
    SaveValue("Settings\\ExperimentalFeatures", "TotalIlluminationEnabled", sExperimentalFeaturesSettings.bTotalIlluminationEnabled);

    ///////////////////////////////////////////////////////////////////////////
    SaveValue("Settings\\SelectObjectDialog", "Columns", selectObjectDialog.columns);
    SaveValue("Settings\\SelectObjectDialog", "LastColumnSortDirection", selectObjectDialog.nLastColumnSortDirection);

    //////////////////////////////////////////////////////////////////////////
    // Asset browser settings
    //////////////////////////////////////////////////////////////////////////
    SaveValue("Settings\\AssetBrowser", "ThumbSize", sAssetBrowserSettings.nThumbSize);
    SaveValue("Settings\\AssetBrowser", "ShowLoadedInLevel", sAssetBrowserSettings.bShowLoadedInLevel);
    SaveValue("Settings\\AssetBrowser", "ShowUsedInLevel", sAssetBrowserSettings.bShowUsedInLevel);
    SaveValue("Settings\\AssetBrowser", "FilenameSearch", sAssetBrowserSettings.sFilenameSearch);
    SaveValue("Settings\\AssetBrowser", "PresetName", sAssetBrowserSettings.sPresetName);
    SaveValue("Settings\\AssetBrowser", "ShowDatabases", sAssetBrowserSettings.sVisibleDatabaseNames);
    SaveValue("Settings\\AssetBrowser", "ShowFavorites", sAssetBrowserSettings.bShowFavorites);
    SaveValue("Settings\\AssetBrowser", "HideLods", sAssetBrowserSettings.bHideLods);
    SaveValue("Settings\\AssetBrowser", "AutoSaveFilterPreset", sAssetBrowserSettings.bAutoSaveFilterPreset);
    SaveValue("Settings\\AssetBrowser", "AutoChangeViewportSelection", sAssetBrowserSettings.bAutoChangeViewportSelection);
    SaveValue("Settings\\AssetBrowser", "AutoFilterFromViewportSelection", sAssetBrowserSettings.bAutoFilterFromViewportSelection);
    SaveValue("Settings\\AssetBrowser", "VisibleColumnNames", sAssetBrowserSettings.sVisibleColumnNames);
    SaveValue("Settings\\AssetBrowser", "ColumnNames", sAssetBrowserSettings.sColumnNames);

    //////////////////////////////////////////////////////////////////////////
    // Deep Selection Settings
    //////////////////////////////////////////////////////////////////////////
    SaveValue("Settings", "DeepSelectionNearness", deepSelectionSettings.fRange);
    SaveValue("Settings", "StickDuplicate", deepSelectionSettings.bStickDuplicate);


    //////////////////////////////////////////////////////////////////////////
    // Object Highlight Colors
    //////////////////////////////////////////////////////////////////////////
    SaveValue("Settings\\ObjectColors", "groupHighlight", objectColorSettings.groupHighlight);
    SaveValue("Settings\\ObjectColors", "entityHighlight", objectColorSettings.entityHighlight);
    SaveValue("Settings\\ObjectColors", "BBoxAlpha", objectColorSettings.fBBoxAlpha);
    SaveValue("Settings\\ObjectColors", "GeometryHighlightColor", objectColorSettings.geometryHighlightColor);
    SaveValue("Settings\\ObjectColors", "SolidBrushGeometryHighlightColor", objectColorSettings.solidBrushGeometryColor);
    SaveValue("Settings\\ObjectColors", "GeometryAlpha", objectColorSettings.fGeomAlpha);
    SaveValue("Settings\\ObjectColors", "ChildGeometryAlpha", objectColorSettings.fChildGeomAlpha);

    //////////////////////////////////////////////////////////////////////////
    // Smart file open settings
    //////////////////////////////////////////////////////////////////////////
    SaveValue("Settings\\SmartFileOpen", "LastSearchTerm", smartOpenSettings.lastSearchTerm);
    SaveValue("Settings\\SmartFileOpen", "DlgRect.Left", smartOpenSettings.rect.left());
    SaveValue("Settings\\SmartFileOpen", "DlgRect.Top", smartOpenSettings.rect.top());
    SaveValue("Settings\\SmartFileOpen", "DlgRect.Right", smartOpenSettings.rect.right());
    SaveValue("Settings\\SmartFileOpen", "DlgRect.Bottom", smartOpenSettings.rect.bottom());

    //////////////////////////////////////////////////////////////////////////
    // Slice settings
    //////////////////////////////////////////////////////////////////////////
    SaveValue("Settings\\Slices", "DynamicByDefault", sliceSettings.dynamicByDefault);

    s_editorSettings()->sync();

    // --- Settings Registry values

    // Prefab System UI
    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::SetPrefabSystemEnabled, prefabSystem);

    AzToolsFramework::Prefab::PrefabLoaderInterface* prefabLoaderInterface =
        AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
    prefabLoaderInterface->SetSaveAllPrefabsPreference(levelSaveSettings.saveAllPrefabsPreference);

    if (!isEditorClosing)
    {
        SaveSettingsRegistryFile();
    }
}

//////////////////////////////////////////////////////////////////////////
void SEditorSettings::Load()
{
    AzToolsFramework::Prefab::PrefabLoaderInterface* prefabLoaderInterface =
        AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
    levelSaveSettings.saveAllPrefabsPreference = prefabLoaderInterface->GetSaveAllPrefabsPreference();

    // Load from Settings Registry
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        prefabSystem, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    const int settingsVersion = s_editorSettings()->value(QStringLiteral("Settings/EditorSettingsVersion"), 0).toInt();

    if (settingsVersion != EditorSettingsVersion)
    {
        s_editorSettings()->setValue(QStringLiteral("Settings/EditorSettingsVersion"), EditorSettingsVersion);
        Save();
        return;
    }

    QString     strPlaceholderString;
    // Load settings from registry.
    LoadValue("Settings", "UndoLevels", undoLevels);
    LoadValue("Settings", "UndoSliceOverrideSaveValue", m_undoSliceOverrideSaveValue);
    LoadValue("Settings", "ShowWelcomeScreenAtStartup", bShowDashboardAtStartup);
    LoadValue("Settings", "ShowCircularDependencyError", m_showCircularDependencyError);
    LoadValue("Settings", "LoadLastLevelAtStartup", bAutoloadLastLevelAtStartup);
    LoadValue("Settings", "MuteAudio", bMuteAudio);
    LoadValue("Settings", "AutoBackup", autoBackupEnabled);
    LoadValue("Settings", "AutoBackupTime", autoBackupTime);
    LoadValue("Settings", "AutoBackupMaxCount", autoBackupMaxCount);
    LoadValue("Settings", "AutoRemindTime", autoRemindTime);
    LoadValue("Settings", "CameraMoveSpeed", cameraMoveSpeed);
    LoadValue("Settings", "CameraRotateSpeed", cameraRotateSpeed);
    LoadValue("Settings", "StylusMode", stylusMode);
    LoadValue("Settings", "RestoreViewportCamera", restoreViewportCamera);
    LoadValue("Settings", "WheelZoomSpeed", wheelZoomSpeed);
    LoadValue("Settings", "InvertYRotation", invertYRotation);
    LoadValue("Settings", "InvertPan", invertPan);
    LoadValue("Settings", "BrMultiplier", fBrMultiplier);
    LoadValue("Settings", "CameraFastMoveSpeed", cameraFastMoveSpeed);
    LoadValue("Settings", "PreviewGeometryWindow", bPreviewGeometryWindow);
    LoadValue("Settings", "AutoSaveTagPoints", bAutoSaveTagPoints);

    LoadValue("Settings\\Navigation", "NavigationContinuousUpdate", bNavigationContinuousUpdate);
    LoadValue("Settings\\Navigation", "NavigationShowAreas", bNavigationShowAreas);
    LoadValue("Settings\\Navigation", "NavigationDebugDisplay", bNavigationDebugDisplay);
    LoadValue("Settings\\Navigation", "NavigationDebugAgentType", navigationDebugAgentType);
    LoadValue("Settings\\Navigation", "VisualizeNavigationAccessibility", bVisualizeNavigationAccessibility);

    LoadValue("Settings", "BackupOnSave", bBackupOnSave);
    LoadValue("Settings", "SaveBackupMaxCount", backupOnSaveMaxCount);
    LoadValue("Settings", "ApplyConfigSpecInEditor", bApplyConfigSpecInEditor);
    LoadValue("Settings", "editorConfigSpec", editorConfigSpec);


    LoadValue("Settings", "TemporaryDirectory", strStandardTempDirectory);

    int consoleBackgroundColorThemeInt = (int)consoleBackgroundColorTheme;
    LoadValue("Settings", "ConsoleBackgroundColorThemeV2", consoleBackgroundColorThemeInt);
    consoleBackgroundColorTheme = static_cast<AzToolsFramework::ConsoleColorTheme>(consoleBackgroundColorThemeInt);
    if (consoleBackgroundColorTheme != AzToolsFramework::ConsoleColorTheme::Dark && consoleBackgroundColorTheme != AzToolsFramework::ConsoleColorTheme::Light)
    {
        consoleBackgroundColorTheme = AzToolsFramework::ConsoleColorTheme::Dark;
    }

    LoadValue("Settings", "ClearConsoleOnGameModeStart", clearConsoleOnGameModeStart);

    LoadValue("Settings", "ShowTimeInConsole", bShowTimeInConsole);

    LoadValue("Settings", "EnableSceneInspector", enableSceneInspector);

    //////////////////////////////////////////////////////////////////////////
    // Viewport Settings.
    //////////////////////////////////////////////////////////////////////////
    LoadValue("Settings", "AlwaysShowRadiuses", viewports.bAlwaysShowRadiuses);
    LoadValue("Settings", "Sync2DViews", viewports.bSync2DViews);
    LoadValue("Settings", "DefaultFov", viewports.fDefaultFov);
    LoadValue("Settings", "AspectRatio", viewports.fDefaultAspectRatio);
    LoadValue("Settings", "ShowSafeFrame", viewports.bShowSafeFrame);
    LoadValue("Settings", "HighlightSelectedGeometry", viewports.bHighlightSelectedGeometry);
    LoadValue("Settings", "HighlightSelectedVegetation", viewports.bHighlightSelectedVegetation);
    LoadValue("Settings", "HighlightMouseOverGeometry", viewports.bHighlightMouseOverGeometry);
    LoadValue("Settings", "ShowMeshStatsOnMouseOver", viewports.bShowMeshStatsOnMouseOver);
    LoadValue("Settings", "DrawEntityLabels", viewports.bDrawEntityLabels);
    LoadValue("Settings", "ShowTriggerBounds", viewports.bShowTriggerBounds);
    LoadValue("Settings", "ShowIcons", viewports.bShowIcons);
    LoadValue("Settings", "ShowSizeBasedIcons", viewports.bShowSizeBasedIcons);
    LoadValue("Settings", "ShowFrozenHelpers", viewports.nShowFrozenHelpers);
    LoadValue("Settings", "FillSelectedShapes", viewports.bFillSelectedShapes);
    LoadValue("Settings", "MapTextureResolution", viewports.nTopMapTextureResolution);
    LoadValue("Settings", "MapSwapXY", viewports.bTopMapSwapXY);
    LoadValue("Settings", "ShowGridGuide", viewports.bShowGridGuide);
    LoadValue("Settings", "HideMouseCursorOnCapture", viewports.bHideMouseCursorWhenCaptured);
    LoadValue("Settings", "DragSquareSize", viewports.nDragSquareSize);
    LoadValue("Settings", "EnableContextMenu", viewports.bEnableContextMenu);
    LoadValue("Settings", "ToolbarIconSizeV2", gui.nToolbarIconSize);
    LoadValue("Settings", "WarningIconsDrawDistance", viewports.fWarningIconsDrawDistance);
    LoadValue("Settings", "ShowScaleWarnings", viewports.bShowScaleWarnings);
    LoadValue("Settings", "ShowRotationWarnings", viewports.bShowRotationWarnings);

    LoadValue("Settings", "TextEditorScript", textEditorForScript);
    LoadValue("Settings", "TextEditorShaders", textEditorForShaders);
    LoadValue("Settings", "TextEditorBSpaces", textEditorForBspaces);
    LoadValue("Settings", "TextureEditor", textureEditor);
    LoadValue("Settings", "AnimationEditor", animEditor);

    LoadEnableSourceControlFlag();

    //////////////////////////////////////////////////////////////////////////
    // Snapping Settings.
    LoadValue("Settings\\Snap", "ConstructPlaneSize", snap.constructPlaneSize);
    LoadValue("Settings\\Snap", "ConstructPlaneDisplay", snap.constructPlaneDisplay);
    LoadValue("Settings\\Snap", "SnapMarkerDisplay", snap.markerDisplay);
    LoadValue("Settings\\Snap", "SnapMarkerColor", snap.markerColor);
    LoadValue("Settings\\Snap", "SnapMarkerSize", snap.markerSize);
    LoadValue("Settings\\Snap", "GridUserDefined", snap.bGridUserDefined);
    LoadValue("Settings\\Snap", "GridGetFromSelected", snap.bGridGetFromSelected);
    //////////////////////////////////////////////////////////////////////////

    LoadValue("Settings", "TerrainTextureExport", terrainTextureExport);

    //////////////////////////////////////////////////////////////////////////
    // Texture browser settings
    //////////////////////////////////////////////////////////////////////////
    LoadValue("Settings\\TextureBrowser", "Cell Size", sTextureBrowserSettings.nCellSize);

    //////////////////////////////////////////////////////////////////////////
    // Experimental features settings
    //////////////////////////////////////////////////////////////////////////
    LoadValue("Settings\\ExperimentalFeatures", "TotalIlluminationEnabled", sExperimentalFeaturesSettings.bTotalIlluminationEnabled);

    //////////////////////////////////////////////////////////////////////////
    LoadValue("Settings\\SelectObjectDialog", "Columns", selectObjectDialog.columns);
    LoadValue("Settings\\SelectObjectDialog", "LastColumnSortDirection", selectObjectDialog.nLastColumnSortDirection);

    //////////////////////////////////////////////////////////////////////////
    // Asset browser settings
    //////////////////////////////////////////////////////////////////////////
    LoadValue("Settings\\AssetBrowser", "ThumbSize", sAssetBrowserSettings.nThumbSize);
    LoadValue("Settings\\AssetBrowser", "ShowLoadedInLevel", sAssetBrowserSettings.bShowLoadedInLevel);
    LoadValue("Settings\\AssetBrowser", "ShowUsedInLevel", sAssetBrowserSettings.bShowUsedInLevel);
    LoadValue("Settings\\AssetBrowser", "FilenameSearch", sAssetBrowserSettings.sFilenameSearch);
    LoadValue("Settings\\AssetBrowser", "PresetName", sAssetBrowserSettings.sPresetName);
    LoadValue("Settings\\AssetBrowser", "ShowDatabases", sAssetBrowserSettings.sVisibleDatabaseNames);
    LoadValue("Settings\\AssetBrowser", "ShowFavorites", sAssetBrowserSettings.bShowFavorites);
    LoadValue("Settings\\AssetBrowser", "HideLods", sAssetBrowserSettings.bHideLods);
    LoadValue("Settings\\AssetBrowser", "AutoSaveFilterPreset", sAssetBrowserSettings.bAutoSaveFilterPreset);
    LoadValue("Settings\\AssetBrowser", "AutoChangeViewportSelection", sAssetBrowserSettings.bAutoChangeViewportSelection);
    LoadValue("Settings\\AssetBrowser", "AutoFilterFromViewportSelection", sAssetBrowserSettings.bAutoFilterFromViewportSelection);
    LoadValue("Settings\\AssetBrowser", "VisibleColumnNames", sAssetBrowserSettings.sVisibleColumnNames);
    LoadValue("Settings\\AssetBrowser", "ColumnNames", sAssetBrowserSettings.sColumnNames);

    if (sAssetBrowserSettings.sVisibleColumnNames == ""
        || sAssetBrowserSettings.sColumnNames == "")
    {
        sAssetBrowserSettings.sColumnNames =
            sAssetBrowserSettings.sVisibleColumnNames = kDefaultColumnsForAssetBrowserList;
    }

    //////////////////////////////////////////////////////////////////////////
    // Deep Selection Settings
    //////////////////////////////////////////////////////////////////////////
    LoadValue("Settings", "DeepSelectionNearness", deepSelectionSettings.fRange);
    LoadValue("Settings", "StickDuplicate", deepSelectionSettings.bStickDuplicate);

    //////////////////////////////////////////////////////////////////////////
    // Object Highlight Colors
    //////////////////////////////////////////////////////////////////////////
    LoadValue("Settings\\ObjectColors", "GroupHighlight", objectColorSettings.groupHighlight);
    LoadValue("Settings\\ObjectColors", "EntityHighlight", objectColorSettings.entityHighlight);
    LoadValue("Settings\\ObjectColors", "BBoxAlpha", objectColorSettings.fBBoxAlpha);
    LoadValue("Settings\\ObjectColors", "GeometryHighlightColor", objectColorSettings.geometryHighlightColor);
    LoadValue("Settings\\ObjectColors", "SolidBrushGeometryHighlightColor", objectColorSettings.solidBrushGeometryColor);
    LoadValue("Settings\\ObjectColors", "GeometryAlpha", objectColorSettings.fGeomAlpha);
    LoadValue("Settings\\ObjectColors", "ChildGeometryAlpha", objectColorSettings.fChildGeomAlpha);

    //////////////////////////////////////////////////////////////////////////
    // Smart file open settings
    //////////////////////////////////////////////////////////////////////////
    int soRcLeft = 0;
    int soRcRight = 0;
    int soRcTop = 0;
    int soRcBottom = 0;

    LoadValue("Settings\\SmartFileOpen", "LastSearchTerm", smartOpenSettings.lastSearchTerm);
    LoadValue("Settings\\SmartFileOpen", "DlgRect.Left", soRcLeft);
    LoadValue("Settings\\SmartFileOpen", "DlgRect.Top", soRcTop);
    LoadValue("Settings\\SmartFileOpen", "DlgRect.Right", soRcRight);
    LoadValue("Settings\\SmartFileOpen", "DlgRect.Bottom", soRcBottom);

    // check for bad values
    QRect screenRc = QGuiApplication::primaryScreen()->availableGeometry();

    if (screenRc.contains(QPoint(soRcLeft, soRcTop))
        && screenRc.contains(QPoint(soRcRight, soRcBottom)))
    {
        smartOpenSettings.rect.setLeft(soRcLeft);
        smartOpenSettings.rect.setTop(soRcTop);
        smartOpenSettings.rect.setRight(soRcRight);
        smartOpenSettings.rect.setBottom(soRcBottom);
    }

    //////////////////////////////////////////////////////////////////////////
    // Slice settings
    //////////////////////////////////////////////////////////////////////////
    LoadValue("Settings\\Slices", "DynamicByDefault", sliceSettings.dynamicByDefault);

    //////////////////////////////////////////////////////////////////////////
    // Load paths.
    //////////////////////////////////////////////////////////////////////////
    for (int id = 0; id < EDITOR_PATH_LAST; id++)
    {
        if (id == EDITOR_PATH_UI_ICONS) // Skip UI icons path, not load it.
        {
            continue;
        }
        int i = 0;
        searchPaths[id].clear();
        while (true)
        {
            const QString key = QStringLiteral("Path_%1_%2").arg(id, 2, 10, QLatin1Char('.')).arg(i, 2, 10, QLatin1Char('.'));
            QString path;
            LoadValue("Paths", key.toUtf8().data(), path);
            if (path.isEmpty())
            {
                break;
            }
            searchPaths[id].push_back(path);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
AZ_CVAR(bool, ed_previewGameInFullscreen_once, false, nullptr, AZ::ConsoleFunctorFlags::IsInvisible, "Preview the game (Ctrl+G, \"Play Game\", etc.) in fullscreen once");
AZ_CVAR(bool, ed_lowercasepaths, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Convert CCryFile paths to lowercase on Open");

void SEditorSettings::PostInitApply()
{
    if (!gEnv || !gEnv->pConsole)
    {
        return;
    }

    // Create CVars.
    REGISTER_CVAR2("ed_highlightGeometry", &viewports.bHighlightMouseOverGeometry, viewports.bHighlightMouseOverGeometry, 0, "Highlight geometry when mouse over it");
    REGISTER_CVAR2("ed_showFrozenHelpers", &viewports.nShowFrozenHelpers, viewports.nShowFrozenHelpers, 0, "Show helpers of frozen objects");
    gEnv->pConsole->RegisterInt("fe_fbx_savetempfile", 0, 0, "When importing an FBX file into Facial Editor, this will save out a conversion FSQ to the Animations/temp folder for trouble shooting");

    REGISTER_CVAR2_CB("ed_toolbarIconSize", &gui.nToolbarIconSize, gui.nToolbarIconSize, VF_NULL, "Override size of the toolbar icons 0-default, 16,32,...", ToolbarIconSizeChanged);

    GetIEditor()->SetEditorConfigSpec(editorConfigSpec, GetISystem()->GetConfigPlatform());
    REGISTER_CVAR2("ed_backgroundUpdatePeriod", &backgroundUpdatePeriod, backgroundUpdatePeriod, 0, "Delay between frame updates (ms) when window is out of focus but not minimized. 0 = disable background update");
    REGISTER_CVAR2("ed_showErrorDialogOnLoad", &showErrorDialogOnLoad, showErrorDialogOnLoad, 0, "Show error dialog on level load");
    REGISTER_CVAR2_CB("ed_keepEditorActive", &keepEditorActive, 0, VF_NULL, "Keep the editor active, even if no focus is set", KeepEditorActiveChanged);
    REGISTER_CVAR2("g_TemporaryLevelName", &g_TemporaryLevelName, "temp_level", VF_NULL, "Temporary level named used for experimental levels.");

    CCryEditApp::instance()->KeepEditorActive(keepEditorActive > 0);
}

//////////////////////////////////////////////////////////////////////////
// needs to be called after crysystem has been loaded
void SEditorSettings::LoadDefaultGamePaths()
{
    //////////////////////////////////////////////////////////////////////////
    // Default paths.
    //////////////////////////////////////////////////////////////////////////
    if (searchPaths[EDITOR_PATH_OBJECTS].empty())
    {
        searchPaths[EDITOR_PATH_OBJECTS].push_back((Path::GetEditingGameDataFolder() + "/Objects").c_str());
    }
    if (searchPaths[EDITOR_PATH_TEXTURES].empty())
    {
        searchPaths[EDITOR_PATH_TEXTURES].push_back((Path::GetEditingGameDataFolder() + "/Textures").c_str());
    }
    if (searchPaths[EDITOR_PATH_SOUNDS].empty())
    {
        searchPaths[EDITOR_PATH_SOUNDS].push_back((Path::GetEditingGameDataFolder() + "/Sounds").c_str());
    }
    if (searchPaths[EDITOR_PATH_MATERIALS].empty())
    {
        searchPaths[EDITOR_PATH_MATERIALS].push_back((Path::GetEditingGameDataFolder() + "/Materials").c_str());
    }

    auto iconsPath = AZ::IO::Path(AZ::Utils::GetEnginePath()) / "Assets";
    iconsPath /= "Editor/UI/Icons";
    iconsPath.MakePreferred();
    searchPaths[EDITOR_PATH_UI_ICONS].push_back(iconsPath.c_str());
}

//////////////////////////////////////////////////////////////////////////
bool SEditorSettings::BrowseTerrainTexture(bool bIsSave)
{
    QString path;

    if (!terrainTextureExport.isEmpty())
    {
        path = Path::GetPath(terrainTextureExport);
    }
    else
    {
        path = Path::GetEditingGameDataFolder().c_str();
    }

    if (bIsSave)
    {
        return CFileUtil::SelectSaveFile("Bitmap Image File (*.bmp)", "bmp", path, terrainTextureExport);
    }
    else
    {
        return CFileUtil::SelectFile("Bitmap Image File (*.bmp)", path, terrainTextureExport);
    }
}

void EnableSourceControl(bool enable)
{
    // Source control component
    using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
    SCRequestBus::Broadcast(&SCRequestBus::Events::EnableSourceControl, enable);
}

void SEditorSettings::SaveEnableSourceControlFlag(bool triggerUpdate /*= false*/)
{
    // Track the original source control value
    bool originalSourceControlFlag;
    LoadValue("Settings", "EnableSourceControl", originalSourceControlFlag);

    // Update only on change
    if (originalSourceControlFlag != enableSourceControl)
    {
        SaveValue("Settings", "EnableSourceControl", enableSourceControl);

        // If we are triggering any update for the source control flag, then set the control state
        if (triggerUpdate)
        {
            EnableSourceControl(enableSourceControl);
        }
    }
}

void SEditorSettings::LoadEnableSourceControlFlag()
{
    constexpr AZStd::string_view enableSourceControlKey = "/Amazon/Settings/EnableSourceControl";
    bool sourceControlEnabledInSettingsRegistry{};
    if (auto registry = AZ::SettingsRegistry::Get(); registry != nullptr &&
        registry->Get(sourceControlEnabledInSettingsRegistry, enableSourceControlKey))
    {
        // Have the SettingsRegistry able to disable the SourceControl Connection
        // only if the "EnableSourceControl" key is found
        if (!sourceControlEnabledInSettingsRegistry)
        {
            EnableSourceControl(false);
            return;
        }
    }
    // Use the QSettings "EnableSourceControl" value if the SettingsRegistry
    // hasn't disabled the SourceControlAPI
    LoadValue("Settings", "EnableSourceControl", enableSourceControl);
    EnableSourceControl(enableSourceControl);
}

AZStd::vector<AZStd::string> SEditorSettings::BuildSettingsList()
{
    if (GetIEditor()->GetSettingsManager())
    {
        // Will need to save the settings at least once to populate the list
        // This will not affect the level nor prompt dialogs
        Save();
        return GetIEditor()->GetSettingsManager()->BuildSettingsList();
    }

    return AZStd::vector<AZStd::string>();
}

void SEditorSettings::ConvertPath(const AZStd::string_view sourcePath, AZStd::string& category, AZStd::string& attribute)
{
    // This API accepts pipe-separated paths like "Category1|Category2|AttributeName"
    // But the SettingsManager requires 2 arguments, a Category like "Category1\Category2" and an attribute "AttributeName"
    // The reason for the difference is to have this API be consistent with the path syntax in Open 3D Engine Python APIs.

    // Find the last pipe separator ("|") in the path
    size_t lastSeparator = sourcePath.find_last_of("|");

    // Everything before the last separator is the category (since only the category is hierarchical)
    category = sourcePath.substr(0, lastSeparator);

    // Everything after the last separator is the attribute
    attribute = sourcePath.substr(lastSeparator + 1, sourcePath.length());

    // Replace pipes with backspaces in the category
    AZStd::replace(category.begin(), category.end(), '|', '\\');
}

AzToolsFramework::EditorSettingsAPIRequests::SettingOutcome SEditorSettings::GetValue(const AZStd::string_view path)
{
    if (path.find("|") == AZStd::string_view::npos)
    {
        return { AZStd::string("Invalid Path - could not find separator \"|\"") };
    }

    AZStd::string category, attribute;
    ConvertPath(path, category, attribute);

    QString result;
    LoadValue(category.c_str(), attribute.c_str(), result);

    AZStd::string actualResult = result.toUtf8().data();

    return { AZStd::any(actualResult) };
}

AzToolsFramework::EditorSettingsAPIRequests::SettingOutcome SEditorSettings::SetValue(const AZStd::string_view path, const AZStd::any& value)
{
    if (path.find("|") == AZStd::string_view::npos)
    {
        return { AZStd::string("Invalid Path - could not find separator \"|\"") };
    }

    AZStd::string category, attribute;
    ConvertPath(path, category, attribute);

    if (value.type() == azrtti_typeid<bool>())
    {
        bool val = AZStd::any_cast<bool>(value);
        SaveValue(category.c_str(), attribute.c_str(), val);
    }
    else if (value.type() == azrtti_typeid<double>())
    {
        SaveValue(category.c_str(), attribute.c_str(), aznumeric_cast<float>(AZStd::any_cast<double>(value)));
    }
    else if (value.type() == azrtti_typeid<AZ::s64>())
    {
        SaveValue(category.c_str(), attribute.c_str(), aznumeric_cast<int>(AZStd::any_cast<AZ::s64>(value)));
    }
    else if (value.type() == azrtti_typeid<AZStd::string>())
    {
        SaveValue(category.c_str(), attribute.c_str(), QString(AZStd::any_cast<AZStd::string>(value).c_str()));
    }
    else if (value.type() == azrtti_typeid<AZStd::string_view>())
    {
        SaveValue(category.c_str(), attribute.c_str(), QString(AZStd::any_cast<AZStd::string_view>(value).data()));
    }
    else
    {
        return { AZStd::string("Invalid Value Type - supported types: string, bool, int, float") };
    }

    // Reload the changes in the Settings object used in the Editor
    Load();

    return { value };
}

void SEditorSettings::SaveSettingsRegistryFile()
{
    auto registry = AZ::SettingsRegistry::Get();
    if (registry == nullptr)
    {
        AZ_Warning("SEditorSettings", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
        return;
    }

    // Resolve path to editorpreferences.setreg
    AZ::IO::FixedMaxPath editorPreferencesFilePath = AZ::Utils::GetProjectPath();
    editorPreferencesFilePath /= "user/Registry/editorpreferences.setreg";

    AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
    dumperSettings.m_prettifyOutput = true;
    dumperSettings.m_includeFilter = [](AZStd::string_view path)
    {
        AZStd::string_view amazonPrefixPath("/Amazon/Preferences");
        AZStd::string_view o3dePrefixPath("/O3DE/Preferences");
        return amazonPrefixPath.starts_with(path.substr(0, amazonPrefixPath.size())) ||
            o3dePrefixPath.starts_with(path.substr(0, o3dePrefixPath.size()));
    };

    AZStd::string stringBuffer;
    AZ::IO::ByteContainerStream stringStream(&stringBuffer);
    if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*registry, "", stringStream, dumperSettings))
    {
        AZ_Warning("SEditorSettings", false, R"(Unable to save changes to the Editor Preferences registry file at "%s"\n)",
            editorPreferencesFilePath.c_str());
        return;
    }

    bool saved{};
    constexpr auto configurationMode = AZ::IO::SystemFile::SF_OPEN_CREATE
        | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH
        | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
    if (AZ::IO::SystemFile outputFile; outputFile.Open(editorPreferencesFilePath.c_str(), configurationMode))
    {
        saved = outputFile.Write(stringBuffer.data(), stringBuffer.size()) == stringBuffer.size();
    }

    AZ_Warning("SEditorSettings", saved, R"(Unable to save Editor Preferences registry file to path "%s"\n)",
        editorPreferencesFilePath.c_str());
}

bool SEditorSettings::SetSettingsRegistry_Bool(const char* key, bool value)
{
    if (auto registry = AZ::SettingsRegistry::Get(); registry != nullptr)
    {
        return registry->Set(key, value);
    }

    return false;
}

bool SEditorSettings::GetSettingsRegistry_Bool(const char* key, bool& value)
{
    if (auto registry = AZ::SettingsRegistry::Get(); registry != nullptr)
    {
        return registry->Get(value, key);
    }

    return false;
}

AzToolsFramework::ConsoleColorTheme SEditorSettings::GetConsoleColorTheme() const
{
    return consoleBackgroundColorTheme;
}

AZ::u64 SEditorSettings::GetMaxNumberOfItemsShownInSearchView() const
{
    return SandboxEditor::MaxItemsShownInAssetBrowserSearch();
}
