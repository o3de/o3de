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

#ifndef CRYINCLUDE_EDITOR_INFOBAR_H
#define CRYINCLUDE_EDITOR_INFOBAR_H

#pragma once
// InfoBar.h : header file
//

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <IAudioSystem.h>
#include <HMDBus.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// CInfoBar dialog

namespace Ui {
    class CInfoBar;
}

class CInfoBar
    : public QWidget
    , public IEditorNotifyListener
    , public AZ::VR::VREventBus::Handler
    , private AzToolsFramework::ComponentModeFramework::EditorComponentModeNotificationBus::Handler
{
    Q_OBJECT

    // Construction
public:
    CInfoBar(QWidget* parent = nullptr);
    ~CInfoBar();

    // Toggle the mute audio button
    void ToggleAudio() { OnBnClickedMuteAudio(); }
    void SetSpeedComboBox(double value);

Q_SIGNALS:
    void ActionTriggered(int command);

    // Implementation
protected:
    void IdleUpdate();
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    virtual void OnOK() {};
    virtual void OnCancel() {};

    void OnBnClickedSyncplayer();
    void OnBnClickedGotoPosition();

    void OnSpeedComboBoxEnter();
    void OnUpdateMoveSpeedText(const QString&);
    void OnBnClickedTerrainCollision();
    void OnBnClickedPhysics();
    void OnBnClickedSingleStepPhys();
    void OnBnClickedDoStepPhys();
    void OnBnClickedMuteAudio();
    void OnBnClickedEnableVR();
    void OnInitDialog();

    //////////////////////////////////////////////////////////////////////////
    /// VR Event Bus Implementation
    //////////////////////////////////////////////////////////////////////////
    void OnHMDInitialized() override;
    void OnHMDShutdown() override;
    //////////////////////////////////////////////////////////////////////////

    // EditorComponentModeNotificationBus
    void EnteredComponentMode(const AZStd::vector<AZ::Uuid>& componentModeTypes) override;
    void LeftComponentMode(const AZStd::vector<AZ::Uuid>& componentModeTypes) override;

    float m_width, m_height;
    //int m_heightMapX,m_heightMapY;
    double m_fieldWidthMultiplier = 1.8;

    int m_numSelected;
    float m_prevMoveSpeed;

    // Speed combobox/lineEdit settings
    double m_minSpeed = 0.1;
    double m_maxSpeed = 100.0;
    double m_speedStep = 0.1;
    int m_numDecimals = 1;

    // Speed presets
    float m_speedPresetValues[3] = { 0.1f, 1.0f, 10.0f };

    bool m_bSelectionChanged;

    bool m_bDragMode;
    QString m_sLastText;

    Vec3 m_lastValue;
    Vec3 m_currValue;
    float m_oldMainVolume;

    Audio::SAudioRequest m_oMuteAudioRequest;
    Audio::SAudioManagerRequestData<Audio::eAMRT_MUTE_ALL> m_oMuteAudioRequestData;
    Audio::SAudioRequest m_oUnmuteAudioRequest;
    Audio::SAudioManagerRequestData<Audio::eAMRT_UNMUTE_ALL> m_oUnmuteAudioRequestData;

    QScopedPointer<Ui::CInfoBar> ui;

    bool m_idleUpdateEnabled = true;
};

#endif // CRYINCLUDE_EDITOR_INFOBAR_H
