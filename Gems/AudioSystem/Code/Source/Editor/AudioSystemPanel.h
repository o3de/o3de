/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <AudioControl.h>
#include <QTreeWidgetFilter.h>
#include <AudioControlFilters.h>

#include <QWidget>
#include <QMenu>

#include <Source/Editor/ui_AudioSystemPanel.h>
#endif

namespace AudioControls
{
    class CATLControlsModel;
    class QFilterButton;

    //-------------------------------------------------------------------------------------------//
    class CAudioSystemPanel
        : public QWidget
        , public Ui::AudioSystemPanel
    {
        Q_OBJECT

    public:
        CAudioSystemPanel();
        void Reload();
        void SetAllowedControls(EACEControlType type, bool bAllowed);

    private slots:
        void SetNameFilter(QString filter);
        void SetHideConnected(bool bHide);

    private:
        // Filtering
        QTreeWidgetFilter m_filter;
        SImplNameFilter m_nameFilter;
        SImplTypeFilter m_typeFilter;
        bool m_allowedATLTypes[AudioControls::EACEControlType::eACET_NUM_TYPES];
        SHideConnectedFilter m_hideConnectedFilter;
    };
} // namespace AudioControls
