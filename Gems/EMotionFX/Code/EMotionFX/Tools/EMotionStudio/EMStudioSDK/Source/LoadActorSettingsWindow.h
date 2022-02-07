/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "EMStudioConfig.h"
#include <QCheckBox>
#include <QDialog>
#endif


namespace EMStudio
{
    class EMSTUDIO_API LoadActorSettingsWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(LoadActorSettingsWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        struct LoadActorSettings
        {
            bool m_loadMeshes;
            bool m_loadCollisionMeshes;
            bool m_loadStandardMaterialLayers;
            bool m_loadSkinningInfo;
            bool m_loadLimits;
            bool m_loadGeometryLoDs;
            bool m_loadSkeletalLoDs;
            bool m_loadTangents;
            bool m_autoGenerateTangents;
            bool m_loadMorphTargets;
            bool m_dualQuaternionSkinning;

            LoadActorSettings();
        };

    public:
        LoadActorSettingsWindow(QWidget* parent, const AZStd::string& filePath);
        virtual ~LoadActorSettingsWindow();
        LoadActorSettings GetLoadActorSettings() const;

    public slots:
        void Accepted();
        void LoadMeshesClicked(bool checked);
        void LoadSkinningInfoClicked(bool checked);

    private:
        QString GetConfigFilename() const;

        QCheckBox* m_loadMeshesCheckbox;
        QCheckBox* m_loadCollisionMeshesCheckbox;
        QCheckBox* m_loadStandardMaterialLayersCheckbox;
        QCheckBox* m_loadSkinningInfoCheckbox;
        QCheckBox* m_loadLimitsCheckbox;
        QCheckBox* m_loadGeometryLoDsCheckbox;
        QCheckBox* m_loadSkeletalLoDsCheckbox;
        QCheckBox* m_loadTangentsCheckbox;
        QCheckBox* m_autoGenerateTangentsCheckbox;
        QCheckBox* m_loadMorphTargetsCheckbox;
        QCheckBox* m_dualQuaternionSkinningCheckbox;
    };
} // namespace EMStudio
