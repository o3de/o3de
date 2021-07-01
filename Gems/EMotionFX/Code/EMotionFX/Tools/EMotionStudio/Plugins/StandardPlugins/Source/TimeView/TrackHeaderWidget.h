/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        HeaderTrackWidget(QWidget* parent, TimeViewPlugin* parentPlugin, TrackHeaderWidget* trackHeaderWidget, TimeTrack* timeTrack, uint32 trackIndex);

        QCheckBox*              mEnabledCheckbox;
        QLabel*                 mNameLabel;
        QLineEdit*              mNameEdit;
        QPushButton*            mRemoveButton;
        TimeTrack*              mTrack;
        uint32                  mTrackIndex;
        TrackHeaderWidget*      mHeaderTrackWidget;
        TimeViewPlugin*         mPlugin;

        bool ValidateName();

        bool eventFilter(QObject* object, QEvent* event) override;

    signals:
        void TrackNameChanged(const QString& text, int trackNr);
        void EnabledStateChanged(bool checked, int trackNr);

    public slots:
        void NameChanged();
        void NameEdited(const QString& text);
        void EnabledCheckBoxChanged(int state);

        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
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

        QWidget* GetAddTrackWidget()                                                                        { return mAddTrackWidget; }

    public slots:
        void OnAddTrackButtonClicked()                                                                      { CommandSystem::CommandAddEventTrack(); }
        void OnTrackNameChanged(const QString& text, int trackNr)                                           { CommandSystem::CommandRenameEventTrack(trackNr, FromQtString(text).c_str()); }
        void OnTrackEnabledStateChanged(bool enabled, int trackNr)                                          { CommandSystem::CommandEnableEventTrack(trackNr, enabled); }
        void OnDetailedNodesCheckBox(int state);
        void OnCheckBox(int state);
        void OnComboBoxIndexChanged(int state);

    private:
        TimeViewPlugin*                     mPlugin;
        QVBoxLayout*                        mMainLayout;
        QWidget*                            mTrackWidget;
        QVBoxLayout*                        mTrackLayout;
        QWidget*                            mAddTrackWidget;
        QPushButton*                        mAddTrackButton;
        MysticQt::DialogStack*              mStackWidget;
        QComboBox*                          mGraphContentsComboBox;
        QComboBox*                          mNodeContentsComboBox;
        QCheckBox*                          mNodeNamesCheckBox;
        QCheckBox*                          mMotionFilesCheckBox;

        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
    };
} // namespace EMStudio
