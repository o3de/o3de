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
#include <MCore/Source/RefCounted.h>
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
