/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QScopedPointer>

#include <QUndoStack>
#endif

namespace AzQtComponents
{
    class SpinBox;
    class DoubleSpinBox;
}

namespace Ui {
    class SpinBoxPage;
}

class SpinBoxPage : public QWidget
{
    Q_OBJECT

public:
    explicit SpinBoxPage(QWidget* parent = nullptr);
    ~SpinBoxPage() override;

    const QUndoStack& getUndoStack() const { return m_undoStack; }

private:

    template <typename SpinBoxType, typename ValueType>
    void track(SpinBoxType* spinBox);

    QScopedPointer<Ui::SpinBoxPage> ui;

    QUndoStack m_undoStack;
};
