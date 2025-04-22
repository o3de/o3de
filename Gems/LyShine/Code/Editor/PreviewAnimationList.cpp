/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include <LyShine/Animation/IUiAnimation.h>

#include <QListWidget>
#include <QMenuBar>

PreviewAnimationList::PreviewAnimationList(EditorWindow* editorWindow)
    : QMainWindow(editorWindow)
    , m_listWidget(new QListWidget(this))
    , m_toolBar(new QToolBar("Play Toolbar", this))
{
    AddMenuItems();

    // Add the Reset button
    AddToolBarButton(QIcon(":/Trackview/play/tvplay-00.png"), Action::Reset,
        "Reset selected animations to start");

    // Add the Play button
    AddToolBarButton(QIcon(":/Trackview/play/tvplay-01.png"), Action::Play,
        "Play/Resume selected animations");

    // Add the Pause button
    AddToolBarButton(QIcon(":/Trackview/play/tvplay-03.png"), Action::Pause,
        "Pause/Resume selected animations");

    // Add the Stop button
    AddToolBarButton(QIcon(":/Trackview/play/tvplay-04.png"), Action::Stop,
        "Stop selected animations and set to end");

    m_toolBar->setFloatable(false);

    addToolBar(m_toolBar);

    // allow multiple selection
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    setCentralWidget(m_listWidget);
}

PreviewAnimationList::~PreviewAnimationList()
{
}

void PreviewAnimationList::Activate(AZ::EntityId canvasEntityId)
{
    m_canvasEntityId = canvasEntityId;

    IUiAnimationSystem* animationSystem = nullptr;
    UiCanvasBus::EventResult(animationSystem, m_canvasEntityId, &UiCanvasBus::Events::GetAnimationSystem);

    if (animationSystem)
    {
        // create a list item for every sequence in the canvas,
        // in the order that they occur in the canvas
        int numSequences = animationSystem->GetNumSequences();
        for (int i = 0; i < numSequences; ++i)
        {
            IUiAnimSequence* animSequence = animationSystem->GetSequence(i);
            QString sequenceName(animSequence->GetName());
            new QListWidgetItem(sequenceName, m_listWidget);
        }

        // set the first item in the list to be selected
        if (m_listWidget->item(0))
        {
            m_listWidget->item(0)->setSelected(true);
        }
    }
}

void PreviewAnimationList::Deactivate()
{
    m_listWidget->clear();
}

QSize PreviewAnimationList::sizeHint() const
{
    return QSize(160, 200);
}

void PreviewAnimationList::AddMenuItems()
{
    QMenu* menu = menuBar()->addMenu("&View");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    QList<QToolBar*> list = findChildren<QToolBar*>();
    for (auto p : list)
    {
        if (p->parent() == this)
        {
            menu->addAction(p->toggleViewAction());
        }
    }
}

void PreviewAnimationList::AddToolBarButton(const QIcon& icon, Action action, const char* tooltip)
{
    QPushButton* button = new QPushButton(icon, "", this);
    QObject::connect(button,
        &QPushButton::clicked,
        this,
        [this, action](bool) { DoActionOnSelectedAnimations(action); });
    button->setToolTip(QString(tooltip));
    m_toolBar->addWidget(button);
}

void PreviewAnimationList::DoActionOnSelectedAnimations(Action action)
{
    IUiAnimationSystem* animationSystem = nullptr;
    UiCanvasBus::EventResult(animationSystem, m_canvasEntityId, &UiCanvasBus::Events::GetAnimationSystem);
    if (nullptr == animationSystem)
    {
        return;     // m_canvasEntityId may not be valid
    }

    QList<QListWidgetItem*> selectedListItems = m_listWidget->selectedItems();

    int numSequences = animationSystem->GetNumSequences();
    for (int i = 0; i < numSequences; ++i)
    {
        IUiAnimSequence* animSequence = animationSystem->GetSequence(i);

        QListWidgetItem* listItem = m_listWidget->item(i);

        if (selectedListItems.contains(listItem))
        {
            switch (action)
            {
            case Action::Play:
                // if some of the selected sequences are paused then we want play to resume them
                if (animSequence->IsPaused())
                {
                    animSequence->Resume();
                }
                else
                {
                    animationSystem->PlaySequence(animSequence, nullptr, false, false);
                }
                break;

            case Action::Pause:
                // the pause button will toggle paused state for the selected sequences
                if (animSequence->IsPaused())
                {
                    animSequence->Resume();
                }
                else
                {
                    animSequence->Pause();
                }
                break;

            case Action::Stop:
                animationSystem->StopSequence(animSequence);
                break;

            case Action::Reset:
                animationSystem->StopSequence(animSequence);
                animSequence->Reset(true);
                break;
            }
        }
    }
}

#include <moc_PreviewAnimationList.cpp>
