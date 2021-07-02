/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            bool mLoadMeshes;
            bool mLoadCollisionMeshes;
            bool mLoadStandardMaterialLayers;
            bool mLoadSkinningInfo;
            bool mLoadLimits;
            bool mLoadGeometryLODs;
            bool mLoadSkeletalLODs;
            bool mLoadTangents;
            bool mAutoGenerateTangents;
            bool mLoadMorphTargets;
            bool mDualQuaternionSkinning;

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

        QCheckBox* mLoadMeshesCheckbox;
        QCheckBox* mLoadCollisionMeshesCheckbox;
        QCheckBox* mLoadStandardMaterialLayersCheckbox;
        QCheckBox* mLoadSkinningInfoCheckbox;
        QCheckBox* mLoadLimitsCheckbox;
        QCheckBox* mLoadGeometryLODsCheckbox;
        QCheckBox* mLoadSkeletalLODsCheckbox;
        QCheckBox* mLoadTangentsCheckbox;
        QCheckBox* mAutoGenerateTangentsCheckbox;
        QCheckBox* mLoadMorphTargetsCheckbox;
        QCheckBox* mDualQuaternionSkinningCheckbox;
    };
} // namespace EMStudio
