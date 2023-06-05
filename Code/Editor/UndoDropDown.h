/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "Undo/IUndoManagerListener.h"
#endif

/// Turns IUndoManagerListener callbacks into signals
class UndoStackStateAdapter
    : public QObject
    , IUndoManagerListener
{
    Q_OBJECT

public:
    explicit UndoStackStateAdapter(QObject* parent = nullptr);

    ~UndoStackStateAdapter();

    void SignalNumUndoRedo(const unsigned int& numUndo, const unsigned int& numRedo)
    {
        emit UndoAvailable(numUndo);
        emit RedoAvailable(numRedo);
    }

signals:
    void UndoAvailable(quint32 count);
    void RedoAvailable(quint32 count);
};
