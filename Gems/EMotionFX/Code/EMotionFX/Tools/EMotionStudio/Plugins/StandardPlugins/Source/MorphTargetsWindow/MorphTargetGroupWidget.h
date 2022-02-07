/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/MorphTarget.h>
#include <EMotionFX/Source/MorphSetupInstance.h>
#include <EMotionFX/Source/ActorInstance.h>
#include "MorphTargetEditWindow.h"
#include <QWidget>
#include <QCheckBox>
#endif


namespace AzQtComponents
{
    class SliderDoubleCombo;
}

namespace EMStudio
{
    class MorphTargetGroupWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MorphTargetGroupWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MorphTargetGroupWidget(const char* name, EMotionFX::ActorInstance* actorInstance, const AZStd::vector<EMotionFX::MorphTarget*>& morphTargets, const AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>& morphTargetInstances, QWidget* parent);
        ~MorphTargetGroupWidget();

        struct MorphTarget
        {
            EMotionFX::MorphTarget*                     m_morphTarget;
            EMotionFX::MorphSetupInstance::MorphTarget* m_morphTargetInstance;
            QCheckBox*                                  m_manualMode;
            AzQtComponents::SliderDoubleCombo*          m_sliderWeight = nullptr;
            float                                       m_oldWeight;
        };

        void UpdateInterface();
        void UpdateMorphTarget(const char* name);
        const MorphTarget* GetMorphTarget(size_t index){ return &m_morphTargets[index]; }

    public slots:
        void SetManualModeForAll(int value);
        void ManualModeClicked();
        void SliderWeightMoved();
        void SliderWeightReleased();
        void EditClicked();
        void ResetAll();

    private:
        AZStd::string               m_name;
        EMotionFX::ActorInstance*   m_actorInstance;
        QCheckBox*                  m_selectAll;
        AZStd::vector<MorphTarget>  m_morphTargets;
        MorphTargetEditWindow*      m_editWindow;
    };
} // namespace EMStudio
