/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MysticQt/Source/DialogStack.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <EMotionFX/Source/MorphTarget.h>
#include <QDialog>
#include <QTableWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QUrl>
#endif

QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QListWidget)
QT_FORWARD_DECLARE_CLASS(QListWidgetItem)
QT_FORWARD_DECLARE_CLASS(QDragEnterEvent)
QT_FORWARD_DECLARE_CLASS(QDropEvent)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QProgressBar)

class LipsyncBatchGeneratorEntryWidget;

namespace EMotionFX
{
    class MorphSetup;
}

namespace EMStudio
{
    // forward declaration
    class EMStudioPlugin;

    // widgets
    class DragTableWidget
        : public QTableWidget
    {
        Q_OBJECT
    public:
        DragTableWidget(int rows = 0, int columns = 0, QWidget* parent = nullptr)
            : QTableWidget(rows, columns, parent)
        {
            // enable drag and drop for the tables
            setAcceptDrops(true);
            setDragEnabled(true);
        }

    protected:
        void dragEnterEvent(QDragEnterEvent* event) override
        {
            event->acceptProposedAction();
        }
        void dragLeaveEvent(QDragLeaveEvent* event) override
        {
            event->accept();
        }

        void dragMoveEvent(QDragMoveEvent* event) override
        {
            event->accept();
        }

        void dropEvent(QDropEvent* event) override
        {
            event->acceptProposedAction();
            emit dataDropped();
        }

    signals:
        void dataDropped();
    };

    /**
     * one single entry of the phoneme selection window
     */
    class VisimeWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(VisimeWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        VisimeWidget(const AZStd::string& filename);
        virtual ~VisimeWidget();

        void SetSelected(bool selected = true)        { m_selected = selected; }
        void UpdateInterface();

        void paintEvent(QPaintEvent* event) override;
        void enterEvent(QEvent* event) override             { MCORE_UNUSED(event); m_mouseWithinWidget = true; repaint(); }
        void leaveEvent(QEvent* event) override             { MCORE_UNUSED(event); m_mouseWithinWidget = false; repaint(); }

    private:
        AZStd::string   m_fileName;
        AZStd::string   m_fileNameWithoutExt;
        QPixmap*        m_pixmap;
        bool            m_selected;
        bool            m_mouseWithinWidget;
    };


    /**
     * class for the phoneme selection dialog.
     */
    class PhonemeSelectionWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(PhonemeSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        PhonemeSelectionWindow(EMotionFX::Actor* actor, size_t lodLevel, EMotionFX::MorphTarget* morphTarget, QWidget* parent = nullptr);
        virtual ~PhonemeSelectionWindow();

        void Init();
        const char* GetPhonemeSetExample(EMotionFX::MorphTarget::EPhonemeSet phonemeSet);

    public slots:
        void UpdateInterface();
        void PhonemeSelectionChanged();
        void RemoveSelectedPhonemeSets();
        void AddSelectedPhonemeSets();
        void ClearSelectedPhonemeSets();

    protected:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void closeEvent(QCloseEvent* event) override;

    private:
        // the morph target
        EMotionFX::Actor*           m_actor;
        EMotionFX::MorphTarget*     m_morphTarget;
        size_t                      m_lodLevel;
        EMotionFX::MorphSetup*      m_morphSetup;

        // the dialogstacks
        MysticQt::DialogStack*      m_possiblePhonemeSets;
        MysticQt::DialogStack*      m_selectedPhonemeSets;
        DragTableWidget*            m_possiblePhonemeSetsTable;
        DragTableWidget*            m_selectedPhonemeSetsTable;

        QPushButton*    m_addPhonemesButton;
        QPushButton*    m_removePhonemesButton;
        QPushButton*    m_clearPhonemesButton;
        QPushButton*    m_addPhonemesButtonArrow;
        QPushButton*    m_removePhonemesButtonArrow;

        bool            m_dirtyFlag;
    };
} // namespace EMStudio
