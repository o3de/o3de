/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mLoadMeshesCheckbox = new QCheckBox("Load Meshes");
        const bool loadMeshesValue = loadActorSettings.value("LoadMeshes", true).toBool();
        mLoadMeshesCheckbox->setChecked(loadMeshesValue);

        // connect the load meshes checkbox to enable/disable all related to mesh
        connect(mLoadMeshesCheckbox, &QCheckBox::clicked, this, &LoadActorSettingsWindow::LoadMeshesClicked);

        // create the load collision meshes checkbox
        mLoadCollisionMeshesCheckbox = new QCheckBox("Load Collision Meshes");
        const bool loadCollisionMeshesValue = loadActorSettings.value("LoadCollisionMeshes", true).toBool();
        mLoadCollisionMeshesCheckbox->setChecked(loadCollisionMeshesValue);

        // create the load standard material layers checkbox
        mLoadStandardMaterialLayersCheckbox = new QCheckBox("Load Standard Material Layers");
        const bool loadStandardMaterialLayersValue = loadActorSettings.value("LoadStandardMaterialLayers", true).toBool();
        mLoadStandardMaterialLayersCheckbox->setChecked(loadStandardMaterialLayersValue);

        // create the load skinning info checkbox
        mLoadSkinningInfoCheckbox = new QCheckBox("Load Skinning Info");
        const bool loadSkinningInfoValue = loadActorSettings.value("LoadSkinningInfo", true).toBool();
        mLoadSkinningInfoCheckbox->setChecked(loadSkinningInfoValue);

        // connect the load meshes checkbox to enable/disable all related to mesh
        connect(mLoadSkinningInfoCheckbox, &QCheckBox::clicked, this, &LoadActorSettingsWindow::LoadSkinningInfoClicked);

        // create the load limits checkbox
        mLoadLimitsCheckbox = new QCheckBox("Load Limits");
        const bool loadLimitsValue = loadActorSettings.value("LoadLimits", true).toBool();
        mLoadLimitsCheckbox->setChecked(loadLimitsValue);

        // create the load geometry LODs checkbox
        mLoadGeometryLODsCheckbox = new QCheckBox("Load Geometry LODs");
        const bool loadGeometryLODsValue = loadActorSettings.value("LoadGeometryLODs", true).toBool();
        mLoadGeometryLODsCheckbox->setChecked(loadGeometryLODsValue);

        // create the load skeletal LODs checkbox
        mLoadSkeletalLODsCheckbox = new QCheckBox("Load Skeletal LODs");
        const bool loadSkeletalLODsValue = loadActorSettings.value("LoadSkeletalLODs", true).toBool();
        mLoadSkeletalLODsCheckbox->setChecked(loadSkeletalLODsValue);

        // create the load tangents checkbox
        mLoadTangentsCheckbox = new QCheckBox("Load Tangents");
        const bool loadTangentsValue = loadActorSettings.value("LoadTangents", true).toBool();
        mLoadTangentsCheckbox->setChecked(loadTangentsValue);

        // create the auto generate tangents checkbox
        mAutoGenerateTangentsCheckbox = new QCheckBox("Auto Generate Tangents");
        const bool autoGenerateTangentsValue = loadActorSettings.value("AutoGenerateTangents", true).toBool();
        mAutoGenerateTangentsCheckbox->setChecked(autoGenerateTangentsValue);

        // create the load morph targets checkbox
        mLoadMorphTargetsCheckbox = new QCheckBox("Load Morph Targets");
        const bool loadMorphTargetsValue = loadActorSettings.value("LoadMorphTargets", true).toBool();
        mLoadMorphTargetsCheckbox->setChecked(loadMorphTargetsValue);

        // create the dual quaternion skinning checkbox
        mDualQuaternionSkinningCheckbox = new QCheckBox("Dual Quaternion Skinning");
        const bool dualQuaternionSkinningValue = loadActorSettings.value("DualQuaternionSkinning", false).toBool();
        mDualQuaternionSkinningCheckbox->setChecked(dualQuaternionSkinningValue);

        // disable the controls if load meshes is not enabled
        if (loadMeshesValue == false)
        {
            mLoadStandardMaterialLayersCheckbox->setDisabled(true);
            mLoadSkinningInfoCheckbox->setDisabled(true);
            mLoadGeometryLODsCheckbox->setDisabled(true);
            mLoadTangentsCheckbox->setDisabled(true);
            mAutoGenerateTangentsCheckbox->setDisabled(true);
            mDualQuaternionSkinningCheckbox->setDisabled(true);
        }
        else
        {
            // disable dual quaternion skinning control if the load dual skinning info is not enabled
            if (loadSkinningInfoValue == false)
            {
                mDualQuaternionSkinningCheckbox->setDisabled(true);
            }
        }

        // create the left part settings layout
        QVBoxLayout* leftPartSettingsLayout = new QVBoxLayout();
        leftPartSettingsLayout->addWidget(mLoadMeshesCheckbox);
        leftPartSettingsLayout->addWidget(mLoadCollisionMeshesCheckbox);
        leftPartSettingsLayout->addWidget(mLoadStandardMaterialLayersCheckbox);
        leftPartSettingsLayout->addWidget(mLoadSkinningInfoCheckbox);
        leftPartSettingsLayout->addWidget(mLoadLimitsCheckbox);

        // create the right part settings layout
        QVBoxLayout* rightPartSettingsLayout = new QVBoxLayout();
        rightPartSettingsLayout->addWidget(mLoadGeometryLODsCheckbox);
        rightPartSettingsLayout->addWidget(mLoadSkeletalLODsCheckbox);
        rightPartSettingsLayout->addWidget(mLoadTangentsCheckbox);
        rightPartSettingsLayout->addWidget(mAutoGenerateTangentsCheckbox);
        rightPartSettingsLayout->addWidget(mLoadMorphTargetsCheckbox);
        rightPartSettingsLayout->addWidget(mDualQuaternionSkinningCheckbox);

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
        loadActorSettings.mLoadMeshes               = mLoadMeshesCheckbox->isChecked();
        loadActorSettings.mLoadCollisionMeshes      = mLoadCollisionMeshesCheckbox->isChecked();
        loadActorSettings.mLoadStandardMaterialLayers = mLoadStandardMaterialLayersCheckbox->isChecked();
        loadActorSettings.mLoadSkinningInfo         = mLoadSkinningInfoCheckbox->isChecked();
        loadActorSettings.mLoadLimits               = mLoadLimitsCheckbox->isChecked();
        loadActorSettings.mLoadGeometryLODs         = mLoadGeometryLODsCheckbox->isChecked();
        loadActorSettings.mLoadSkeletalLODs         = mLoadSkeletalLODsCheckbox->isChecked();
        loadActorSettings.mLoadTangents             = mLoadTangentsCheckbox->isChecked();
        loadActorSettings.mAutoGenerateTangents     = mAutoGenerateTangentsCheckbox->isChecked();
        loadActorSettings.mLoadMorphTargets         = mLoadMorphTargetsCheckbox->isChecked();
        loadActorSettings.mDualQuaternionSkinning   = mDualQuaternionSkinningCheckbox->isChecked();
        return loadActorSettings;
    }


    void LoadActorSettingsWindow::Accepted()
    {
        // open the load actor settings file
        QSettings loadActorSettings(GetConfigFilename(), QSettings::IniFormat, this);

        // set all values
        loadActorSettings.setValue("LoadMeshes", mLoadMeshesCheckbox->isChecked());
        loadActorSettings.setValue("LoadCollisionMeshes", mLoadCollisionMeshesCheckbox->isChecked());
        loadActorSettings.setValue("LoadStandardMaterialLayers", mLoadStandardMaterialLayersCheckbox->isChecked());
        loadActorSettings.setValue("LoadSkinningInfo", mLoadSkinningInfoCheckbox->isChecked());
        loadActorSettings.setValue("LoadLimits", mLoadLimitsCheckbox->isChecked());
        loadActorSettings.setValue("LoadGeometryLODs", mLoadGeometryLODsCheckbox->isChecked());
        loadActorSettings.setValue("LoadSkeletalLODs", mLoadSkeletalLODsCheckbox->isChecked());
        loadActorSettings.setValue("LoadTangents", mLoadTangentsCheckbox->isChecked());
        loadActorSettings.setValue("AutoGenerateTangents", mAutoGenerateTangentsCheckbox->isChecked());
        loadActorSettings.setValue("LoadMorphTargets", mLoadMorphTargetsCheckbox->isChecked());
        loadActorSettings.setValue("DualQuaternionSkinning", mDualQuaternionSkinningCheckbox->isChecked());
    }


    void LoadActorSettingsWindow::LoadMeshesClicked(bool checked)
    {
        // enable or disable controls
        mLoadStandardMaterialLayersCheckbox->setEnabled(checked);
        mLoadSkinningInfoCheckbox->setEnabled(checked);
        mLoadGeometryLODsCheckbox->setEnabled(checked);
        mLoadTangentsCheckbox->setEnabled(checked);
        mAutoGenerateTangentsCheckbox->setEnabled(checked);

        // the dual quaternion skinning control is enabled based on the laod skinning info control
        // when the the load meshes is not enabled, the control is disabled
        if (checked)
        {
            mDualQuaternionSkinningCheckbox->setEnabled(mLoadSkinningInfoCheckbox->isChecked());
        }
        else
        {
            mDualQuaternionSkinningCheckbox->setDisabled(true);
        }
    }


    void LoadActorSettingsWindow::LoadSkinningInfoClicked(bool checked)
    {
        mDualQuaternionSkinningCheckbox->setEnabled(checked);
    }


    QString LoadActorSettingsWindow::GetConfigFilename() const
    {
        QString result = GetManager()->GetAppDataFolder().c_str();
        result += "EMStudioLoadActorSettings.cfg";
        return result;
    }


    LoadActorSettingsWindow::LoadActorSettings::LoadActorSettings() :
        mLoadMeshes(true),
        mLoadCollisionMeshes(true),
        mLoadStandardMaterialLayers(true),
        mLoadSkinningInfo(true),
        mLoadLimits(true),
        mLoadGeometryLODs(true),
        mLoadSkeletalLODs(true),
        mLoadTangents(true),
        mAutoGenerateTangents(true),
        mLoadMorphTargets(true),
        mDualQuaternionSkinning(false)
    {
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_LoadActorSettingsWindow.cpp>
