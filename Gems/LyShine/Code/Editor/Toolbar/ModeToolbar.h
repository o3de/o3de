/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include "AlignToolbarSection.h"

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <QToolBar>
#endif

class EditorWindow;
class QActionGroup;
class QAction;

class ModeToolbar : public QToolBar
{
    Q_OBJECT

public:
    explicit ModeToolbar(EditorWindow* parent);

    void SetCheckedItem(int index);

    AlignToolbarSection* GetAlignToolbarSection() const
    {
        return m_alignToolbarSection.get();
    }

private:
    void AddModes(EditorWindow* parent);

    QActionGroup* m_group;
    QAction* m_previousAction;

    std::unique_ptr<AlignToolbarSection> m_alignToolbarSection;
};
