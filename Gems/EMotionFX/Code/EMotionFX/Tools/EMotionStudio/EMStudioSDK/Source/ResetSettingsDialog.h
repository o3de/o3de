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
#include <MCore/Source/MemoryObject.h>
#include <QDialog>
#endif

QT_FORWARD_DECLARE_CLASS(QCheckBox)

namespace EMStudio
{
    class EMSTUDIO_API ResetSettingsDialog
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ResetSettingsDialog, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        ResetSettingsDialog(QWidget* parent);

        bool IsActorsChecked() const;
        bool IsMotionsChecked() const;
        bool IsMotionSetsChecked() const;
        bool IsAnimGraphsChecked() const;

    private:
        QCheckBox*          m_actorCheckbox;
        QCheckBox*          m_motionSetCheckbox;
        QCheckBox*          m_motionCheckbox;
        QCheckBox*          m_animGraphCheckbox;
    };
} // namespace EMStudio
