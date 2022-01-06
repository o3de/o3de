/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector3.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>
#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <QIcon>

#include "Settings.h"

class CEditorPreferencesPage_General
    : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_General, "{9CFBBE85-560D-4720-A830-50EF25D06ED5}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_General();
    virtual ~CEditorPreferencesPage_General() = default;

    virtual const char* GetCategory() override { return "General Settings"; }
    virtual const char* GetTitle() override;
    virtual QIcon& GetIcon() override;
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }

private:
    void InitializeSettings();

    struct GeneralSettings
    {
        AZ_TYPE_INFO(GeneralSettings, "{C2AE8F6D-7AA6-499E-A3E8-ECCD0AC6F3D2}")

        bool m_previewPanel;
        bool m_applyConfigSpec;
        bool m_enableSourceControl;
        bool m_clearConsoleOnGameModeStart;
        AzToolsFramework::ConsoleColorTheme m_consoleBackgroundColorTheme;
        bool m_autoLoadLastLevel;
        bool m_bShowTimeInConsole;
        AzQtComponents::ToolBar::ToolBarIconSize m_toolbarIconSize;
        bool m_stylusMode;
        bool m_restoreViewportCamera;
        bool m_bShowNews;
        bool m_enableSceneInspector;
    };

    struct LevelSaveSettings
    {
        AZ_TYPE_INFO(LevelSaveSettings, "{E297DAE3-3985-4BC2-8B43-45F3B1522F6B}");
        AzToolsFramework::Prefab::SaveAllPrefabsPreference m_saveAllPrefabsPreference;
    };

    struct Messaging
    {
        AZ_TYPE_INFO(Messaging, "{A6AD87CB-E905-409B-A2BF-C43CDCE63B0C}")

        bool m_showDashboard;
        bool m_showCircularDependencyError;
    };

    struct Undo
    {
        AZ_TYPE_INFO(Undo, "{A3AC0728-F132-4BF2-B122-8A631B636E81}")

        int m_undoLevels;
        bool m_undoSliceOverrideSaveValue;
    };

    struct DeepSelection
    {
        AZ_TYPE_INFO(DeepSelection, "{225616BF-66DE-41EC-9FDD-F5A104112547}")

        float m_deepSelectionRange;
        bool m_stickDuplicate;
    };

    struct SliceSettings
    {
        AZ_TYPE_INFO(SliceSettings, "{8505CCC1-874C-4389-B51A-B9E5FF70CFDA}")

        bool m_slicesDynamicByDefault;
    };

    GeneralSettings m_generalSettings;
    LevelSaveSettings m_levelSaveSettings;
    Messaging m_messaging;
    Undo m_undo;
    DeepSelection m_deepSelection;
    SliceSettings m_sliceSettings;
    QIcon m_icon;
};

static const char* EditorPreferencesGeneralRestoreViewportCameraSettingName = "Restore Viewport Camera on Game Mode Exit";

