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

#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TrackDataWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TrackHeaderWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewToolBar.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeTrack.h>
#include <QComboBox>
#include <QPaintEvent>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <MysticQt/Source/MysticQtConfig.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/Recorder.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <AzQtComponents/Components/Widgets/CheckBox.h>


namespace EMStudio
{
    // constructor
    TrackHeaderWidget::TrackHeaderWidget(TimeViewPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        mPlugin                     = plugin;
        mTrackLayout                = nullptr;
        mTrackWidget                = nullptr;
        mStackWidget                = nullptr;
        mNodeNamesCheckBox          = nullptr;
        mMotionFilesCheckBox        = nullptr;

        // create the main layout
        mMainLayout = new QVBoxLayout();
        mMainLayout->setMargin(2);
        mMainLayout->setSpacing(0);
        mMainLayout->setAlignment(Qt::AlignTop);

        ////////////////////////////////
        // Create the add event button
        ////////////////////////////////
        QWidget* mainAddWidget = new QWidget();
        QHBoxLayout* mainAddWidgetLayout = new QHBoxLayout();
        mainAddWidgetLayout->setContentsMargins(6, 0, 3, 0);
        mainAddWidget->setLayout(mainAddWidgetLayout);
        QLabel* label = new QLabel(tr("Add event track"));
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        mainAddWidgetLayout->addWidget(label);

        QToolButton* addButton = new QToolButton();
        addButton->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Plus.svg"));
        addButton->setToolTip("Add a new event track");
        connect(addButton, &QToolButton::clicked, this, &TrackHeaderWidget::OnAddTrackButtonClicked);
        mainAddWidgetLayout->addWidget(addButton);

        mainAddWidget->setFixedSize(175, 40);
        mainAddWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        mMainLayout->addWidget(mainAddWidget);
        mAddTrackWidget = mainAddWidget;
        ////////////////////////////////

        // recorder settings
        mStackWidget = new MysticQt::DialogStack();

        //-----------

        QWidget* contentsWidget = new QWidget();
        QVBoxLayout* contentsLayout = new QVBoxLayout();
        contentsLayout->setSpacing(1);
        contentsLayout->setMargin(0);
        contentsWidget->setLayout(contentsLayout);

        mNodeNamesCheckBox = new QCheckBox("Show Node Names");
        mNodeNamesCheckBox->setChecked(true);
        mNodeNamesCheckBox->setCheckable(true);
        AzQtComponents::CheckBox::applyToggleSwitchStyle(mNodeNamesCheckBox);
        connect(mNodeNamesCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        contentsLayout->addWidget(mNodeNamesCheckBox);

        mMotionFilesCheckBox = new QCheckBox("Show Motion Files");
        mMotionFilesCheckBox->setChecked(false);
        mMotionFilesCheckBox->setCheckable(true);
        AzQtComponents::CheckBox::applyToggleSwitchStyle(mMotionFilesCheckBox);
        connect(mMotionFilesCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        contentsLayout->addWidget(mMotionFilesCheckBox);

        QHBoxLayout* comboLayout = new QHBoxLayout();
        comboLayout->addWidget(new QLabel("Nodes:"));
        mNodeContentsComboBox = new QComboBox();
        mNodeContentsComboBox->setEditable(false);
        mNodeContentsComboBox->addItem("Global Weights");
        mNodeContentsComboBox->addItem("Local Weights");
        mNodeContentsComboBox->addItem("Local Time");
        mNodeContentsComboBox->setCurrentIndex(0);
        connect(mNodeContentsComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &TrackHeaderWidget::OnComboBoxIndexChanged);
        comboLayout->addWidget(mNodeContentsComboBox);
        contentsLayout->addLayout(comboLayout);

        comboLayout = new QHBoxLayout();
        comboLayout->addWidget(new QLabel("Graph:"));
        mGraphContentsComboBox = new QComboBox();
        mGraphContentsComboBox->setEditable(false);
        mGraphContentsComboBox->addItem("Global Weights");
        mGraphContentsComboBox->addItem("Local Weights");
        mGraphContentsComboBox->addItem("Local Time");
        mGraphContentsComboBox->setCurrentIndex(0);
        connect(mGraphContentsComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &TrackHeaderWidget::OnComboBoxIndexChanged);
        comboLayout->addWidget(mGraphContentsComboBox);
        contentsLayout->addLayout(comboLayout);

        mStackWidget->Add(contentsWidget, "Contents", false, false, true);

        //-----------

        mMainLayout->addWidget(mStackWidget);
        setFocusPolicy(Qt::StrongFocus);
        setLayout(mMainLayout);

        ReInit();
    }


    // destructor
    TrackHeaderWidget::~TrackHeaderWidget()
    {
    }


    void TrackHeaderWidget::ReInit()
    {
        mPlugin->SetRedrawFlag();

        if (mTrackWidget)
        {
            mTrackWidget->hide();
            mMainLayout->removeWidget(mTrackWidget);
            mTrackWidget->deleteLater(); // TODO: this causes flickering, but normal deletion will make it crash
        }

        mTrackWidget = nullptr;

        // If we are in anim graph mode and have a recording, don't init for the motions.
        if ((mPlugin->GetMode() != TimeViewMode::Motion) &&
            (EMotionFX::GetRecorder().GetIsRecording() || EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon || EMotionFX::GetRecorder().GetIsInPlayMode() || !mPlugin->mMotion))
        {
            mAddTrackWidget->setVisible(false);
            setVisible(false);

            if ((mPlugin->GetMode() != TimeViewMode::Motion) &&
                (EMotionFX::GetRecorder().GetIsRecording() || EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon))
            {
                mStackWidget->setVisible(true);
            }
            else
            {
                mStackWidget->setVisible(false);
            }

            return;
        }

        mAddTrackWidget->setVisible(true);
        setVisible(true);
        mStackWidget->setVisible(false);

        const uint32 numTracks = mPlugin->mTracks.GetLength();
        if (numTracks == 0)
        {
            return;
        }

        mTrackWidget = new QWidget();
        mTrackLayout = new QVBoxLayout();
        mTrackLayout->setMargin(0);
        mTrackLayout->setSpacing(1);

        for (uint32 i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = mPlugin->mTracks[i];

            if (track->GetIsVisible() == false)
            {
                continue;
            }

            HeaderTrackWidget* widget = new HeaderTrackWidget(mTrackWidget, mPlugin, this, track, i);

            connect(widget, &HeaderTrackWidget::TrackNameChanged, this, &TrackHeaderWidget::OnTrackNameChanged);
            connect(widget, &HeaderTrackWidget::EnabledStateChanged, this, &TrackHeaderWidget::OnTrackEnabledStateChanged);

            mTrackLayout->addWidget(widget);
        }

        mTrackWidget->setLayout(mTrackLayout);
        mMainLayout->addWidget(mTrackWidget);
    }


    HeaderTrackWidget::HeaderTrackWidget(QWidget* parent, TimeViewPlugin* parentPlugin, TrackHeaderWidget* trackHeaderWidget, TimeTrack* timeTrack, uint32 trackIndex)
        : QWidget(parent)
    {
        mPlugin = parentPlugin;

        QHBoxLayout* mainLayout = new QHBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setSpacing(0);

        mHeaderTrackWidget  = trackHeaderWidget;
        mEnabledCheckbox    = new QCheckBox();
        mNameLabel          = new QLabel(timeTrack->GetName());
        mNameEdit           = new QLineEdit(timeTrack->GetName());
        mTrack              = timeTrack;
        mTrackIndex         = trackIndex;

        mNameEdit->setVisible(false);
        mNameEdit->setFrame(false);

        mEnabledCheckbox->setFixedWidth(36);
        AzQtComponents::CheckBox::applyToggleSwitchStyle(mEnabledCheckbox);

        if (timeTrack->GetIsEnabled())
        {
            mEnabledCheckbox->setCheckState(Qt::Checked);
        }
        else
        {
            mNameEdit->setStyleSheet("background-color: rgb(70, 70, 70);");
            mEnabledCheckbox->setCheckState(Qt::Unchecked);
        }

        if (timeTrack->GetIsDeletable() == false)
        {
            mNameEdit->setReadOnly(true);
            mEnabledCheckbox->setEnabled(false);
        }
        else
        {
            mNameLabel->installEventFilter(this);
        }

        connect(mNameEdit, &QLineEdit::editingFinished, this, &HeaderTrackWidget::NameChanged);
        connect(mNameEdit, &QLineEdit::textEdited, this, &HeaderTrackWidget::NameEdited);
        connect(mEnabledCheckbox, &QCheckBox::stateChanged, this, &HeaderTrackWidget::EnabledCheckBoxChanged);

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos)
        {
            QMenu m;
            auto action = m.addAction(tr("Remove track"), this, [=]
            {
                mPlugin->GetTrackDataWidget()->RemoveTrack(mTrackIndex);
            });
            action->setEnabled(mTrack->GetIsDeletable());
            m.exec(mapToGlobal(pos));
        });

        mainLayout->insertSpacing(0, 4);
        mainLayout->addWidget(mNameLabel);
        mainLayout->addWidget(mNameEdit);
        mainLayout->addWidget(mEnabledCheckbox);
        mainLayout->insertSpacing(5, 2);

        //mainLayout->setAlignment(mEnabledCheckbox, Qt::AlignLeft);

        setLayout(mainLayout);

        setMinimumHeight(20);
        setMaximumHeight(20);
    }

    bool HeaderTrackWidget::eventFilter(QObject* object, QEvent* event)
    {
        if (object == mNameLabel && event->type() == QEvent::MouseButtonDblClick)
        {
            mNameLabel->setVisible(false);
            mNameEdit->setVisible(true);
            mNameEdit->selectAll();
            mNameEdit->setFocus();
        }
        return QWidget::eventFilter(object, event);
    }

    void HeaderTrackWidget::NameChanged()
    {
        MCORE_ASSERT(sender()->inherits("QLineEdit"));
        mPlugin->SetRedrawFlag();
        QLineEdit* widget = qobject_cast<QLineEdit*>(sender());
        mNameLabel->setVisible(true);
        mNameEdit->setVisible(false);
        if (ValidateName())
        {
            mNameLabel->setText(widget->text());
            emit TrackNameChanged(widget->text(), mTrackIndex);
        }
    }

    bool HeaderTrackWidget::ValidateName()
    {
        AZStd::string name = mNameEdit->text().toUtf8().data();

        bool nameUnique = true;
        const uint32 numTracks = mPlugin->GetNumTracks();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = mPlugin->GetTrack(i);

            if (mTrack != track)
            {
                if (name == track->GetName())
                {
                    nameUnique = false;
                    break;
                }
            }
        }

        return nameUnique;
    }


    void HeaderTrackWidget::NameEdited(const QString& text)
    {
        MCORE_UNUSED(text);
        mPlugin->SetRedrawFlag();
        if (ValidateName() == false)
        {
            GetManager()->SetWidgetAsInvalidInput(mNameEdit);
        }
        else
        {
            mNameEdit->setStyleSheet("");
        }
    }


    void HeaderTrackWidget::EnabledCheckBoxChanged(int state)
    {
        mPlugin->SetRedrawFlag();

        bool enabled = false;
        if (state == Qt::Checked)
        {
            enabled = true;
        }

        emit EnabledStateChanged(enabled, mTrackIndex);
    }


    // propagate key events to the plugin and let it handle by a shared function
    void HeaderTrackWidget::keyPressEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void HeaderTrackWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyReleaseEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackHeaderWidget::keyPressEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackHeaderWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyReleaseEvent(event);
        }
    }


    // detailed nodes checkbox hit
    void TrackHeaderWidget::OnDetailedNodesCheckBox(int state)
    {
        MCORE_UNUSED(state);
        mPlugin->SetRedrawFlag();

        RecorderGroup* recorderGroup = mPlugin->GetTimeViewToolBar()->GetRecorderGroup();
        if (!recorderGroup->GetDetailedNodes())
        {
            mPlugin->mTrackDataWidget->mNodeHistoryItemHeight = 20;
        }
        else
        {
            mPlugin->mTrackDataWidget->mNodeHistoryItemHeight = 35;
        }
    }


    // a checkbox state changed
    void TrackHeaderWidget::OnCheckBox(int state)
    {
        MCORE_UNUSED(state);
        mPlugin->SetRedrawFlag();
    }


    // a combobox index changed
    void TrackHeaderWidget::OnComboBoxIndexChanged(int state)
    {
        MCORE_UNUSED(state);
        mPlugin->SetRedrawFlag();
    }
} // namespace EMStudio
