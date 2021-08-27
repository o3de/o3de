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
#include <QDialog>
#include <MCore/Source/StandardHeaders.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#endif

QT_FORWARD_DECLARE_CLASS(QPushButton)


namespace EMStudio
{
    class EMSTUDIO_API UnitScaleWindow
        : public QDialog
    {
        Q_OBJECT
                       MCORE_MEMORYOBJECTCATEGORY(UnitScaleWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        UnitScaleWindow(QWidget* parent);
        ~UnitScaleWindow();

        float GetScaleFactor() const            { return m_scaleFactor; }

    private slots:
        void OnOKButton();
        void OnCancelButton();

    private:
        float                           m_scaleFactor;
        QPushButton*                    m_ok;
        QPushButton*                    m_cancel;
        AzQtComponents::DoubleSpinBox*  m_scaleSpinBox;
    };
} // namespace EMStudio
