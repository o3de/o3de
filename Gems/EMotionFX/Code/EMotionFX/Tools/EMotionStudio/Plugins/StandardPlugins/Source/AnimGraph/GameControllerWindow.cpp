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

#include "GameControllerWindow.h"

#include "AnimGraphPlugin.h"
#include "ParameterWindow.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QString>
#include <QTimerEvent>
#include <QComboBox>

#include <IEditor.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/Parameter/TagParameter.h>
#include <MCore/Source/LogManager.h>
#include <MysticQt/Source/DialogStack.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Components/Widgets/Slider.h>
#include "BlendNodeSelectionWindow.h"
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphHierarchyWidget.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>

namespace EMStudio
{
    // constructor
    GameControllerWindow::GameControllerWindow(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        mPlugin                     = plugin;
        mAnimGraph                  = nullptr;
        mDynamicWidget              = nullptr;
        mPresetNameLineEdit         = nullptr;
        mParameterGridLayout        = nullptr;
        mDeadZoneValueLabel         = nullptr;
        mButtonGridLayout           = nullptr;
        mDeadZoneSlider             = nullptr;
        mPresetComboBox             = nullptr;
        mInterfaceTimerID           = MCORE_INVALIDINDEX32;
        mGameControllerTimerID      = MCORE_INVALIDINDEX32;
        mString.reserve(4096);
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        mGameController         = nullptr;
    #endif

        Init();
    }


    // destructor
    GameControllerWindow::~GameControllerWindow()
    {
        // stop the timers
        mInterfaceTimer.stop();
        mGameControllerTimer.stop();

        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mCreateCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        delete mCreateCallback;
        delete mRemoveCallback;
        delete mAdjustCallback;
        delete mClearSelectionCallback;
        delete mSelectCallback;
        delete mUnselectCallback;

        // get rid of the game controller
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        if (mGameController)
        {
            mGameController->Shutdown();
            delete mGameController;
        }
    #endif
    }


    // initialize the game controller window
    void GameControllerWindow::Init()
    {
        // create the callbacks
        mCreateCallback             = new CommandCreateBlendParameterCallback(false);
        mRemoveCallback             = new CommandRemoveBlendParameterCallback(false);
        mAdjustCallback             = new CommandAdjustBlendParameterCallback(false);
        mSelectCallback             = new CommandSelectCallback(false);
        mUnselectCallback           = new CommandUnselectCallback(false);
        mClearSelectionCallback     = new CommandClearSelectionCallback(false);

        // hook the callbacks to the commands
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateParameter", mCreateCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveParameter", mRemoveCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustParameter", mAdjustCallback);
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);

        InitGameController();

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        setLayout(layout);

        // create the dialog stack
        mDialogStack = new MysticQt::DialogStack();
        layout->addWidget(mDialogStack);

        // add the game controller
        mGameControllerComboBox = new QComboBox();
        UpdateGameControllerComboBox();

        QHBoxLayout* gameControllerLayout = new QHBoxLayout();
        gameControllerLayout->setMargin(0);
        QLabel* activeControllerLabel = new QLabel("Active Controller:");
        activeControllerLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        gameControllerLayout->addWidget(activeControllerLabel);
        gameControllerLayout->addWidget(mGameControllerComboBox);
        gameControllerLayout->addWidget(EMStudioManager::MakeSeperatorLabel(1, 20));

        // create the presets interface
        QHBoxLayout* horizontalLayout = new QHBoxLayout();
        horizontalLayout->setMargin(0);

        mPresetComboBox     = new QComboBox();
        mAddPresetButton    = new QPushButton();
        mRemovePresetButton = new QPushButton();
        mPresetNameLineEdit = new QLineEdit();

        connect(mPresetComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &GameControllerWindow::OnPresetComboBox);
        connect(mAddPresetButton, &QPushButton::clicked, this, &GameControllerWindow::OnAddPresetButton);
        connect(mRemovePresetButton, &QPushButton::clicked, this, &GameControllerWindow::OnRemovePresetButton);
        connect(mPresetNameLineEdit, &QLineEdit::textEdited, this, &GameControllerWindow::OnPresetNameEdited);
        connect(mPresetNameLineEdit, &QLineEdit::returnPressed, this, &GameControllerWindow::OnPresetNameChanged);

        EMStudioManager::MakeTransparentButton(mAddPresetButton, "/Images/Icons/Plus.svg", "Add a game controller preset");
        EMStudioManager::MakeTransparentButton(mRemovePresetButton, "/Images/Icons/Remove.svg", "Remove a game controller preset");

        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        buttonsLayout->addWidget(mAddPresetButton);
        buttonsLayout->addWidget(mRemovePresetButton);
        buttonsLayout->setSpacing(0);
        buttonsLayout->setMargin(0);

        horizontalLayout->addWidget(new QLabel("Preset:"));
        horizontalLayout->addWidget(mPresetComboBox);
        horizontalLayout->addLayout(buttonsLayout);
        horizontalLayout->addWidget(mPresetNameLineEdit);

        gameControllerLayout->addLayout(horizontalLayout);
        QWidget* dummyWidget = new QWidget();
        dummyWidget->setObjectName("StyledWidgetDark");
        dummyWidget->setLayout(gameControllerLayout);
        mDialogStack->Add(dummyWidget, "Game Controller And Preset Selection");
        connect(mGameControllerComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &GameControllerWindow::OnGameControllerComboBox);

        DisablePresetInterface();
        AutoSelectGameController();

        connect(GetMainWindow(), &MainWindow::HardwareChangeDetected, this, &GameControllerWindow::HardwareChangeDetected);
    }


    // automatically selec the game controller in the combo box
    void GameControllerWindow::AutoSelectGameController()
    {
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        // this will call ReInit();
        if (mGameController->GetDeviceNameString().empty() == false && mGameControllerComboBox->count() > 1)
        {
            mGameControllerComboBox->setCurrentIndex(1);
        }
        else
        {
            mGameControllerComboBox->setCurrentIndex(0);
        }
    #endif
    }


    // initialize the game controller
    void GameControllerWindow::InitGameController()
    {
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        if (mGameController)
        {
            mGameController->Shutdown();
            delete mGameController;
            mGameController = nullptr;
        }

        // create the game controller object
        mGameController = new GameController();

        // Call mainWindow->window() to make sure you get the top level window which the mainWindow might not in fact be.
        //IEditor* editor = nullptr;
        //AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        //QMainWindow* mainWindow = editor->GetEditorMainWindow();
        //HWND hWnd = reinterpret_cast<HWND>( mainWindow->window()->winId() );
        HWND hWnd = nullptr;
        if (mGameController->Init(hWnd) == false)
        {
            MCore::LogError("Cannot initialize game controller.");
        }
    #endif
    }


    void GameControllerWindow::UpdateGameControllerComboBox()
    {
        // clear it and add the none option
        mGameControllerComboBox->clear();
        mGameControllerComboBox->addItem(NO_GAMECONTROLLER_NAME);

        // add the gamepad in case it is valid and the device name is not empty
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        if (mGameController->GetIsValid() && mGameController->GetDeviceNameString().empty() == false)
        {
            mGameControllerComboBox->addItem(mGameController->GetDeviceName());
        }
    #endif

        // always adjust the size of the combobox to the currently selected text
        mGameControllerComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    }


    // adjust the game controller combobox value
    void GameControllerWindow::OnGameControllerComboBox(int value)
    {
        MCORE_UNUSED(value);

        //QComboBox* combo = qobject_cast<QComboBox*>( sender() );
        //String stringValue = combo->currentText().toAscii().data();
        ReInit();

        // update the parameter window
        mPlugin->GetParameterWindow()->Reinit(/*forceReinit*/true);
    }


    void GameControllerWindow::DisablePresetInterface()
    {
        mPresetComboBox->blockSignals(true);
        mPresetComboBox->clear();
        mPresetComboBox->blockSignals(false);

        mPresetNameLineEdit->blockSignals(true);
        mPresetNameLineEdit->setText("");
        mPresetNameLineEdit->blockSignals(false);

        mPresetComboBox->setEnabled(false);
        mPresetNameLineEdit->setEnabled(false);
        mAddPresetButton->setEnabled(false);
        mRemovePresetButton->setEnabled(false);
    }


    // reint the game controller window
    void GameControllerWindow::ReInit()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        mAnimGraph = animGraph;

        // remove all existing items
        if (mDynamicWidget)
        {
            mDialogStack->Remove(mDynamicWidget);
        }
        mDynamicWidget = nullptr;
        mInterfaceTimer.stop();
        mGameControllerTimer.stop();

        // check if we need to recreate the dynamic widget
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        if (mGameController->GetIsValid() == false || mGameControllerComboBox->currentText() != mGameController->GetDeviceName())
        {
            DisablePresetInterface();
            return;
        }
    #else
        DisablePresetInterface();
        return;
    #endif

        if (animGraph == nullptr)
        {
            DisablePresetInterface();
            return;
        }

        // create the dynamic widget
        mDynamicWidget = new QWidget();
        mDynamicWidget->setObjectName("StyledWidgetDark");

        // get the game controller settings from the anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = animGraph->GetGameControllerSettings();

        // in case there is no preset yet create a default one
        uint32 numPresets = gameControllerSettings.GetNumPresets();
        if (numPresets == 0)
        {
            EMotionFX::AnimGraphGameControllerSettings::Preset* preset = aznew EMotionFX::AnimGraphGameControllerSettings::Preset("Default");
            gameControllerSettings.AddPreset(preset);
            gameControllerSettings.SetActivePreset(preset);
            numPresets = 1;
        }

        // get the active preset
        EMotionFX::AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetActivePreset();

        // create the parameter grid layout
        mParameterGridLayout = new QGridLayout();
        mParameterGridLayout->setAlignment(Qt::AlignTop);
        mParameterGridLayout->setMargin(0);

        // add all parameters
        //  uint32 startRow = 0;
        mParameterInfos.Clear();

        const EMotionFX::ValueParameterVector& parameters = animGraph->RecursivelyGetValueParameters();
        const size_t numParameters = parameters.size();
        mParameterInfos.Reserve(static_cast<uint32>(numParameters));

        for (size_t parameterIndex = 0; parameterIndex < numParameters; ++parameterIndex)
        {
            const EMotionFX::ValueParameter* parameter = parameters[parameterIndex];

            if (!azrtti_istypeof<EMotionFX::FloatParameter>(parameter)
                && azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::Vector2Parameter>())
            {
                continue;
            }

            EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* settingsInfo = activePreset->FindParameterInfo(parameter->GetName().c_str());
            if (settingsInfo == nullptr)
            {
                continue;
            }

            // add the parameter name to the layout
            AZStd::string labelString = parameter->GetName();
            labelString += ":";
            QLabel* label = new QLabel(labelString.c_str());
            label->setToolTip(parameter->GetDescription().c_str());
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            mParameterGridLayout->addWidget(label, parameterIndex, 0);

            // add the axis combo box to the layout
            QComboBox* axesComboBox = new QComboBox();
            axesComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            axesComboBox->addItem("None");

            // iterate over the elements and add the ones which are present on the current game controller to the combo box
            uint32 selectedComboItem = 0;
        #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
            if (parameter->GetType() == MCore::AttributeFloat::TYPE_ID)
            {
                uint32 numPresentElements = 0;
                for (uint32 j = 0; j < GameController::NUM_ELEMENTS; ++j)
                {
                    // check if the element is present and add it to the combo box if yes
                    if (mGameController->GetIsPresent(j))
                    {
                        // add the name of the element to the combo box
                        axesComboBox->addItem(mGameController->GetElementEnumName(j));

                        // in case the current element is the one the parameter is assigned to, remember the correct index
                        if (j == settingsInfo->m_axis)
                        {
                            selectedComboItem = numPresentElements + 1;
                        }

                        // increase the number of present elements on the plugged in game controller
                        numPresentElements++;
                    }
                }
            }
            else if (parameter->GetType() == MCore::AttributeVector2::TYPE_ID)
            {
                uint32 numPresentElements = 0;
                if (mGameController->GetIsPresent(GameController::ELEM_POS_X) && mGameController->GetIsPresent(GameController::ELEM_POS_Y))
                {
                    axesComboBox->addItem("Pos XY");
                    if (settingsInfo->m_axis == 0)
                    {
                        selectedComboItem = numPresentElements + 1;
                    }
                    numPresentElements++;
                }

                if (mGameController->GetIsPresent(GameController::ELEM_ROT_X) && mGameController->GetIsPresent(GameController::ELEM_ROT_Y))
                {
                    axesComboBox->addItem("Rot XY");
                    if (settingsInfo->m_axis == 1)
                    {
                        selectedComboItem = numPresentElements + 1;
                    }
                    numPresentElements++;
                }
            }
        #endif
            connect(axesComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &GameControllerWindow::OnAxisComboBox);

            // select the given axis in the combo box or select none if there is no assignment yet or the assigned axis wasn't found on the current game controller
            axesComboBox->setCurrentIndex(selectedComboItem);
            mParameterGridLayout->addWidget(axesComboBox, parameterIndex, 1);

            // add the mode combo box to the layout
            QComboBox* modeComboBox = new QComboBox();
            modeComboBox->addItem("Standard Mode");
            modeComboBox->addItem("Zero To One Mode");
            modeComboBox->addItem("Parameter Range Mode");
            modeComboBox->addItem("Positive Param Range Mode");
            modeComboBox->addItem("Negative Param Range Mode");
            modeComboBox->addItem("Rotate Character");
            modeComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            connect(modeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &GameControllerWindow::OnParameterModeComboBox);
            modeComboBox->setCurrentIndex(settingsInfo->m_mode);
            mParameterGridLayout->addWidget(modeComboBox, parameterIndex, 2);

            // add the invert checkbox to the layout
            QHBoxLayout* invertCheckBoxLayout = new QHBoxLayout();
            invertCheckBoxLayout->setMargin(0);
            QLabel* invertLabel = new QLabel("Invert");
            invertCheckBoxLayout->addWidget(invertLabel);
            QCheckBox* invertCheckbox = new QCheckBox();
            invertLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            invertCheckbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            connect(invertCheckbox, &QCheckBox::stateChanged, this, &GameControllerWindow::OnInvertCheckBoxChanged);
            invertCheckbox->setCheckState(settingsInfo->m_invert ? Qt::Checked : Qt::Unchecked);
            invertCheckBoxLayout->addWidget(invertCheckbox);
            mParameterGridLayout->addLayout(invertCheckBoxLayout, parameterIndex, 3);

            // add the current value edit field to the layout
            QLineEdit* valueEdit = new QLineEdit();
            valueEdit->setEnabled(false);
            valueEdit->setReadOnly(true);
            valueEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            valueEdit->setMinimumWidth(70);
            valueEdit->setMaximumWidth(70);
            mParameterGridLayout->addWidget(valueEdit, parameterIndex, 4);

            // create the parameter info and add it to the array
            ParameterInfo paramInfo;
            paramInfo.mParameter = parameter;
            paramInfo.mAxis      = axesComboBox;
            paramInfo.mMode      = modeComboBox;
            paramInfo.mInvert    = invertCheckbox;
            paramInfo.mValue     = valueEdit;
            mParameterInfos.Add(paramInfo);

            // update the interface
            UpdateParameterInterface(&paramInfo);
        }

        // create the button layout
        mButtonGridLayout = new QGridLayout();
        mButtonGridLayout->setAlignment(Qt::AlignTop);
        mButtonGridLayout->setMargin(0);

        // clear the button infos
        mButtonInfos.Clear();

        // get the number of buttons and iterate through them
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        const uint32 numButtons = mGameController->GetNumButtons();
        for (uint32 i = 0; i < numButtons; ++i)
        {
            EMotionFX::AnimGraphGameControllerSettings::ButtonInfo* settingsInfo = activePreset->FindButtonInfo(i);
            MCORE_ASSERT(settingsInfo);

            // add the button name to the layout
            mString = AZStd::string::format("Button %s%d", (i < 10) ? "0" : "", i);
            QLabel* nameLabel = new QLabel(mString.c_str());
            nameLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            mButtonGridLayout->addWidget(nameLabel, i, 0);

            // add the mode combo box to the layout
            QComboBox* modeComboBox = new QComboBox();
            modeComboBox->addItem("None");
            modeComboBox->addItem("Switch To State Mode");
            modeComboBox->addItem("Toggle Bool Parameter Mode");
            modeComboBox->addItem("Enable Bool While Pressed Mode");
            modeComboBox->addItem("Disable Bool While Pressed Mode");
            modeComboBox->addItem("Enable Bool For One Frame Only");
            //modeComboBox->addItem( "Execute Script Mode" );
            modeComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            connect(modeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &GameControllerWindow::OnButtonModeComboBox);
            modeComboBox->setCurrentIndex(settingsInfo->m_mode);
            mButtonGridLayout->addWidget(modeComboBox, i, 1);

            mButtonInfos.Add(ButtonInfo(i, modeComboBox));

            // reinit the dynamic part of the button layout
            ReInitButtonInterface(i);
        }

        // real time preview of the controller
        mPreviewLabels.Clear();
        mPreviewLabels.Resize(GameController::NUM_ELEMENTS + 1);
        QVBoxLayout* realtimePreviewLayout = new QVBoxLayout();
        QGridLayout* previewGridLayout = new QGridLayout();
        previewGridLayout->setAlignment(Qt::AlignTop);
        previewGridLayout->setSpacing(5);
        uint32 realTimePreviewLabelCounter = 0;
        for (uint32 i = 0; i < GameController::NUM_ELEMENTS; ++i)
        {
            if (mGameController->GetIsPresent(i))
            {
                QLabel* elementNameLabel = new QLabel(mGameController->GetElementEnumName(i));
                elementNameLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                previewGridLayout->addWidget(elementNameLabel, realTimePreviewLabelCounter, 0);

                mPreviewLabels[i] = new QLabel();
                previewGridLayout->addWidget(mPreviewLabels[i], realTimePreviewLabelCounter, 1, Qt::AlignLeft);

                realTimePreviewLabelCounter++;
            }
            else
            {
                mPreviewLabels[i] = nullptr;
            }
        }
        realtimePreviewLayout->addLayout(previewGridLayout);

        // add the special case label for the pressed buttons
        mPreviewLabels[GameController::NUM_ELEMENTS] = new QLabel();
        QLabel* realtimeButtonNameLabel = new QLabel("Buttons");
        realtimeButtonNameLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        previewGridLayout->addWidget(realtimeButtonNameLabel, realTimePreviewLabelCounter, 0);
        previewGridLayout->addWidget(mPreviewLabels[GameController::NUM_ELEMENTS], realTimePreviewLabelCounter, 1, Qt::AlignLeft);

        // add the dead zone elements
        QHBoxLayout* deadZoneLayout = new QHBoxLayout();
        deadZoneLayout->setMargin(0);

        QLabel* deadZoneLabel = new QLabel("Dead Zone");
        deadZoneLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        previewGridLayout->addWidget(deadZoneLabel, realTimePreviewLabelCounter + 1, 0);

        mDeadZoneSlider = new AzQtComponents::SliderInt(Qt::Horizontal);
        mDeadZoneSlider->setRange(1, 90);
        mDeadZoneSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        deadZoneLayout->addWidget(mDeadZoneSlider);

        mDeadZoneValueLabel = new QLabel();
        deadZoneLayout->addWidget(mDeadZoneValueLabel);
        previewGridLayout->addLayout(deadZoneLayout, realTimePreviewLabelCounter + 1, 1);

        mDeadZoneSlider->setValue(aznumeric_cast<int>(mGameController->GetDeadZone() * 100));
        mString = AZStd::string::format("%.2f", mGameController->GetDeadZone());
        mDeadZoneValueLabel->setText(mString.c_str());
        connect(mDeadZoneSlider, &AzQtComponents::SliderInt::valueChanged, this, &GameControllerWindow::OnDeadZoneSliderChanged);
    #endif

        // start the timers
        mInterfaceTimer.start(1000 / 20, this);
        mInterfaceTimerID = mInterfaceTimer.timerId();
        mGameControllerTimer.start(1000 / 100, this);
        mGameControllerTimerID = mGameControllerTimer.timerId();

        // create the vertical layout for the parameter and the button setup
        QVBoxLayout* verticalLayout = new QVBoxLayout();
        verticalLayout->setAlignment(Qt::AlignTop);

        ////////////////////////////

        mPresetComboBox->blockSignals(true);
        mPresetComboBox->clear();
        // add the presets to the combo box
        for (uint32 i = 0; i < numPresets; ++i)
        {
            mPresetComboBox->addItem(gameControllerSettings.GetPreset(i)->GetName());
        }

        // select the active preset
        const uint32 activePresetIndex = gameControllerSettings.GetActivePresetIndex();
        if (activePresetIndex != MCORE_INVALIDINDEX32)
        {
            mPresetComboBox->setCurrentIndex(activePresetIndex);
        }
        mPresetComboBox->blockSignals(false);

        // set the name of the active preset
        if (gameControllerSettings.GetActivePreset())
        {
            mPresetNameLineEdit->blockSignals(true);
            mPresetNameLineEdit->setText(gameControllerSettings.GetActivePreset()->GetName());
            mPresetNameLineEdit->blockSignals(false);
        }

        mPresetComboBox->setEnabled(true);
        mPresetNameLineEdit->setEnabled(true);
        mAddPresetButton->setEnabled(true);
        mRemovePresetButton->setEnabled(true);

        ////////////////////////////

        // construct the parameters name layout
        QHBoxLayout* parameterNameLayout = new QHBoxLayout();
        QLabel* label = new QLabel("Parameters");
        label->setStyleSheet("color: rgb(244, 156, 28);");
        label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        parameterNameLayout->addWidget(label);

        // add spacer item
        QWidget* spacerItem = new QWidget();
        spacerItem->setStyleSheet("background-color: qlineargradient(x1:0, y1:0, x2:1, y2:, stop:0 rgb(55, 55, 55), stop:0.5 rgb(144, 152, 160), stop:1 rgb(55, 55, 55));");
        spacerItem->setMinimumHeight(1);
        spacerItem->setMaximumHeight(1);
        spacerItem->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        parameterNameLayout->addWidget(spacerItem);


        // construct the buttons name layout
        QHBoxLayout* buttonNameLayout = new QHBoxLayout();
        label = new QLabel("Buttons");
        label->setStyleSheet("color: rgb(244, 156, 28);");
        label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        buttonNameLayout->addWidget(label);

        // add spacer item
        spacerItem = new QWidget();
        spacerItem->setStyleSheet("background-color: qlineargradient(x1:0, y1:0, x2:1, y2:, stop:0 rgb(55, 55, 55), stop:0.5 rgb(144, 152, 160), stop:1 rgb(55, 55, 55));");
        spacerItem->setMinimumHeight(1);
        spacerItem->setMaximumHeight(1);
        spacerItem->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        buttonNameLayout->addWidget(spacerItem);

        verticalLayout->addLayout(parameterNameLayout);
        verticalLayout->addLayout(mParameterGridLayout);
        verticalLayout->addLayout(buttonNameLayout);
        verticalLayout->addLayout(mButtonGridLayout);

        // main dynamic widget layout
        QHBoxLayout* dynamicWidgetLayout = new QHBoxLayout();
        dynamicWidgetLayout->setMargin(0);

        // add the left side
        dynamicWidgetLayout->addLayout(verticalLayout);

        // add the realtime preview window to the dynamic widget layout
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        QWidget* realTimePreviewWidget = new QWidget();
        realTimePreviewWidget->setMinimumWidth(200);
        realTimePreviewWidget->setMaximumWidth(200);
        realTimePreviewWidget->setStyleSheet("background-color: rgb(65, 65, 65);");
        realTimePreviewWidget->setLayout(realtimePreviewLayout);
        dynamicWidgetLayout->addWidget(realTimePreviewWidget);
        dynamicWidgetLayout->setAlignment(realTimePreviewWidget, Qt::AlignTop);
    #endif
        mDynamicWidget->setLayout(dynamicWidgetLayout);

        mDialogStack->Add(mDynamicWidget, "Game Controller Mapping", false, true);
    }


    void GameControllerWindow::OnDeadZoneSliderChanged(int value)
    {
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        mGameController->SetDeadZone(value * 0.01f);
        mString = AZStd::string::format("%.2f", value * 0.01f);
        mDeadZoneValueLabel->setText(mString.c_str());
    #else
        MCORE_UNUSED(value);
    #endif
    }


    GameControllerWindow::ButtonInfo* GameControllerWindow::FindButtonInfo(QWidget* widget)
    {
        // get the number of button infos and iterate through them
        const uint32 numButtonInfos = mButtonInfos.GetLength();
        for (uint32 i = 0; i < numButtonInfos; ++i)
        {
            if (mButtonInfos[i].mWidget == widget)
            {
                return &mButtonInfos[i];
            }
        }

        // return failure
        return nullptr;
    }


    GameControllerWindow::ParameterInfo* GameControllerWindow::FindParamInfoByModeComboBox(QComboBox* comboBox)
    {
        // get the number of parameter infos and iterate through them
        const uint32 numParamInfos = mParameterInfos.GetLength();
        for (uint32 i = 0; i < numParamInfos; ++i)
        {
            if (mParameterInfos[i].mMode == comboBox)
            {
                return &mParameterInfos[i];
            }
        }

        // return failure
        return nullptr;
    }


    // find the interface parameter info based on the attribute info
    GameControllerWindow::ParameterInfo* GameControllerWindow::FindButtonInfoByAttributeInfo(const EMotionFX::Parameter* parameter)
    {
        // get the number of parameter infos and iterate through them
        const uint32 numParamInfos = mParameterInfos.GetLength();
        for (uint32 i = 0; i < numParamInfos; ++i)
        {
            if (mParameterInfos[i].mParameter == parameter)
            {
                return &mParameterInfos[i];
            }
        }

        // return failure
        return nullptr;
    }


    // enable/disable controls for a given parameter
    void GameControllerWindow::UpdateParameterInterface(ParameterInfo* parameterInfo)
    {
        int comboAxisIndex = parameterInfo->mAxis->currentIndex();
        if (comboAxisIndex == 0) // None
        {
            parameterInfo->mMode->setEnabled(false);
            parameterInfo->mInvert->setEnabled(false);
            parameterInfo->mValue->setEnabled(false);
            parameterInfo->mValue->setText("");
        }
        else // some mode set
        {
            parameterInfo->mMode->setEnabled(true);
            parameterInfo->mInvert->setEnabled(true);
            parameterInfo->mValue->setEnabled(true);
        }
    }


    void GameControllerWindow::OnParameterModeComboBox(int value)
    {
        MCORE_UNUSED(value);

        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        // get the active preset
        EMotionFX::AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetActivePreset();
        if (activePreset == nullptr)
        {
            return;
        }

        QComboBox* combo = qobject_cast<QComboBox*>(sender());
        ParameterInfo* paramInfo = FindParamInfoByModeComboBox(combo);
        if (paramInfo == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* settingsInfo = activePreset->FindParameterInfo(paramInfo->mParameter->GetName().c_str());
        MCORE_ASSERT(settingsInfo);
        settingsInfo->m_mode = (EMotionFX::AnimGraphGameControllerSettings::ParameterMode)combo->currentIndex();
    }


    void GameControllerWindow::ReInitButtonInterface(uint32 buttonIndex)
    {
        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        // get the active preset
        EMotionFX::AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetActivePreset();
        if (activePreset == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraphGameControllerSettings::ButtonInfo* settingsInfo = activePreset->FindButtonInfo(buttonIndex);
        MCORE_ASSERT(settingsInfo);

        // remove the old widget
        QLayoutItem* oldLayoutItem = mButtonGridLayout->itemAtPosition(buttonIndex, 2);
        if (oldLayoutItem)
        {
            QWidget* oldWidget = oldLayoutItem->widget();
            if (oldWidget)
            {
                oldWidget->hide();
                oldWidget->deleteLater();
            }
        }

        QWidget* widget = nullptr;
        switch (settingsInfo->m_mode)
        {
        case EMotionFX::AnimGraphGameControllerSettings::BUTTONMODE_NONE:
            break;

        case EMotionFX::AnimGraphGameControllerSettings::BUTTONMODE_SWITCHSTATE:
        {
            widget = new QWidget();
            widget->setObjectName("GameControllerButtonModeSettings");
            widget->setStyleSheet("#GameControllerButtonModeSettings{ background-color: transparent; }");
            QHBoxLayout* layout = new QHBoxLayout();
            layout->setMargin(0);

            auto browseEdit = new AzQtComponents::BrowseEdit();
            browseEdit->setPlaceholderText("Select node");
            browseEdit->setProperty("ButtonIndex", buttonIndex);
            if (settingsInfo->m_string.empty() == false)
            {
                browseEdit->setText(settingsInfo->m_string.c_str());
            }

            connect(browseEdit, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &GameControllerWindow::OnSelectNodeButtonClicked);

            browseEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            layout->addWidget(new QLabel("State:"));
            layout->addWidget(browseEdit);
            widget->setLayout(layout);
            break;
        }

        //case AnimGraphGameControllerSettings::BUTTONMODE_ENABLEBOOLFORONLYONEFRAMEONLY:
        //case AnimGraphGameControllerSettings::BUTTONMODE_TOGGLEBOOLEANPARAMETER:
        //case AnimGraphGameControllerSettings::BUTTONMODE_ENABLEBOOLWHILEPRESSED:
        //case AnimGraphGameControllerSettings::BUTTONMODE_DISABLEBOOLWHILEPRESSED:
        default:
        {
            widget = new QWidget();
            widget->setObjectName("GameControllerButtonModeSettings");
            widget->setStyleSheet("#GameControllerButtonModeSettings{ background-color: transparent; }");
            QHBoxLayout* layout = new QHBoxLayout();
            layout->setMargin(0);
            QComboBox* comboBox = new QComboBox();

            const EMotionFX::ValueParameterVector& valueParameters = mAnimGraph->RecursivelyGetValueParameters();
            for (const EMotionFX::ValueParameter* valueParameter : valueParameters)
            {
                if (azrtti_typeid(valueParameter) == azrtti_typeid<EMotionFX::BoolParameter>() ||
                    azrtti_typeid(valueParameter) == azrtti_typeid<EMotionFX::TagParameter>())
                {
                    comboBox->addItem(valueParameter->GetName().c_str());
                }
            }

            connect(comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &GameControllerWindow::OnButtonParameterComboBox);
            comboBox->setProperty("ButtonIndex", buttonIndex);

            // select the correct parameter
            int comboIndex = comboBox->findText(settingsInfo->m_string.c_str());
            if (comboIndex != -1)
            {
                comboBox->setCurrentIndex(comboIndex);
            }
            else
            {
                if (comboBox->count() > 0)
                {
                    //MCORE_ASSERT(false); // this shouldn't happen, please report Ben
                    //comboBox->setCurrentIndex( 0 );
                    //settingsInfo->m_string = comboBox->itemText(0).toAscii().data();
                }
            }

            layout->addWidget(new QLabel("Bool Parameter:"));
            layout->addWidget(comboBox);
            widget->setLayout(layout);
            break;
        }
        }

        if (widget)
        {
            mButtonGridLayout->addWidget(widget, buttonIndex, 2);
        }
    }



    // open the node selection dialog for the node
    void GameControllerWindow::OnSelectNodeButtonClicked()
    {
        // get the sender widget
        auto browseEdit = qobject_cast<AzQtComponents::BrowseEdit*>(sender());
        if (browseEdit == nullptr)
        {
            return;
        }

        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        // get the active preset
        EMotionFX::AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetActivePreset();
        if (activePreset == nullptr)
        {
            return;
        }

        int32 buttonIndex = browseEdit->property("ButtonIndex").toInt();

        EMotionFX::AnimGraphGameControllerSettings::ButtonInfo* settingsInfo = activePreset->FindButtonInfo(buttonIndex);
        MCORE_ASSERT(settingsInfo);

        // create and show the state selection window
        BlendNodeSelectionWindow stateSelectionWindow(browseEdit);
        stateSelectionWindow.GetAnimGraphHierarchyWidget().SetSingleSelectionMode(true);
        stateSelectionWindow.GetAnimGraphHierarchyWidget().SetFilterNodeType(azrtti_typeid<EMotionFX::AnimGraphStateMachine>());
        stateSelectionWindow.setModal(true);
        if (stateSelectionWindow.exec() == QDialog::Rejected)   // we pressed cancel or the close cross
        {
            return;
        }

        // Get the selected states.
        const AZStd::vector<AnimGraphSelectionItem>& selectedStates = stateSelectionWindow.GetAnimGraphHierarchyWidget().GetSelectedItems();
        if (selectedStates.empty())
        {
            return;
        }

        settingsInfo->m_string = selectedStates[0].mNodeName.c_str();
        browseEdit->setPlaceholderText(selectedStates[0].mNodeName.c_str());
    }



    void GameControllerWindow::OnButtonParameterComboBox(int value)
    {
        MCORE_UNUSED(value);

        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        // get the active preset
        EMotionFX::AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetActivePreset();
        if (activePreset == nullptr)
        {
            return;
        }

        QComboBox* combo = qobject_cast<QComboBox*>(sender());
        int32 buttonIndex = combo->property("ButtonIndex").toInt();

        EMotionFX::AnimGraphGameControllerSettings::ButtonInfo* settingsInfo = activePreset->FindButtonInfo(buttonIndex);
        MCORE_ASSERT(settingsInfo);

        const AZStd::string parameterName = combo->currentText().toUtf8().data();
        const EMotionFX::Parameter* parameter = mAnimGraph->FindParameterByName(parameterName);
        if (parameter)
        {
            settingsInfo->m_string = parameter->GetName();
        }
        else
        {
            settingsInfo->m_string.clear();
        }

        // update the parameter window
        mPlugin->GetParameterWindow()->Reinit(/*forceReinit*/true);
    }



    void GameControllerWindow::OnButtonModeComboBox(int value)
    {
        MCORE_UNUSED(value);

        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        // get the active preset
        EMotionFX::AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetActivePreset();
        if (activePreset == nullptr)
        {
            return;
        }

        QComboBox* combo = qobject_cast<QComboBox*>(sender());
        ButtonInfo* buttonInfo = FindButtonInfo(combo);
        if (buttonInfo == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraphGameControllerSettings::ButtonInfo* settingsInfo = activePreset->FindButtonInfo(buttonInfo->mButtonIndex);
        MCORE_ASSERT(settingsInfo);
        settingsInfo->m_mode = (EMotionFX::AnimGraphGameControllerSettings::ButtonMode)combo->currentIndex();

        // check if the button info is pointing to a correct parameter
        const AZStd::string parameterName = settingsInfo->m_string;
        const EMotionFX::Parameter* parameter = mAnimGraph->FindParameterByName(parameterName);
        if (parameterName.empty())
        {
            // The parameter name is empty in case the button info has not been assigned with one yet.
            // Default it to the first compatible parameter.
            const EMotionFX::ValueParameterVector& valueParameters = mAnimGraph->RecursivelyGetValueParameters();
            for (const EMotionFX::ValueParameter* valueParameter : valueParameters)
            {
                if (azrtti_typeid(valueParameter) == azrtti_typeid<EMotionFX::BoolParameter>() ||
                    azrtti_typeid(valueParameter) == azrtti_typeid<EMotionFX::TagParameter>())
                {
                    settingsInfo->m_string = valueParameter->GetName();
                    break;
                }
            }
        }

        ReInitButtonInterface(buttonInfo->mButtonIndex);

        // update the parameter window
        mPlugin->GetParameterWindow()->Reinit(/*forceReinit*/true);
    }


    void GameControllerWindow::OnAddPresetButton()
    {
        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        uint32 presetNumber = gameControllerSettings.GetNumPresets();
        mString = AZStd::string::format("Preset %d", presetNumber);
        while (gameControllerSettings.FindPresetIndexByName(mString.c_str()) != MCORE_INVALIDINDEX32)
        {
            presetNumber++;
            mString = AZStd::string::format("Preset %d", presetNumber);
        }

        EMotionFX::AnimGraphGameControllerSettings::Preset* preset = aznew EMotionFX::AnimGraphGameControllerSettings::Preset(mString.c_str());
        gameControllerSettings.AddPreset(preset);

        ReInit();
    }


    void GameControllerWindow::OnPresetComboBox(int value)
    {
        MCORE_UNUSED(value);

        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        QComboBox* combo = qobject_cast<QComboBox*>(sender());
        EMotionFX::AnimGraphGameControllerSettings::Preset* preset = gameControllerSettings.GetPreset(combo->currentIndex());
        gameControllerSettings.SetActivePreset(preset);

        ReInit();
    }


    void GameControllerWindow::OnRemovePresetButton()
    {
        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        uint32 presetIndex = mPresetComboBox->currentIndex();
        gameControllerSettings.RemovePreset(presetIndex);

        EMotionFX::AnimGraphGameControllerSettings::Preset* preset = nullptr;
        if (gameControllerSettings.GetNumPresets() > 0)
        {
            if (presetIndex >= gameControllerSettings.GetNumPresets())
            {
                preset = gameControllerSettings.GetPreset(gameControllerSettings.GetNumPresets() - 1);
            }
            else
            {
                preset = gameControllerSettings.GetPreset(presetIndex);
            }
        }

        gameControllerSettings.SetActivePreset(preset);

        ReInit();
    }


    void GameControllerWindow::OnPresetNameChanged()
    {
        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        assert(sender()->inherits("QLineEdit"));
        QLineEdit* widget = qobject_cast<QLineEdit*>(sender());
        AZStd::string newValue;
        FromQtString(widget->text(), &newValue);

        // get the currently selected preset
        uint32 presetIndex = mPresetComboBox->currentIndex();

        uint32 newValueIndex = gameControllerSettings.FindPresetIndexByName(newValue.c_str());
        if (newValueIndex == MCORE_INVALIDINDEX32)
        {
            EMotionFX::AnimGraphGameControllerSettings::Preset* preset = gameControllerSettings.GetPreset(presetIndex);
            preset->SetName(newValue.c_str());
            ReInit();
        }
    }


    void GameControllerWindow::OnPresetNameEdited(const QString& text)
    {
        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        // check if there already is a preset with the currently entered name
        uint32 presetIndex = gameControllerSettings.FindPresetIndexByName(FromQtString(text).c_str());
        if (presetIndex != MCORE_INVALIDINDEX32 && presetIndex != gameControllerSettings.GetActivePresetIndex())
        {
            GetManager()->SetWidgetAsInvalidInput(mPresetNameLineEdit);
        }
        else
        {
            mPresetNameLineEdit->setStyleSheet("");
        }
    }


    GameControllerWindow::ParameterInfo* GameControllerWindow::FindParamInfoByAxisComboBox(QComboBox* comboBox)
    {
        // get the number of parameter infos and iterate through them
        const uint32 numParamInfos = mParameterInfos.GetLength();
        for (uint32 i = 0; i < numParamInfos; ++i)
        {
            if (mParameterInfos[i].mAxis == comboBox)
            {
                return &mParameterInfos[i];
            }
        }

        // return failure
        return nullptr;
    }


    void GameControllerWindow::OnAxisComboBox(int value)
    {
        MCORE_UNUSED(value);

        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        // get the active preset
        EMotionFX::AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetActivePreset();
        if (activePreset == nullptr)
        {
            return;
        }

        QComboBox* combo = qobject_cast<QComboBox*>(sender());
        ParameterInfo* paramInfo = FindParamInfoByAxisComboBox(combo);
        if (paramInfo == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* settingsInfo = activePreset->FindParameterInfo(paramInfo->mParameter->GetName().c_str());
        MCORE_ASSERT(settingsInfo);

    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        if (azrtti_istypeof<EMotionFX::FloatParameter>(paramInfo->mParameter))
        {
            const uint32 elementID = mGameController->FindElemendIDByName(FromQtString(combo->currentText()).c_str());
            if (elementID >= MCORE_INVALIDINDEX8)
            {
                settingsInfo->m_axis = MCORE_INVALIDINDEX8;
            }
            else
            {
                settingsInfo->m_axis = elementID;
            }
        }
        else
        if (azrtti_typeid(paramInfo->mParameter) == azrtti_typeid<EMotionFX::Vector2Parameter>())
        {
            if (value == 0)
            {
                settingsInfo->m_axis = MCORE_INVALIDINDEX8;
            }
            else
            {
                settingsInfo->m_axis = value - 1;
            }
        }
    #else
        settingsInfo->m_axis = MCORE_INVALIDINDEX8;
    #endif

        // update the interface
        UpdateParameterInterface(paramInfo);

        // update the parameter window
        mPlugin->GetParameterWindow()->Reinit(/*forceReinit*/true);
    }


    GameControllerWindow::ParameterInfo* GameControllerWindow::FindParamInfoByCheckBox(QCheckBox* checkBox)
    {
        // get the number of parameter infos and iterate through them
        const uint32 numParamInfos = mParameterInfos.GetLength();
        for (uint32 i = 0; i < numParamInfos; ++i)
        {
            if (mParameterInfos[i].mInvert == checkBox)
            {
                return &mParameterInfos[i];
            }
        }

        // return failure
        return nullptr;
    }


    void GameControllerWindow::OnInvertCheckBoxChanged(int state)
    {
        MCORE_UNUSED(state);

        // get the game controller settings from the current anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        // get the active preset
        EMotionFX::AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetActivePreset();
        if (activePreset == nullptr)
        {
            return;
        }

        QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender());
        ParameterInfo* paramInfo = FindParamInfoByCheckBox(checkBox);
        if (paramInfo == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* settingsInfo = activePreset->FindParameterInfo(paramInfo->mParameter->GetName().c_str());
        MCORE_ASSERT(settingsInfo);
        settingsInfo->m_invert = checkBox->checkState() == Qt::Checked ? true : false;
    }


    // new hardware got detected, reinit direct input
    void GameControllerWindow::HardwareChangeDetected()
    {
        // in case there is no controller plugged in watch out for a new one
        InitGameController();
        UpdateGameControllerComboBox();
        AutoSelectGameController();
        ReInit();
        mPlugin->GetParameterWindow()->Reinit(/*forceReinit*/true);
    }


    // handle timer event
    void GameControllerWindow::timerEvent(QTimerEvent* event)
    {
    #if !AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        MCORE_UNUSED(event);
    #endif

        if (EMotionFX::GetRecorder().GetIsInPlayMode() && EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            return;
        }

        // update the game controller
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        mGameController->Update();

        // check if the game controller is usable and if we have actually checked it in the combobox, if not return directly
        if (mGameController->GetIsValid() == false || mGameControllerComboBox->currentIndex() == 0)
        {
            return;
        }
    #else
        return;
    #endif

    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        // get the selected actor instance
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            return;
        }

        // get the anim graph instance for the selected actor instance
        EMotionFX::AnimGraphInstance* animGraphInstance = nullptr;
        animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            if (animGraphInstance->GetAnimGraph() != mAnimGraph) // if the selected anim graph instance isn't equal to the one of the actor instance
            {
                return;
            }
        }
        else
        {
            return;
        }

        // get the game controller settings from the anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = mAnimGraph->GetGameControllerSettings();

        // get the active preset
        EMotionFX::AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetActivePreset();
        if (activePreset == nullptr)
        {
            return;
        }

        const float timeDelta = mDeltaTimer.StampAndGetDeltaTimeInSeconds();

        // get the number of parameters and iterate through them
        const EMotionFX::ValueParameterVector& valueParameters = mAnimGraph->RecursivelyGetValueParameters();
        const size_t valueParametersCount = valueParameters.size();
        for (size_t parameterIndex = 0; parameterIndex < valueParametersCount; ++parameterIndex)
        {
            // get the game controller settings info for the given parameter
            const EMotionFX::ValueParameter* valueParameter = valueParameters[parameterIndex];
            EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* settingsInfo = activePreset->FindParameterInfo(valueParameter->GetName().c_str());
            MCORE_ASSERT(settingsInfo);

            // skip all parameters whose axis is set to None
            if (settingsInfo->m_axis == MCORE_INVALIDINDEX8)
            {
                continue;
            }

            // find the corresponding attribute
            MCore::Attribute* attribute = animGraphInstance->GetParameterValue(parameterIndex);

            if (attribute->GetType() == MCore::AttributeFloat::TYPE_ID)
            {
                // get the current value from the game controller
                float value = mGameController->GetValue(settingsInfo->m_axis);
                const EMotionFX::FloatParameter* floatParameter = static_cast<const EMotionFX::FloatParameter*>(valueParameter);
                const float minValue = floatParameter->GetMinValue();
                const float maxValue = floatParameter->GetMaxValue();

                switch (settingsInfo->m_mode)
                {
                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_STANDARD:
                {
                    if (settingsInfo->m_invert)
                    {
                        value = -value;
                    }

                    break;
                }

                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_ZEROTOONE:
                {
                    const float normalizedValue = aznumeric_cast<float>((value + 1.0) * 0.5f);
                    value = normalizedValue;

                    if (settingsInfo->m_invert)
                    {
                        value = 1.0f - value;
                    }

                    break;
                }

                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_PARAMRANGE:
                {
                    float normalizedValue = aznumeric_cast<float>((value + 1.0) * 0.5f);
                    if (settingsInfo->m_invert)
                    {
                        normalizedValue = 1.0f - normalizedValue;
                    }

                    value = minValue + normalizedValue * (maxValue - minValue);

                    break;
                }

                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_POSITIVETOPARAMRANGE:
                {
                    if (value < 0.0f)
                    {
                        break;
                    }

                    if (settingsInfo->m_invert)
                    {
                        value = -value;
                    }
                    value = minValue + value * (maxValue - minValue);

                    break;
                }

                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_NEGATIVETOPARAMRANGE:
                {
                    if (value > 0.0f)
                    {
                        break;
                    }

                    if (settingsInfo->m_invert)
                    {
                        value = -value;
                    }
                    value = minValue + value * (maxValue - minValue);

                    break;
                }

                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_ROTATE_CHARACTER:
                {
                    if (settingsInfo->m_invert)
                    {
                        value = -value;
                    }

                    if (value > 0.1f || value < -0.1f)
                    {
                        // only process in case the parameter info is enabled
                        if (settingsInfo->m_enabled)
                        {
                            AZ::Quaternion localRot = actorInstance->GetLocalSpaceTransform().mRotation;
                            localRot = localRot * MCore::CreateFromAxisAndAngle(AZ::Vector3(0.0f, 0.0f, 1.0f), value * timeDelta * 3.0f);
                            actorInstance->SetLocalSpaceRotation(localRot);
                        }
                    }

                    break;
                }
                }

                // set the value to the attribute in case the parameter info is enabled
                if (settingsInfo->m_enabled)
                {
                    static_cast<MCore::AttributeFloat*>(attribute)->SetValue(value);
                }

                // check if we also need to update the attribute widget in the parameter window
                if (event->timerId() == mInterfaceTimerID)
                {
                    // find the corresponding attribute widget and set the value in case the parameter info is enabled
                    if (settingsInfo->m_enabled)
                    {
                        mPlugin->GetParameterWindow()->UpdateParameterValue(valueParameter);
                    }

                    // also update the preview value in the game controller window
                    ParameterInfo* interfaceParamInfo = FindButtonInfoByAttributeInfo(valueParameter);
                    if (interfaceParamInfo)
                    {
                        mString = AZStd::string::format("%.2f", value);
                        interfaceParamInfo->mValue->setText(mString.c_str());
                    }
                }
            } // if it's a float attribute
            else if (azrtti_typeid(valueParameter) == azrtti_typeid<EMotionFX::Vector2Parameter>())
            {
                // get the current value from the game controller
                AZ::Vector2 value(0.0f, 0.0f);
                if (settingsInfo->m_axis == 0)
                {
                    value.SetX(mGameController->GetValue(GameController::ELEM_POS_X));
                    value.SetY(mGameController->GetValue(GameController::ELEM_POS_Y));
                }
                else
                {
                    value.SetX(mGameController->GetValue(GameController::ELEM_ROT_X));
                    value.SetY(mGameController->GetValue(GameController::ELEM_ROT_Y));
                }

                const EMotionFX::Vector2Parameter* vector2Parameter = static_cast<const EMotionFX::Vector2Parameter*>(valueParameter);
                const AZ::Vector2 minValue = vector2Parameter->GetMinValue();
                const AZ::Vector2 maxValue = vector2Parameter->GetMaxValue();

                switch (settingsInfo->m_mode)
                {
                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_STANDARD:
                {
                    if (settingsInfo->m_invert)
                    {
                        value = -value;
                    }

                    break;
                }

                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_ZEROTOONE:
                {
                    const float normalizedValueX = aznumeric_cast<float>((value.GetX() + 1.0) * 0.5f);
                    value.SetX(normalizedValueX);

                    const float normalizedValueY = aznumeric_cast<float>((value.GetY() + 1.0) * 0.5f);
                    value.SetY(normalizedValueY);

                    if (settingsInfo->m_invert)
                    {
                        value.SetX(1.0f - value.GetX());
                        value.SetY(1.0f - value.GetY());
                    }

                    break;
                }

                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_PARAMRANGE:
                {
                    float normalizedValueX = aznumeric_cast<float>((value.GetX() + 1.0) * 0.5f);
                    float normalizedValueY = aznumeric_cast<float>((value.GetY() + 1.0) * 0.5f);
                    if (settingsInfo->m_invert)
                    {
                        normalizedValueX = 1.0f - normalizedValueX;
                        normalizedValueY = 1.0f - normalizedValueY;
                    }

                    value.SetX(minValue.GetX() + normalizedValueX * (maxValue.GetX() - minValue.GetX()));
                    value.SetY(minValue.GetY() + normalizedValueY * (maxValue.GetY() - minValue.GetY()));

                    break;
                }

                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_POSITIVETOPARAMRANGE:
                {
                    if (value.GetX() > 0.0f)
                    {
                        if (settingsInfo->m_invert)
                        {
                            value.SetX(-value.GetX());
                        }
                        value.SetX(minValue.GetX() + value.GetX() * (maxValue.GetX() - minValue.GetX()));
                    }

                    if (value.GetY() > 0.0f)
                    {
                        if (settingsInfo->m_invert)
                        {
                            value.SetY(-value.GetY());
                        }
                        value.SetY(minValue.GetY() + value.GetY() * (maxValue.GetY() - minValue.GetY()));
                    }

                    break;
                }

                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_NEGATIVETOPARAMRANGE:
                {
                    if (value.GetX() < 0.0f)
                    {
                        if (settingsInfo->m_invert)
                        {
                            value.SetX(-value.GetX());
                        }
                        value.SetX(minValue.GetX() + value.GetX() * (maxValue.GetX() - minValue.GetX()));
                    }

                    if (value.GetY() < 0.0f)
                    {
                        if (settingsInfo->m_invert)
                        {
                            value.SetY(-value.GetY());
                        }
                        value.SetY(minValue.GetY() + value.GetY() * (maxValue.GetY() - minValue.GetY()));
                    }

                    break;
                }

                case EMotionFX::AnimGraphGameControllerSettings::PARAMMODE_ROTATE_CHARACTER:
                {
                    if (settingsInfo->m_invert)
                    {
                        value = -value;
                    }

                    if (value.GetX() > 0.1f || value.GetX() < -0.1f)
                    {
                        // only process in case the parameter info is enabled
                        if (settingsInfo->m_enabled)
                        {
                            AZ::Quaternion localRot = actorInstance->GetLocalSpaceTransform().mRotation;
                            localRot = localRot * MCore::CreateFromAxisAndAngle(AZ::Vector3(0.0f, 0.0f, 1.0f), value.GetX() * timeDelta * 3.0f);
                            actorInstance->SetLocalSpaceRotation(localRot);
                        }
                    }

                    break;
                }
                default:
                    MCORE_ASSERT(false);
                }

                // set the value to the attribute in case the parameter info is enabled
                if (settingsInfo->m_enabled)
                {
                    static_cast<MCore::AttributeVector2*>(attribute)->SetValue(value);
                }

                // check if we also need to update the attribute widget in the parameter window
                if (event->timerId() == mInterfaceTimerID)
                {
                    // find the corresponding attribute widget and set the value in case the parameter info is enabled
                    if (settingsInfo->m_enabled)
                    {
                        mPlugin->GetParameterWindow()->UpdateParameterValue(valueParameter);
                    }

                    // also update the preview value in the game controller window
                    ParameterInfo* interfaceParamInfo = FindButtonInfoByAttributeInfo(valueParameter);
                    if (interfaceParamInfo)
                    {
                        mString = AZStd::string::format("%.2f, %.2f", value.GetX(), value.GetY());
                        interfaceParamInfo->mValue->setText(mString.c_str());
                    }
                }
            } // if it's a vector2 attribute
        } // for all parameters

        // update the buttons
        const uint32 numButtons = mGameController->GetNumButtons();
        for (uint32 i = 0; i < numButtons; ++i)
        {
            const bool isPressed = mGameController->GetIsButtonPressed(i);

            // get the game controller settings info for the given button
            EMotionFX::AnimGraphGameControllerSettings::ButtonInfo* settingsInfo = activePreset->FindButtonInfo(i);
            MCORE_ASSERT(settingsInfo);

            if (settingsInfo->m_string.empty())
            {
                continue;
            }

            // skip this button in case control is disabled
            if (settingsInfo->m_enabled == false)
            {
                continue;
            }

            // Find the corresponding value parameter.
            const AZ::Outcome<size_t> parameterIndex = mAnimGraph->FindValueParameterIndexByName(settingsInfo->m_string);

            MCore::AttributeBool* boolAttribute = nullptr;
            if (parameterIndex.IsSuccess())
            {
                MCore::Attribute* attribute = animGraphInstance->GetParameterValue(static_cast<uint32>(parameterIndex.GetValue()));

                if (attribute->GetType() == MCore::AttributeBool::TYPE_ID)
                {
                    boolAttribute = static_cast<MCore::AttributeBool*>(attribute);
                }
            }

            switch (settingsInfo->m_mode)
            {
            case EMotionFX::AnimGraphGameControllerSettings::BUTTONMODE_NONE:
                break;

            case EMotionFX::AnimGraphGameControllerSettings::BUTTONMODE_SWITCHSTATE:
            {
                if (isPressed)
                {
                    // switch to the desired state
                    if (animGraphInstance)
                    {
                        animGraphInstance->TransitionToState(settingsInfo->m_string.c_str());
                    }
                }
                break;
            }

            case EMotionFX::AnimGraphGameControllerSettings::BUTTONMODE_TOGGLEBOOLEANPARAMETER:
            {
                if (boolAttribute)
                {
                    const bool oldValue = boolAttribute->GetValue();

                    if (isPressed && settingsInfo->m_oldIsPressed == false)
                    {
                        boolAttribute->SetValue((!oldValue) ? true : false);
                    }

                    // check if we also need to update the attribute widget in the parameter window
                    if (event->timerId() == mInterfaceTimerID)
                    {
                        const EMotionFX::ValueParameter* valueParameter = mAnimGraph->FindValueParameter(parameterIndex.GetValue());
                        mPlugin->GetParameterWindow()->UpdateParameterValue(valueParameter);
                    }
                }

                break;
            }

            case EMotionFX::AnimGraphGameControllerSettings::BUTTONMODE_ENABLEBOOLWHILEPRESSED:
            {
                if (boolAttribute)
                {
                    boolAttribute->SetValue(isPressed ? true : false);

                    // check if we also need to update the attribute widget in the parameter window
                    if (event->timerId() == mInterfaceTimerID)
                    {
                        const EMotionFX::ValueParameter* valueParameter = mAnimGraph->FindValueParameter(parameterIndex.GetValue());
                        mPlugin->GetParameterWindow()->UpdateParameterValue(valueParameter);
                    }
                }

                break;
            }

            case EMotionFX::AnimGraphGameControllerSettings::BUTTONMODE_DISABLEBOOLWHILEPRESSED:
            {
                if (boolAttribute)
                {
                    boolAttribute->SetValue((!isPressed) ? true : false);

                    // check if we also need to update the attribute widget in the parameter window
                    if (event->timerId() == mInterfaceTimerID)
                    {
                        const EMotionFX::ValueParameter* valueParameter = mAnimGraph->FindValueParameter(parameterIndex.GetValue());
                        mPlugin->GetParameterWindow()->UpdateParameterValue(valueParameter);
                    }
                }

                break;
            }

            case EMotionFX::AnimGraphGameControllerSettings::BUTTONMODE_ENABLEBOOLFORONLYONEFRAMEONLY:
            {
                if (boolAttribute)
                {
                    // in case the button got pressed and we are allowed to set it to true, do that for only one frame
                    static bool isAllowed = true;
                    if (isPressed && isAllowed)
                    {
                        // set the bool parameter to true this time
                        boolAttribute->SetValue(true);

                        // don't allow to set the boolean parameter to true next frame
                        isAllowed = false;
                    }
                    else
                    {
                        // disable the boolean parameter as either the button is not pressed or we are not allowed to enable it as that single frame tick already happened
                        boolAttribute->SetValue(false);

                        // allow it again as soon as the user left the button
                        if (isPressed == false)
                        {
                            isAllowed = true;
                        }
                    }

                    // check if we also need to update the attribute widget in the parameter window
                    if (event->timerId() == mInterfaceTimerID)
                    {
                        const EMotionFX::ValueParameter* valueParameter = mAnimGraph->FindValueParameter(parameterIndex.GetValue());
                        mPlugin->GetParameterWindow()->UpdateParameterValue(valueParameter);
                    }
                }

                break;
            }
            }

            // store the information about the button press for the next frame
            settingsInfo->m_oldIsPressed = isPressed;
        }

        // check if the interface timer is ticking
        if (event->timerId() == mInterfaceTimerID)
        {
            // update the interface elements
            for (uint32 i = 0; i < GameController::NUM_ELEMENTS; ++i)
            {
                if (mGameController->GetIsPresent(i))
                {
                    const float value = mGameController->GetValue(i);
                    if (value > 1000.0f)
                    {
                        mString.clear();
                    }
                    else
                    {
                        mString = AZStd::string::format("%.2f", value);
                    }

                    mPreviewLabels[i]->setText(mString.c_str());
                }
            }

            // update the active button string
            mString.clear();
            for (uint32 i = 0; i < numButtons; ++i)
            {
                if (mGameController->GetIsButtonPressed(i))
                {
                    mString += AZStd::string::format("%s%d ", (i < 10) ? "0" : "", i);
                }
            }
            if (mString.size() == 0)
            {
                mPreviewLabels[GameController::NUM_ELEMENTS]->setText(" ");
            }
            else
            {
                mPreviewLabels[GameController::NUM_ELEMENTS]->setText(mString.c_str());
            }
        }
    #endif
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // CreateParameter callback
    //----------------------------------------------------------------------------------------------------------------------------------
    void ReInitGameControllerWindow()
    {
        // get the plugin object
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return;
        }

        // re-init the param window
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        animGraphPlugin->GetGameControllerWindow()->ReInit();
    #endif
    }


    bool GameControllerWindow::CommandCreateBlendParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); ReInitGameControllerWindow(); return true; }
    bool GameControllerWindow::CommandCreateBlendParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); ReInitGameControllerWindow(); return true; }
    bool GameControllerWindow::CommandRemoveBlendParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); ReInitGameControllerWindow(); return true; }
    bool GameControllerWindow::CommandRemoveBlendParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); ReInitGameControllerWindow(); return true; }
    bool GameControllerWindow::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        ReInitGameControllerWindow();
        return true;
    }
    bool GameControllerWindow::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        ReInitGameControllerWindow();
        return true;
    }
    bool GameControllerWindow::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        ReInitGameControllerWindow();
        return true;
    }
    bool GameControllerWindow::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        ReInitGameControllerWindow();
        return true;
    }
    bool GameControllerWindow::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)           { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); ReInitGameControllerWindow(); return true; }
    bool GameControllerWindow::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); ReInitGameControllerWindow(); return true; }

    bool GameControllerWindow::CommandAdjustBlendParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        // get the plugin object
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        uint32                 animGraphID    = commandLine.GetValueAsInt("animGraphID", command);
        EMotionFX::AnimGraph*  animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);

        if (animGraph == nullptr)
        {
            MCore::LogError("Cannot adjust parameter to anim graph. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        // get the game controller settings from the anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = animGraph->GetGameControllerSettings();

        AZStd::string name;
        commandLine.GetValue("name", command, &name);

        AZStd::string newName;
        commandLine.GetValue("newName", command, &newName);

        gameControllerSettings.OnParameterNameChange(name.c_str(), newName.c_str());

        ReInitGameControllerWindow();
        return true;
    }


    bool GameControllerWindow::CommandAdjustBlendParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        // get the plugin object
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        //AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        const uint32                animGraphID    = commandLine.GetValueAsInt("animGraphID", command);
        EMotionFX::AnimGraph*      animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            MCore::LogError("Cannot adjust parameter to anim graph. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        // get the game controller settings from the anim graph
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = animGraph->GetGameControllerSettings();

        AZStd::string name;
        commandLine.GetValue("name", command, &name);

        AZStd::string newName;
        commandLine.GetValue("newName", command, &newName);

        gameControllerSettings.OnParameterNameChange(newName.c_str(), name.c_str());

        ReInitGameControllerWindow();
        return true;
    }
} // namespace EMStudio
