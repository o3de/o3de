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
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <QListWidget>
#include <QDialog>
#endif



namespace EMStudio
{
    class EMSTUDIO_API MorphTargetSelectionWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MorphTargetSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        MorphTargetSelectionWindow(QWidget* parent, bool multiSelect);
        virtual ~MorphTargetSelectionWindow();

        void Update(EMotionFX::MorphSetup* morphSetup, const AZStd::vector<uint32>& selection);
        const AZStd::vector<uint32>& GetMorphTargetIDs() const;

    public slots:
        void OnSelectionChanged();

    private:
        AZStd::vector<uint32>   mSelection;
        QListWidget*            mListWidget;
        QPushButton*            mOKButton;
        QPushButton*            mCancelButton;
    };
} // namespace EMStudio