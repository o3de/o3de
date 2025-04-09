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
#endif

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QComboBox)

namespace O3DE::ProjectManager
{
    class FormComboBoxWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit FormComboBoxWidget(const QString& labelText, const QStringList& items = {}, QWidget* parent = nullptr);
        ~FormComboBoxWidget() = default;

        //! Set the error message for to display when invalid.
        void setErrorLabelText(const QString& labelText);
        void setErrorLabelVisible(bool visible);

        QComboBox* comboBox() const;

    protected:
        virtual bool eventFilter(QObject* object, QEvent* event);

        QLabel* m_errorLabel = nullptr;
        QFrame* m_frame = nullptr;
        QHBoxLayout* m_frameLayout = nullptr;
        QComboBox* m_comboBox = nullptr;

    private slots:
        void onFocus();
        void onFocusOut();

    private:
        void mousePressEvent(QMouseEvent* event) override;
        void refreshStyle();
    };
} // namespace O3DE::ProjectManager
