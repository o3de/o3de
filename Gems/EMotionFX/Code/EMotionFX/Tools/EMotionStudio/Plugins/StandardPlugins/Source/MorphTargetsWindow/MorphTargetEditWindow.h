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
        EMotionFX::MorphTarget* GetMorphTarget()            { return mMorphTarget; }

    public slots:
        void Accepted();
        void MorphTargetRangeMinValueChanged(double value);
        void MorphTargetRangeMaxValueChanged(double value);
        void EditPhonemeButtonClicked();

    private:
        EMotionFX::ActorInstance*       mActorInstance;
        EMotionFX::MorphTarget*         mMorphTarget;
        AzQtComponents::DoubleSpinBox*  mRangeMin;
        AzQtComponents::DoubleSpinBox*  mRangeMax;
        PhonemeSelectionWindow*         mPhonemeSelectionWindow;
    };
} // namespace EMStudio
