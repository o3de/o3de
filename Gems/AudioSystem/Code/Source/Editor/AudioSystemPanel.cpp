/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioSystemPanel.h>

#include <ATLControlsModel.h>
#include <AudioControl.h>
#include <AudioControlsEditorPlugin.h>
#include <CryFile.h>
#include <CryPath.h>
#include <IAudioSystemEditor.h>
#include <IEditor.h>
#include <QAudioControlEditorIcons.h>

#include <QWidgetAction>
#include <QPushButton>
#include <QPaintEvent>
#include <QPainter>
#include <QMessageBox>
#include <QMimeData>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    CAudioSystemPanel::CAudioSystemPanel()
    {
        setupUi(this);

        m_filter.SetTree(m_pControlList);
        m_filter.AddFilter(&m_nameFilter);
        m_filter.AddFilter(&m_typeFilter);
        m_filter.AddFilter(&m_hideConnectedFilter);

        connect(m_pExternalListFilter, SIGNAL(textChanged(QString)), this, SLOT(SetNameFilter(QString)));
        connect(m_pHideAssignedCheckbox, SIGNAL(clicked(bool)), this, SLOT(SetHideConnected(bool)));

        m_pControlList->setContextMenuPolicy(Qt::CustomContextMenu);
        m_pControlList->UpdateModel();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemPanel::SetNameFilter(QString filter)
    {
        m_nameFilter.SetFilter(filter);
        m_filter.ApplyFilter();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemPanel::SetHideConnected(bool bHide)
    {
        m_hideConnectedFilter.SetHideConnected(bHide);
        m_filter.ApplyFilter();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemPanel::Reload()
    {
        m_pControlList->Refresh();
        m_filter.ApplyFilter();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemPanel::SetAllowedControls(EACEControlType type, bool bAllowed)
    {
        const AudioControls::IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (pAudioSystemEditorImpl)
        {
            m_allowedATLTypes[type] = bAllowed;
            uint nMask = 0;
            for (int i = 0; i < AudioControls::EACEControlType::eACET_NUM_TYPES; ++i)
            {
                if (m_allowedATLTypes[i])
                {
                    nMask |= pAudioSystemEditorImpl->GetCompatibleTypes((EACEControlType)i);
                }
            }
            m_typeFilter.SetAllowedControlsMask(nMask);
            m_filter.ApplyFilter();
        }
    }
} // namespace AudioControls

#include <Source/Editor/moc_AudioSystemPanel.cpp>
