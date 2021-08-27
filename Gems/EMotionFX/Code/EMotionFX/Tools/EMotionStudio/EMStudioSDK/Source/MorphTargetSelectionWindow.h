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
        AZStd::vector<uint32>   m_selection;
        QListWidget*            m_listWidget;
        QPushButton*            m_okButton;
        QPushButton*            m_cancelButton;
    };
} // namespace EMStudio
