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
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MorphTarget.h>
#include "PhonemeSelectionWindow.h"
#include <QDialog>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#endif


namespace EMStudio
{
    class MorphTargetEditWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MorphTargetEditWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MorphTargetEditWindow(EMotionFX::ActorInstance* actorInstance, EMotionFX::MorphTarget* morphTarget, QWidget* parent);
        ~MorphTargetEditWindow();

        void UpdateInterface();
        EMotionFX::MorphTarget* GetMorphTarget()            { return m_morphTarget; }

    public slots:
        void Accepted();
        void MorphTargetRangeMinValueChanged(double value);
        void MorphTargetRangeMaxValueChanged(double value);
        void EditPhonemeButtonClicked();

    private:
        EMotionFX::ActorInstance*       m_actorInstance;
        EMotionFX::MorphTarget*         m_morphTarget;
        AzQtComponents::DoubleSpinBox*  m_rangeMin;
        AzQtComponents::DoubleSpinBox*  m_rangeMax;
        PhonemeSelectionWindow*         m_phonemeSelectionWindow;
    };
} // namespace EMStudio
