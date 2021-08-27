/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_GAMECONTROLLERWINDOW_H
#define __EMSTUDIO_GAMECONTROLLERWINDOW_H

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"


#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Debug/Timer.h>
#include <MysticQt/Source/DialogStack.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphGameControllerSettings.h>

#include "AttributesWindow.h"

#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
#include "GameController.h"
#endif

#include <QWidget>
#include <QSplitter>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QBasicTimer>
#include <QComboBox>

#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#endif

#define NO_GAMECONTROLLER_NAME "None"

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace AzQtComponents
{
    class SliderInt;
}

namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;

    class GameControllerWindow
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
                 MCORE_MEMORYOBJECTCATEGORY(GameControllerWindow, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        GameControllerWindow(AnimGraphPlugin* plugin, QWidget* parent = nullptr);
        ~GameControllerWindow();

        void Init();
        void ReInit();
        void DisablePresetInterface();

        MCORE_INLINE bool GetIsGameControllerValid() const
        {
            #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
            if (m_gameController == nullptr)
            {
                return false;
            }
            if (m_gameControllerComboBox->currentIndex() == 0)
            {
                return false;
            }
            return m_gameController->GetIsValid();
            #else
            return false;
            #endif
        }

    private slots:
        void OnGameControllerComboBox(int value);
        void OnParameterModeComboBox(int value);
        void OnAxisComboBox(int value);
        void OnPresetComboBox(int value);
        void OnInvertCheckBoxChanged(int state);
        void OnAddPresetButton();
        void OnRemovePresetButton();
        void OnPresetNameEdited(const QString& text);
        void OnPresetNameChanged();
        void OnDeadZoneSliderChanged(int value);

        void HardwareChangeDetected();

        void OnButtonModeComboBox(int value);
        void OnButtonParameterComboBox(int value);

        void OnSelectNodeButtonClicked();

    private:
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateBlendParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveBlendParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustBlendParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);

        CommandCreateBlendParameterCallback*    m_createCallback;
        CommandRemoveBlendParameterCallback*    m_removeCallback;
        CommandAdjustBlendParameterCallback*    m_adjustCallback;
        CommandSelectCallback*                  m_selectCallback;
        CommandUnselectCallback*                m_unselectCallback;
        CommandClearSelectionCallback*          m_clearSelectionCallback;

        struct ParameterInfo
        {
            MCORE_MEMORYOBJECTCATEGORY(GameControllerWindow::ParameterInfo, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

            const EMotionFX::Parameter*         m_parameter;
            QComboBox*                          m_axis;
            QComboBox*                          m_mode;
            QCheckBox*                          m_invert;
            QLineEdit*                          m_value;
        };

        struct ButtonInfo
        {
            MCORE_MEMORYOBJECTCATEGORY(GameControllerWindow::ButtonInfo, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
            ButtonInfo(uint32 index, QWidget* widget)
            {
                m_buttonIndex    = index;
                m_widget         = widget;
            }

            uint32                              m_buttonIndex;
            QWidget*                            m_widget;
        };

        ParameterInfo* FindParamInfoByModeComboBox(QComboBox* comboBox);
        ParameterInfo* FindParamInfoByAxisComboBox(QComboBox* comboBox);
        ParameterInfo* FindParamInfoByCheckBox(QCheckBox* checkBox);
        ParameterInfo* FindButtonInfoByAttributeInfo(const EMotionFX::Parameter* parameter);
        ButtonInfo* FindButtonInfo(QWidget* widget);

        void ReInitButtonInterface(uint32 buttonIndex);
        void UpdateParameterInterface(ParameterInfo* parameterInfo);
        void UpdateGameControllerComboBox();

        AnimGraphPlugin*                m_plugin;
        AZStd::vector<QLabel*>           m_previewLabels;
        AZStd::vector<ParameterInfo>     m_parameterInfos;
        AZStd::vector<ButtonInfo>        m_buttonInfos;
        QBasicTimer                     m_interfaceTimer;
        QBasicTimer                     m_gameControllerTimer;
        AZ::Debug::Timer                m_deltaTimer;
        int                             m_interfaceTimerId;
        int                             m_gameControllerTimerId;

        #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        GameController*             m_gameController;
        #endif
        EMotionFX::AnimGraph*          m_animGraph;

        MysticQt::DialogStack*          m_dialogStack;

        QWidget*                        m_dynamicWidget;
        AzQtComponents::SliderInt*      m_deadZoneSlider;
        QLabel*                         m_deadZoneValueLabel;
        QGridLayout*                    m_parameterGridLayout;
        QGridLayout*                    m_buttonGridLayout;
        QComboBox*                      m_gameControllerComboBox;

        // preset interface elements
        QComboBox*                      m_presetComboBox;
        QLineEdit*                      m_presetNameLineEdit;
        QPushButton*                    m_addPresetButton;
        QPushButton*                    m_removePresetButton;

        AZStd::string                   m_string;

        void timerEvent(QTimerEvent* event);
        void InitGameController();
        void AutoSelectGameController();
    };
} // namespace EMStudio

#endif
