/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QLabel>
#endif

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace O3DE::ProjectManager
{
    class TextOverflowDialog
        : public QDialog
    {
    public:
        explicit TextOverflowDialog(const QString& title = {}, const QString& text = {}, QWidget* parent = nullptr);
        ~TextOverflowDialog() = default;
    };


    class TextOverflowLabel
        : public QLabel
    {
    public:
        static QString ElideLinkedText(const QString& text, int maxLength);

        explicit TextOverflowLabel(const QString& title = {}, const QString& text = {}, QWidget* parent = nullptr);
        ~TextOverflowLabel() = default;

    private slots:
        void OnLinkActivated(const QString& link);

        QString m_title;
        QString m_fullText;
    };
} // namespace O3DE::ProjectManager
