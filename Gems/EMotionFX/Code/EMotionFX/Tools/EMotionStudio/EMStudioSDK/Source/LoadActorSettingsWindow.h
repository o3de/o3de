/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
