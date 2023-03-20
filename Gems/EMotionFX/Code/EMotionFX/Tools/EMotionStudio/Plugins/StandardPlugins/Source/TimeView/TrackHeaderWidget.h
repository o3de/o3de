/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <MysticQt/Source/MysticQtConfig.h>
#include <MysticQt/Source/DialogStack.h>
#include "../StandardPluginsConfig.h"
#include <QWidget>
#include <QPen>
#include <QFont>
#endif

QT_FORWARD_DECLARE_CLASS(QBrush)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QComboBox)


namespace EMStudio
{
    // forward declarations
    class TrackDataHeaderWidget;
    class TargetDataWidget;
    class TimeViewPlugin;
    class TimeTrack;
    class TrackHeaderWidget;

    class HeaderTrackWidget
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(HeaderTrackWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        HeaderTrackWidget(QWidget* parent, TimeViewPlugin* parentPlugin, TrackHeaderWidget* trackHeaderWidget, TimeTrack* timeTrack, size_t trackIndex);

        QCheckBox*              m_enabledCheckbox;
        QLabel*                 m_nameLabel;
        QLineEdit*              m_nameEdit;
        QPushButton*            m_removeButton;
        TimeTrack*              m_track;
        size_t                  m_trackIndex;
        TrackHeaderWidget*      m_headerTrackWidget;
        TimeViewPlugin*         m_plugin;

        bool ValidateName();

        bool eventFilter(QObject* object, QEvent* event) override;

    signals:
        void TrackNameChanged(const QString& text, size_t trackNr);
        void EnabledStateChanged(bool checked, size_t trackNr);

    public slots:
        void NameChanged();
        void NameEdited(const QString& text);
        void EnabledCheckBoxChanged(int state);

        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
    };

    /**
     * The part left of the timeline and data.
     */
    class TrackHeaderWidget
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(TrackHeaderWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

        friend class TrackDataHeaderWidget;
        friend class TrackDataWidget;
        friend class TimeViewPlugin;
    public:
        TrackHeaderWidget(TimeViewPlugin* plugin, QWidget* parent = nullptr);
        ~TrackHeaderWidget();

        void ReInit();
        void UpdateDataContents();

        QWidget* GetAddTrackWidget()                                                                        { return m_addTrackWidget; }

    public slots:
        void OnAddTrackButtonClicked()                                                                      { CommandSystem::CommandAddEventTrack(); }
        void OnTrackNameChanged(const QString& text, size_t trackNr)                                           { CommandSystem::CommandRenameEventTrack(trackNr, FromQtString(text).c_str()); }
        void OnTrackEnabledStateChanged(bool enabled, size_t trackNr)                                          { CommandSystem::CommandEnableEventTrack(trackNr, enabled); }
        void OnDetailedNodesCheckBox(int state);
        void OnCheckBox(int state);
        void OnComboBoxIndexChanged(int state);

    private:
        TimeViewPlugin*                     m_plugin;
        QVBoxLayout*                        m_mainLayout;
        QWidget*                            m_trackWidget;
        QVBoxLayout*                        m_trackLayout;
        QWidget*                            m_addTrackWidget;
        QPushButton*                        m_addTrackButton;
        MysticQt::DialogStack*              m_stackWidget;
        QComboBox*                          m_graphContentsComboBox;
        QComboBox*                          m_nodeContentsComboBox;
        QCheckBox*                          m_nodeNamesCheckBox;
        QCheckBox*                          m_motionFilesCheckBox;

        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
    };
} // namespace EMStudio
