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
#include <QToolBar>
#endif

class EditorWindow;
class QActionGroup;
class QAction;
class AlignToolbarSection;

class ModeToolbar
    : public QToolBar
{
    Q_OBJECT

public:

    explicit ModeToolbar(EditorWindow* parent);

    void SetCheckedItem(int index);

    AlignToolbarSection* GetAlignToolbarSection()
    {
        return m_alignToolbarSection.get();
    }

private:

    void AddModes(EditorWindow* parent);

    QActionGroup* m_group;
    QAction* m_previousAction;

    std::unique_ptr<AlignToolbarSection> m_alignToolbarSection;
};
