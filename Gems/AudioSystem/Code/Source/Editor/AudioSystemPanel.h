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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
