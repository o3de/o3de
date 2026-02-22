/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/EntityId.h>
#include <QMainWindow>
#endif

class EditorWindow;
class QIcon;
class QListWidget;
class QToolBar;
struct IUiAnimationSystem;
struct IUiAnimSequence;

class PreviewAnimationList : public QMainWindow
{
    Q_OBJECT

public:
    PreviewAnimationList(EditorWindow* editorWindow);
    ~PreviewAnimationList();

    //! Initialize the animation list
    void Activate(AZ::EntityId canvasEntityId);

    //! Clear the animation list
    void Deactivate();

private:
    //! Actions that can be performed by the toolbar
    enum class Action
    {
        Play,
        Pause,
        Stop,
        Reset
    };

    QSize sizeHint() const override;

    void AddMenuItems();
    void AddToolBarButton(const QIcon& icon, Action action, const char* tooltip);
    void DoActionOnSelectedAnimations(Action action);

private:
    AZ::EntityId m_canvasEntityId;
    QListWidget* m_listWidget;
    QToolBar* m_toolBar;
};
