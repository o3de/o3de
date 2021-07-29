/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QUndoStack>
#include <QUndoGroup>
#endif

class UndoStack
    : public QUndoStack
{
    Q_OBJECT

public:

    UndoStack(QUndoGroup* group);

    void SetIsExecuting(bool b);
    bool GetIsExecuting() const;

private:

    bool m_isExecuting;
};
