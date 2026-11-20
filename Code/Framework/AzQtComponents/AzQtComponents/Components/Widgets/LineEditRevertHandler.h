/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QObject>
#include <QString>
#endif

class QLineEdit;
class QWidget;

namespace AzQtComponents
{
    //! Event filter that gives any QLineEdit Esc / Ctrl+Z revert-to-focus-in behavior.
    //!
    //! On FocusIn the current text is captured. When the user presses Escape or
    //! Ctrl+Z (while the text differs from the captured value), the text is restored
    //! and focus is cleared on the designated target widget via a deferred call.
    //!
    //! Usage:
    //!   new LineEditRevertHandler(lineEdit, clearFocusTarget);
    //!
    //! The handler parents itself to @p lineEdit so it is destroyed automatically.
    class AZ_QT_COMPONENTS_API LineEditRevertHandler : public QObject
    {
        Q_OBJECT
    public:
        //! @param lineEdit       The QLineEdit to watch.
        //! @param clearFocusTarget  The widget that receives clearFocus() on revert.
        //!                          Pass nullptr to clear focus on the lineEdit itself.
        explicit LineEditRevertHandler(QLineEdit* lineEdit, QWidget* clearFocusTarget = nullptr);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        QLineEdit* m_lineEdit = nullptr;
        QWidget* m_clearFocusTarget = nullptr;
        QString m_textOnFocusIn;
    };

} // namespace AzQtComponents
