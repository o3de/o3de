/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMStudioManager.h"
#include "LoadActorSettingsWindow.h"
#include <MCore/Source/StringConversions.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>


namespace EMStudio
{
    // constructor
    LoadActorSettingsWindow::LoadActorSettingsWindow(QWidget* parent, const AZStd::string& filePath)
        : QDialog(parent)
    {
        // set the window title
        setWindowTitle("Load Actor Settings");

        AZStd::string filename;
        AzFramework::StringFunc::Path::GetFileName(filePath.c_str(), filename);

        // create the top layout
        QLabel* topLabel = new QLabel(QString("File: <b>%1</b>").arg(filename.c_str()));
        topLabel->setStyleSheet("background-color: rgb(40, 40, 40); padding: 6px;");
        topLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QVBoxLayout* topLayout = new QVBoxLayout();
        topLayout->addWidget(topLabel);
        topLayout->setMargin(0);

        // open the load actor settings file
        
        const QSettings loadActorSettings(GetConfigFilename(), QSettings::IniFormat, this);

        // create the load meshes checkbox
        m_loadMeshesCheckbox = new QCheckBox("Load Meshes");
        const bool loadMeshesValue = loadActorSettings.value("LoadMeshes", true).toBool();
        m_loadMeshesCheckbox->setChecked(loadMeshesValue);

        // connect the load meshes checkbox to enable/disable all related to mesh
        connect(m_loadMeshesCheckbox, &QCheckBox::clicked, this, &LoadActorSettingsWindow::LoadMeshesClicked);

        // create the load collision meshes checkbox
        m_loadCollisionMeshesCheckbox = new QCheckBox("Load Collision Meshes");
        const bool loadCollisionMeshesValue = loadActorSettings.value("LoadCollisionMeshes", true).toBool();
        m_loadCollisionMeshesCheckbox->setChecked(loadCollisionMeshesValue);

        // create the load standard material layers checkbox
        m_loadStandardMaterialLayersCheckbox = new QCheckBox("Load Standard Material Layers");
        const bool loadStandardMaterialLayersValue = loadActorSettings.value("LoadStandardMaterialLayers", true).toBool();
        m_loadStandardMaterialLayersCheckbox->setChecked(loadStandardMaterialLayersValue);

        // create the load skinning info checkbox
        m_loadSkinningInfoCheckbox = new QCheckBox("Load Skinning Info");
        const bool loadSkinningInfoValue = loadActorSettings.value("LoadSkinningInfo", true).toBool();
        m_loadSkinningInfoCheckbox->setChecked(loadSkinningInfoValue);

        // connect the load meshes checkbox to enable/disable all related to mesh
        connect(m_loadSkinningInfoCheckbox, &QCheckBox::clicked, this, &LoadActorSettingsWindow::LoadSkinningInfoClicked);

        // create the load limits checkbox
        m_loadLimitsCheckbox = new QCheckBox("Load Limits");
        const bool loadLimitsValue = loadActorSettings.value("LoadLimits", true).toBool();
        m_loadLimitsCheckbox->setChecked(loadLimitsValue);

        // create the load geometry LODs checkbox
        m_loadGeometryLoDsCheckbox = new QCheckBox("Load Geometry LODs");
        const bool loadGeometryLODsValue = loadActorSettings.value("LoadGeometryLODs", true).toBool();
        m_loadGeometryLoDsCheckbox->setChecked(loadGeometryLODsValue);

        // create the load skeletal LODs checkbox
        m_loadSkeletalLoDsCheckbox = new QCheckBox("Load Skeletal LODs");
        const bool loadSkeletalLODsValue = loadActorSettings.value("LoadSkeletalLODs", true).toBool();
        m_loadSkeletalLoDsCheckbox->setChecked(loadSkeletalLODsValue);

        // create the load tangents checkbox
        m_loadTangentsCheckbox = new QCheckBox("Load Tangents");
        const bool loadTangentsValue = loadActorSettings.value("LoadTangents", true).toBool();
        m_loadTangentsCheckbox->setChecked(loadTangentsValue);

        // create the auto generate tangents checkbox
        m_autoGenerateTangentsCheckbox = new QCheckBox("Auto Generate Tangents");
        const bool autoGenerateTangentsValue = loadActorSettings.value("AutoGenerateTangents", true).toBool();
        m_autoGenerateTangentsCheckbox->setChecked(autoGenerateTangentsValue);

        // create the load morph targets checkbox
        m_loadMorphTargetsCheckbox = new QCheckBox("Load Morph Targets");
        const bool loadMorphTargetsValue = loadActorSettings.value("LoadMorphTargets", true).toBool();
        m_loadMorphTargetsCheckbox->setChecked(loadMorphTargetsValue);

        // create the dual quaternion skinning checkbox
        m_dualQuaternionSkinningCheckbox = new QCheckBox("Dual Quaternion Skinning");
        const bool dualQuaternionSkinningValue = loadActorSettings.value("DualQuaternionSkinning", false).toBool();
        m_dualQuaternionSkinningCheckbox->setChecked(dualQuaternionSkinningValue);

        // disable the controls if load meshes is not enabled
        if (loadMeshesValue == false)
        {
            m_loadStandardMaterialLayersCheckbox->setDisabled(true);
            m_loadSkinningInfoCheckbox->setDisabled(true);
            m_loadGeometryLoDsCheckbox->setDisabled(true);
            m_loadTangentsCheckbox->setDisabled(true);
            m_autoGenerateTangentsCheckbox->setDisabled(true);
            m_dualQuaternionSkinningCheckbox->setDisabled(true);
        }
        else
        {
            // disable dual quaternion skinning control if the load dual skinning info is not enabled
            if (loadSkinningInfoValue == false)
            {
                m_dualQuaternionSkinningCheckbox->setDisabled(true);
            }
        }

        // create the left part settings layout
        QVBoxLayout* leftPartSettingsLayout = new QVBoxLayout();
        leftPartSettingsLayout->addWidget(m_loadMeshesCheckbox);
        leftPartSettingsLayout->addWidget(m_loadCollisionMeshesCheckbox);
        leftPartSettingsLayout->addWidget(m_loadStandardMaterialLayersCheckbox);
        leftPartSettingsLayout->addWidget(m_loadSkinningInfoCheckbox);
        leftPartSettingsLayout->addWidget(m_loadLimitsCheckbox);

        // create the right part settings layout
        QVBoxLayout* rightPartSettingsLayout = new QVBoxLayout();
        rightPartSettingsLayout->addWidget(m_loadGeometryLoDsCheckbox);
        rightPartSettingsLayout->addWidget(m_loadSkeletalLoDsCheckbox);
        rightPartSettingsLayout->addWidget(m_loadTangentsCheckbox);
        rightPartSettingsLayout->addWidget(m_autoGenerateTangentsCheckbox);
        rightPartSettingsLayout->addWidget(m_loadMorphTargetsCheckbox);
        rightPartSettingsLayout->addWidget(m_dualQuaternionSkinningCheckbox);

        // create the settings layout
        QHBoxLayout* settingsLayout = new QHBoxLayout();
        settingsLayout->addLayout(leftPartSettingsLayout);
        settingsLayout->addLayout(rightPartSettingsLayout);
        settingsLayout->setAlignment(Qt::AlignCenter);

        // create the settings layout widget
        QWidget* settingsLayoutWidget = new QWidget();
        settingsLayoutWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        settingsLayoutWidget->setLayout(settingsLayout);

        // create the button layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("OK");
        QPushButton* cancelButton = new QPushButton("Cancel");
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);
        buttonLayout->setAlignment(Qt::AlignBottom);
        buttonLayout->setContentsMargins(6, 0, 6, 6);

        // connect the buttons
        connect(okButton, &QPushButton::clicked, this, &LoadActorSettingsWindow::accept);
        connect(cancelButton, &QPushButton::clicked, this, &LoadActorSettingsWindow::reject);

        // create the button layout widget
        QWidget* buttonLayoutWidget = new QWidget();
        buttonLayoutWidget->setLayout(buttonLayout);

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();
        layout->addLayout(topLayout);
        layout->addWidget(settingsLayoutWidget);
        layout->addWidget(buttonLayoutWidget);
        layout->setSpacing(0);
        layout->setMargin(0);

        // set the layout
        setLayout(layout);

        // connect the dialog action
        connect(this, &LoadActorSettingsWindow::accepted, this, &LoadActorSettingsWindow::Accepted);
    }


    // destructor
    LoadActorSettingsWindow::~LoadActorSettingsWindow()
    {
    }


    LoadActorSettingsWindow::LoadActorSettings LoadActorSettingsWindow::GetLoadActorSettings() const
    {
        LoadActorSettings loadActorSettings;
        loadActorSettings.m_loadMeshes               = m_loadMeshesCheckbox->isChecked();
        loadActorSettings.m_loadCollisionMeshes      = m_loadCollisionMeshesCheckbox->isChecked();
        loadActorSettings.m_loadStandardMaterialLayers = m_loadStandardMaterialLayersCheckbox->isChecked();
        loadActorSettings.m_loadSkinningInfo         = m_loadSkinningInfoCheckbox->isChecked();
        loadActorSettings.m_loadLimits               = m_loadLimitsCheckbox->isChecked();
        loadActorSettings.m_loadGeometryLoDs         = m_loadGeometryLoDsCheckbox->isChecked();
        loadActorSettings.m_loadSkeletalLoDs         = m_loadSkeletalLoDsCheckbox->isChecked();
        loadActorSettings.m_loadTangents             = m_loadTangentsCheckbox->isChecked();
        loadActorSettings.m_autoGenerateTangents     = m_autoGenerateTangentsCheckbox->isChecked();
        loadActorSettings.m_loadMorphTargets         = m_loadMorphTargetsCheckbox->isChecked();
        loadActorSettings.m_dualQuaternionSkinning   = m_dualQuaternionSkinningCheckbox->isChecked();
        return loadActorSettings;
    }


    void LoadActorSettingsWindow::Accepted()
    {
        // open the load actor settings file
        QSettings loadActorSettings(GetConfigFilename(), QSettings::IniFormat, this);

        // set all values
        loadActorSettings.setValue("LoadMeshes", m_loadMeshesCheckbox->isChecked());
        loadActorSettings.setValue("LoadCollisionMeshes", m_loadCollisionMeshesCheckbox->isChecked());
        loadActorSettings.setValue("LoadStandardMaterialLayers", m_loadStandardMaterialLayersCheckbox->isChecked());
        loadActorSettings.setValue("LoadSkinningInfo", m_loadSkinningInfoCheckbox->isChecked());
        loadActorSettings.setValue("LoadLimits", m_loadLimitsCheckbox->isChecked());
        loadActorSettings.setValue("LoadGeometryLODs", m_loadGeometryLoDsCheckbox->isChecked());
        loadActorSettings.setValue("LoadSkeletalLODs", m_loadSkeletalLoDsCheckbox->isChecked());
        loadActorSettings.setValue("LoadTangents", m_loadTangentsCheckbox->isChecked());
        loadActorSettings.setValue("AutoGenerateTangents", m_autoGenerateTangentsCheckbox->isChecked());
        loadActorSettings.setValue("LoadMorphTargets", m_loadMorphTargetsCheckbox->isChecked());
        loadActorSettings.setValue("DualQuaternionSkinning", m_dualQuaternionSkinningCheckbox->isChecked());
    }


    void LoadActorSettingsWindow::LoadMeshesClicked(bool checked)
    {
        // enable or disable controls
        m_loadStandardMaterialLayersCheckbox->setEnabled(checked);
        m_loadSkinningInfoCheckbox->setEnabled(checked);
        m_loadGeometryLoDsCheckbox->setEnabled(checked);
        m_loadTangentsCheckbox->setEnabled(checked);
        m_autoGenerateTangentsCheckbox->setEnabled(checked);

        // the dual quaternion skinning control is enabled based on the laod skinning info control
        // when the the load meshes is not enabled, the control is disabled
        if (checked)
        {
            m_dualQuaternionSkinningCheckbox->setEnabled(m_loadSkinningInfoCheckbox->isChecked());
        }
        else
        {
            m_dualQuaternionSkinningCheckbox->setDisabled(true);
        }
    }


    void LoadActorSettingsWindow::LoadSkinningInfoClicked(bool checked)
    {
        m_dualQuaternionSkinningCheckbox->setEnabled(checked);
    }


    QString LoadActorSettingsWindow::GetConfigFilename() const
    {
        QString result = GetManager()->GetAppDataFolder().c_str();
        result += "EMStudioLoadActorSettings.cfg";
        return result;
    }


    LoadActorSettingsWindow::LoadActorSettings::LoadActorSettings() :
        m_loadMeshes(true),
        m_loadCollisionMeshes(true),
        m_loadStandardMaterialLayers(true),
        m_loadSkinningInfo(true),
        m_loadLimits(true),
        m_loadGeometryLoDs(true),
        m_loadSkeletalLoDs(true),
        m_loadTangents(true),
        m_autoGenerateTangents(true),
        m_loadMorphTargets(true),
        m_dualQuaternionSkinning(false)
    {
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_LoadActorSettingsWindow.cpp>
