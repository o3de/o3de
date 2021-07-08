/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QTreeView>
#include <QStandardItemModel>
#include <QHeaderView>

#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/any.h>

#include <ISystem.h>
#endif

class QGridLayout;
class QLabel;

namespace Ui
{
    class GraphicsSettingsDialog;
}

// Description:
//   Status of cvar for a specifc platform and spec level
//   editedValue - current setting within Graphics Settings Dialog box
//   overwrittenValue - original setting from platform config file (set to originalValue if not found)
//   originalValue - original settings from sys_spec config file index

struct CVarFileStatus
{
    AZStd::any editedValue;
    AZStd::any overwrittenValue;
    AZStd::any originalValue;
    CVarFileStatus(AZStd::any edit, AZStd::any over, AZStd::any orig) : editedValue(edit), overwrittenValue(over), originalValue(orig) {}
};

// Description:
//   Status of specific cvar for Editor mapping
//   type - CVAR_INT / CVAR_FLOAT / CVAR_STRING
//   cvarGroup - source of cvar (sys_spec_particles, sys_spec_physics, etc.) or "miscellaneous" if only specified in platform config file
//   fileVals = CVarFileStatus for each spec level of a specific platform

struct CVarInfo
{
    int type;
    AZStd::string cvarGroup;
    AZStd::vector<CVarFileStatus> fileVals;
};

enum class GraphicsSettings
{
    GameEffects,
    Light,
    ObjectDetail,
    Particles,
    Physics,
    PostProcessing,
    Quality,
    Shading,
    Shadows,
    Sound,
    Texture,
    TextureResolution,
    VolumetricEffects,
    Water,
    Miscellaneous,
    numSettings
};

class GraphicsSettingsHeaderView;

class GraphicsSettingsTreeView
    : public QTreeView
{
    Q_OBJECT

public:
    GraphicsSettingsTreeView(QWidget* parent = nullptr);

};

class GraphicsSettingsModel
    : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit GraphicsSettingsModel(QObject* parent = 0);

    Qt::ItemFlags flags(const QModelIndex& index) const override;
};

class GraphicsSettingsDialog
    : public QDialog,
      public ILoadConfigurationEntrySink
{
    Q_OBJECT

public:

    explicit GraphicsSettingsDialog(QWidget* parent = nullptr);
    virtual ~GraphicsSettingsDialog();

    bool IsCustom(void) { return m_showCustomSpec; }
    void UnloadCustomSpec(int specLevel);

    // ILoadConfigurationEntrySink
    void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup) override;

public slots:
    //Accept and reject
    void reject() override;
    void accept() override;

private slots:
    //Update UIs
    void PlatformChanged(const QString& platform);
    bool CVarChanged(AZStd::any val, const char* cvarName, int specLevel);
    void CVarChanged(int i);
    void CVarChanged(double d);
    void CVarChanged(const QString& s);

private:

    // The struct ParameterWidget is used to store the parameter widget
    // m_parameterName will be the name of the parameter the widget represent.
    struct ParameterWidget
    {
        ParameterWidget(QWidget* widget, QString parameterName = "");

        QString GetToolTip() const;

        const char* PARAMETER_TOOLTIP = "The variable will update render parameter \"%1\".";
        QWidget* m_widget;
        QString m_parameterName;
    };

    struct CollapseGroup
    {
        QString m_groupName;
        QStandardItem* m_groupRow;
        QTreeView* m_treeView;
        bool m_isCollapsed;

        CollapseGroup(QTreeView* treeView);

        void ToggleCollapsed();
    };

    void OpenCustomSpecDialog();
    void ApplyCustomSpec(const QString& customFilePath);
    bool IsCustomSpecAlreadyLoaded(const AZStd::string& filename) const;
    void SetSettingsTree(int numColumns);
    void SetCollapsed(const QModelIndex& index, bool flag);

    enum CVarStateComparison
    {
        EDITED_OVERWRITTEN_COMPARE = 1,
        EDITED_ORIGINAL_COMPARE = 2,
        OVERWRITTEN_ORIGINAL_COMPARE = 3,

        END_CVARSTATE_COMPARE,
    };

    bool CheckCVarStatesForDiff(AZStd::pair<AZStd::string, CVarInfo>* it, int cfgFileIndex, CVarStateComparison cmp);

    // Save out settings into project-level cfg files
    void SaveSystemSettings();

    // Load in project-level cfg files for current platform
    void LoadPlatformConfigurations();
    // Build UI column for spec level of current platform
    void BuildColumn(int specLevel);
    // Initial UI building
    void BuildUI();
    // Cleaning out UI before loading new platform information
    void CleanUI();
    // Shows/hides custom spec option
    void ShowCustomSpecOption(bool show);
    // Shows/hides category labels and dropdowns
    void ShowCategories(bool show);
    // Warns about unsaved changes (returns true if accepted)
    bool SendUnsavedChangesWarning(bool cancel);

    void LoadCVarGroupDirectory(const AZStd::string& path);

    /////////////////////////////////////////////
    // UI help functions

    // Setup collapsed buttons
    void SetCollapsedLayout(const QString& groupName, QStandardItem* groupItem);
    // Sets the platform entry index for the given platform
    void SetPlatformEntry(ESystemConfigPlatform platform);
    // Gets the platform enum given the platform name
    ESystemConfigPlatform GetConfigPlatformFromName(const AZStd::string& platformName);

    ////////////////////////////////////////////
    // Members

    // Qt values
    const int INPUT_MIN_WIDTH = 100;
    const int INPUT_MIN_HEIGHT = 20;
    const int INPUT_ROW_SPAN = 1;
    const int INPUT_COLUMN_SPAN = 1;
    const int CVAR_ROW_OFFSET = 2;
    const int CVAR_VALUE_COLUMN_OFFSET = 1;
    const int PLATFORM_LABEL_ROW = 1;
    const int CVAR_LABEL_COLUMN = 1;

    // Tool tips
    const QString SETTINGS_FILE_PATH = "Config/spec/";
    const char* CFG_FILEFILTER = "Cfg File(*.cfg);;All files(*)";

    const int m_numSpecLevels = 4;

    bool m_showCustomSpec;
    bool m_showCategories;
    GraphicsSettingsModel* m_graphicsSettingsModel;
    GraphicsSettingsHeaderView* m_headerView;
    int m_numColumns{ 0 };

    const char* m_cvarGroupsFolder = "Config/CVarGroups";

    QScopedPointer<Ui::GraphicsSettingsDialog> m_ui;

    QVector<CollapseGroup*> m_uiCollapseGroup;

    QVector<ParameterWidget*> m_parameterWidgets;

    AZStd::string m_currentConfigFilename;
    size_t m_currentSpecIndex;

    // cvar name --> pair(type, CVarStatus for each file)
    AZStd::unordered_map<AZStd::string, CVarInfo> m_cVarTracker;

    AZStd::unordered_map<ESystemConfigPlatform, AZStd::vector<AZStd::string> > m_cfgFiles;

    AZStd::vector<AZStd::pair<AZStd::string, ESystemConfigPlatform> > m_platformStrings;

    struct CVarGroupInfo
    {
        QVector<QLabel*> m_platformLabels;
        QVector<QLabel*> m_cvarLabels;
        QVector<AzQtComponents::SpinBox*> m_cvarSpinBoxes;
        QVector<AzQtComponents::DoubleSpinBox*> m_cvarDoubleSpinBoxes;
        QVector<QLineEdit*> m_cvarLineEdits;
        QVector<QToolButton*> m_specFileArea;
        QVector<QWidget*> m_widgetInsertOrder;
        QStandardItem* m_treeRowItem;
        int m_currentRow;
    };

    AZStd::unordered_map<AZStd::string, CVarGroupInfo> m_cvarGroupData;
    AZStd::vector<AZStd::string> m_cvarGroupOrder;

    ESystemConfigPlatform m_currentPlatform;

    int m_dirtyCVarCount;
};

class GraphicsSettingsHeaderView
    : public QHeaderView
{
        Q_OBJECT
public:
    GraphicsSettingsHeaderView(GraphicsSettingsDialog* dialog, Qt::Orientation orientation, QWidget* parent = nullptr);

private:
    bool event(QEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    GraphicsSettingsDialog* m_dialog;
    int m_index{ -1 };
};


