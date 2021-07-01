/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_GAMECONTROLLERWINDOW_H
#define __EMSTUDIO_GAMECONTROLLERWINDOW_H

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"


#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
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
            if (mGameController == nullptr)
            {
                return false;
            }
            if (mGameControllerComboBox->currentIndex() == 0)
            {
                return false;
            }
            return mGameController->GetIsValid();
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

        CommandCreateBlendParameterCallback*    mCreateCallback;
        CommandRemoveBlendParameterCallback*    mRemoveCallback;
        CommandAdjustBlendParameterCallback*    mAdjustCallback;
        CommandSelectCallback*                  mSelectCallback;
        CommandUnselectCallback*                mUnselectCallback;
        CommandClearSelectionCallback*          mClearSelectionCallback;

        struct ParameterInfo
        {
            MCORE_MEMORYOBJECTCATEGORY(GameControllerWindow::ParameterInfo, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

            const EMotionFX::Parameter*         mParameter;
            QComboBox*                          mAxis;
            QComboBox*                          mMode;
            QCheckBox*                          mInvert;
            QLineEdit*                          mValue;
        };

        struct ButtonInfo
        {
            MCORE_MEMORYOBJECTCATEGORY(GameControllerWindow::ButtonInfo, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
            ButtonInfo(uint32 index, QWidget* widget)
            {
                mButtonIndex    = index;
                mWidget         = widget;
            }

            uint32                              mButtonIndex;
            QWidget*                            mWidget;
        };

        ParameterInfo* FindParamInfoByModeComboBox(QComboBox* comboBox);
        ParameterInfo* FindParamInfoByAxisComboBox(QComboBox* comboBox);
        ParameterInfo* FindParamInfoByCheckBox(QCheckBox* checkBox);
        ParameterInfo* FindButtonInfoByAttributeInfo(const EMotionFX::Parameter* parameter);
        ButtonInfo* FindButtonInfo(QWidget* widget);

        void ReInitButtonInterface(uint32 buttonIndex);
        void UpdateParameterInterface(ParameterInfo* parameterInfo);
        void UpdateGameControllerComboBox();

        AnimGraphPlugin*                mPlugin;
        MCore::Array<QLabel*>           mPreviewLabels;
        MCore::Array<ParameterInfo>     mParameterInfos;
        MCore::Array<ButtonInfo>        mButtonInfos;
        QBasicTimer                     mInterfaceTimer;
        QBasicTimer                     mGameControllerTimer;
        AZ::Debug::Timer                mDeltaTimer;
        int                             mInterfaceTimerID;
        int                             mGameControllerTimerID;

        #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        GameController*             mGameController;
        #endif
        EMotionFX::AnimGraph*          mAnimGraph;

        MysticQt::DialogStack*          mDialogStack;

        QWidget*                        mDynamicWidget;
        AzQtComponents::SliderInt*      mDeadZoneSlider;
        QLabel*                         mDeadZoneValueLabel;
        QGridLayout*                    mParameterGridLayout;
        QGridLayout*                    mButtonGridLayout;
        QComboBox*                      mGameControllerComboBox;

        // preset interface elements
        QComboBox*                      mPresetComboBox;
        QLineEdit*                      mPresetNameLineEdit;
        QPushButton*                    mAddPresetButton;
        QPushButton*                    mRemovePresetButton;

        AZStd::string                   mString;

        void timerEvent(QTimerEvent* event);
        void InitGameController();
        void AutoSelectGameController();
    };
} // namespace EMStudio

#endif
