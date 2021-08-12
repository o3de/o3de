/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        m_plugin                     = plugin;
        m_trackLayout                = nullptr;
        m_trackWidget                = nullptr;
        m_stackWidget                = nullptr;
        m_nodeNamesCheckBox          = nullptr;
        m_motionFilesCheckBox        = nullptr;

        // create the main layout
        m_mainLayout = new QVBoxLayout();
        m_mainLayout->setMargin(2);
        m_mainLayout->setSpacing(0);
        m_mainLayout->setAlignment(Qt::AlignTop);

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
        addButton->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.svg"));
        addButton->setToolTip("Add a new event track");
        connect(addButton, &QToolButton::clicked, this, &TrackHeaderWidget::OnAddTrackButtonClicked);
        mainAddWidgetLayout->addWidget(addButton);

        mainAddWidget->setFixedSize(175, 40);
        mainAddWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_mainLayout->addWidget(mainAddWidget);
        m_addTrackWidget = mainAddWidget;
        ////////////////////////////////

        // recorder settings
        m_stackWidget = new MysticQt::DialogStack();

        //-----------

        QWidget* contentsWidget = new QWidget();
        QVBoxLayout* contentsLayout = new QVBoxLayout();
        contentsLayout->setSpacing(1);
        contentsLayout->setMargin(0);
        contentsWidget->setLayout(contentsLayout);

        m_nodeNamesCheckBox = new QCheckBox("Show Node Names");
        m_nodeNamesCheckBox->setChecked(true);
        m_nodeNamesCheckBox->setCheckable(true);
        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_nodeNamesCheckBox);
        connect(m_nodeNamesCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        contentsLayout->addWidget(m_nodeNamesCheckBox);

        m_motionFilesCheckBox = new QCheckBox("Show Motion Files");
        m_motionFilesCheckBox->setChecked(false);
        m_motionFilesCheckBox->setCheckable(true);
        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_motionFilesCheckBox);
        connect(m_motionFilesCheckBox, &QCheckBox::stateChanged, this, &TrackHeaderWidget::OnCheckBox);
        contentsLayout->addWidget(m_motionFilesCheckBox);

        QHBoxLayout* comboLayout = new QHBoxLayout();
        comboLayout->addWidget(new QLabel("Nodes:"));
        m_nodeContentsComboBox = new QComboBox();
        m_nodeContentsComboBox->setEditable(false);
        m_nodeContentsComboBox->addItem("Global Weights");
        m_nodeContentsComboBox->addItem("Local Weights");
        m_nodeContentsComboBox->addItem("Local Time");
        m_nodeContentsComboBox->setCurrentIndex(0);
        connect(m_nodeContentsComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &TrackHeaderWidget::OnComboBoxIndexChanged);
        comboLayout->addWidget(m_nodeContentsComboBox);
        contentsLayout->addLayout(comboLayout);

        comboLayout = new QHBoxLayout();
        comboLayout->addWidget(new QLabel("Graph:"));
        m_graphContentsComboBox = new QComboBox();
        m_graphContentsComboBox->setEditable(false);
        m_graphContentsComboBox->addItem("Global Weights");
        m_graphContentsComboBox->addItem("Local Weights");
        m_graphContentsComboBox->addItem("Local Time");
        m_graphContentsComboBox->setCurrentIndex(0);
        connect(m_graphContentsComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &TrackHeaderWidget::OnComboBoxIndexChanged);
        comboLayout->addWidget(m_graphContentsComboBox);
        contentsLayout->addLayout(comboLayout);

        m_stackWidget->Add(contentsWidget, "Contents", false, false, true);

        //-----------

        m_mainLayout->addWidget(m_stackWidget);
        setFocusPolicy(Qt::StrongFocus);
        setLayout(m_mainLayout);

        ReInit();
    }


    // destructor
    TrackHeaderWidget::~TrackHeaderWidget()
    {
    }


    void TrackHeaderWidget::ReInit()
    {
        m_plugin->SetRedrawFlag();

        if (m_trackWidget)
        {
            m_trackWidget->hide();
            m_mainLayout->removeWidget(m_trackWidget);
            m_trackWidget->deleteLater(); // TODO: this causes flickering, but normal deletion will make it crash
        }

        m_trackWidget = nullptr;

        // If we are in anim graph mode and have a recording, don't init for the motions.
        if ((m_plugin->GetMode() != TimeViewMode::Motion) &&
            (EMotionFX::GetRecorder().GetIsRecording() || EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon || EMotionFX::GetRecorder().GetIsInPlayMode() || !m_plugin->m_motion))
        {
            m_addTrackWidget->setVisible(false);
            setVisible(false);

            if ((m_plugin->GetMode() != TimeViewMode::Motion) &&
                (EMotionFX::GetRecorder().GetIsRecording() || EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon))
            {
                m_stackWidget->setVisible(true);
            }
            else
            {
                m_stackWidget->setVisible(false);
            }

            return;
        }

        m_addTrackWidget->setVisible(true);
        setVisible(true);
        m_stackWidget->setVisible(false);

        if (m_plugin->m_tracks.empty())
        {
            return;
        }

        m_trackWidget = new QWidget();
        m_trackLayout = new QVBoxLayout();
        m_trackLayout->setMargin(0);
        m_trackLayout->setSpacing(1);

        const size_t numTracks = m_plugin->m_tracks.size();
        for (size_t i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = m_plugin->m_tracks[i];

            if (track->GetIsVisible() == false)
            {
                continue;
            }

            HeaderTrackWidget* widget = new HeaderTrackWidget(m_trackWidget, m_plugin, this, track, i);

            connect(widget, &HeaderTrackWidget::TrackNameChanged, this, &TrackHeaderWidget::OnTrackNameChanged);
            connect(widget, &HeaderTrackWidget::EnabledStateChanged, this, &TrackHeaderWidget::OnTrackEnabledStateChanged);

            m_trackLayout->addWidget(widget);
        }

        m_trackWidget->setLayout(m_trackLayout);
        m_mainLayout->addWidget(m_trackWidget);
    }


    HeaderTrackWidget::HeaderTrackWidget(QWidget* parent, TimeViewPlugin* parentPlugin, TrackHeaderWidget* trackHeaderWidget, TimeTrack* timeTrack, size_t trackIndex)
        : QWidget(parent)
    {
        m_plugin = parentPlugin;

        QHBoxLayout* mainLayout = new QHBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setSpacing(0);

        m_headerTrackWidget  = trackHeaderWidget;
        m_enabledCheckbox    = new QCheckBox();
        m_nameLabel          = new QLabel(timeTrack->GetName());
        m_nameEdit           = new QLineEdit(timeTrack->GetName());
        m_track              = timeTrack;
        m_trackIndex         = trackIndex;

        m_nameEdit->setVisible(false);
        m_nameEdit->setFrame(false);

        m_enabledCheckbox->setFixedWidth(36);
        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_enabledCheckbox);

        if (timeTrack->GetIsEnabled())
        {
            m_enabledCheckbox->setCheckState(Qt::Checked);
        }
        else
        {
            m_nameEdit->setStyleSheet("background-color: rgb(70, 70, 70);");
            m_enabledCheckbox->setCheckState(Qt::Unchecked);
        }

        if (timeTrack->GetIsDeletable() == false)
        {
            m_nameEdit->setReadOnly(true);
            m_enabledCheckbox->setEnabled(false);
        }
        else
        {
            m_nameLabel->installEventFilter(this);
        }

        connect(m_nameEdit, &QLineEdit::editingFinished, this, &HeaderTrackWidget::NameChanged);
        connect(m_nameEdit, &QLineEdit::textEdited, this, &HeaderTrackWidget::NameEdited);
        connect(m_enabledCheckbox, &QCheckBox::stateChanged, this, &HeaderTrackWidget::EnabledCheckBoxChanged);

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos)
        {
            QMenu m;
            auto action = m.addAction(tr("Remove track"), this, [=]
            {
                m_plugin->GetTrackDataWidget()->RemoveTrack(m_trackIndex);
            });
            action->setEnabled(m_track->GetIsDeletable());
            m.exec(mapToGlobal(pos));
        });

        mainLayout->insertSpacing(0, 4);
        mainLayout->addWidget(m_nameLabel);
        mainLayout->addWidget(m_nameEdit);
        mainLayout->addWidget(m_enabledCheckbox);
        mainLayout->insertSpacing(5, 2);

        setLayout(mainLayout);

        setMinimumHeight(20);
        setMaximumHeight(20);
    }

    bool HeaderTrackWidget::eventFilter(QObject* object, QEvent* event)
    {
        if (object == m_nameLabel && event->type() == QEvent::MouseButtonDblClick)
        {
            m_nameLabel->setVisible(false);
            m_nameEdit->setVisible(true);
            m_nameEdit->selectAll();
            m_nameEdit->setFocus();
        }
        return QWidget::eventFilter(object, event);
    }

    void HeaderTrackWidget::NameChanged()
    {
        MCORE_ASSERT(sender()->inherits("QLineEdit"));
        m_plugin->SetRedrawFlag();
        QLineEdit* widget = qobject_cast<QLineEdit*>(sender());
        m_nameLabel->setVisible(true);
        m_nameEdit->setVisible(false);
        if (ValidateName())
        {
            m_nameLabel->setText(widget->text());
            emit TrackNameChanged(widget->text(), m_trackIndex);
        }
    }

    bool HeaderTrackWidget::ValidateName()
    {
        AZStd::string name = m_nameEdit->text().toUtf8().data();

        bool nameUnique = true;
        const size_t numTracks = m_plugin->GetNumTracks();
        for (size_t i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = m_plugin->GetTrack(i);

            if (m_track != track)
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
        m_plugin->SetRedrawFlag();
        if (ValidateName() == false)
        {
            GetManager()->SetWidgetAsInvalidInput(m_nameEdit);
        }
        else
        {
            m_nameEdit->setStyleSheet("");
        }
    }


    void HeaderTrackWidget::EnabledCheckBoxChanged(int state)
    {
        m_plugin->SetRedrawFlag();

        bool enabled = false;
        if (state == Qt::Checked)
        {
            enabled = true;
        }

        emit EnabledStateChanged(enabled, m_trackIndex);
    }


    // propagate key events to the plugin and let it handle by a shared function
    void HeaderTrackWidget::keyPressEvent(QKeyEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void HeaderTrackWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->OnKeyReleaseEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackHeaderWidget::keyPressEvent(QKeyEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackHeaderWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->OnKeyReleaseEvent(event);
        }
    }


    // detailed nodes checkbox hit
    void TrackHeaderWidget::OnDetailedNodesCheckBox(int state)
    {
        MCORE_UNUSED(state);
        m_plugin->SetRedrawFlag();

        RecorderGroup* recorderGroup = m_plugin->GetTimeViewToolBar()->GetRecorderGroup();
        if (!recorderGroup->GetDetailedNodes())
        {
            m_plugin->m_trackDataWidget->m_nodeHistoryItemHeight = 20;
        }
        else
        {
            m_plugin->m_trackDataWidget->m_nodeHistoryItemHeight = 35;
        }
    }


    // a checkbox state changed
    void TrackHeaderWidget::OnCheckBox(int state)
    {
        MCORE_UNUSED(state);
        m_plugin->SetRedrawFlag();
    }


    // a combobox index changed
    void TrackHeaderWidget::OnComboBoxIndexChanged(int state)
    {
        MCORE_UNUSED(state);
        m_plugin->SetRedrawFlag();
    }
} // namespace EMStudio
