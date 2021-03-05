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

        float GetScaleFactor() const            { return mScaleFactor; }

    private slots:
        void OnOKButton();
        void OnCancelButton();

    private:
        float                           mScaleFactor;
        QPushButton*                    mOK;
        QPushButton*                    mCancel;
        AzQtComponents::DoubleSpinBox*  mScaleSpinBox;
    };
} // namespace EMStudio
