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
#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#endif


class EditorWindow;
struct IUiAnimationSystem;
struct IUiAnimSequence;
QT_FORWARD_DECLARE_CLASS(QListWidget)

class PreviewAnimationList
    : public QMainWindow
{
    Q_OBJECT

public: // member functions

    PreviewAnimationList(EditorWindow* editorWindow);
    ~PreviewAnimationList();

    //! Initialize the animation list
    void Activate(AZ::EntityId canvasEntityId);

    //! Clear the animation list
    void Deactivate();

private: // types

    //! Actions that can be performed by the toolbar
    enum class Action
    {
        Play,
        Pause,
        Stop,
        Reset
    };

private: // member functions

    QSize sizeHint() const override;

    void AddMenuItems();

    void AddToolBarButton(const QIcon& icon, Action action, const char* tooltip);

    void DoActionOnSelectedAnimations(Action action);

private: // data

    AZ::EntityId m_canvasEntityId;
    QListWidget* m_listWidget;
    QToolBar* m_toolBar;
};
