/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

class QSignalMapper;

namespace Ui
{
    class CustomMessageBoxDialog;
}

namespace News 
{
    class ArticleDetails;
    class BuilderResourceManifest;
    class SelectImage;
    class ResourceManifest;
    class Resource;

    //! Allows to add custom buttons which is not well-supported by default QMessageBox
    class QCustomMessageBox
        : public QDialog
    {
        Q_OBJECT

    public:
        enum Icon {
            NoIcon = 0,
            Information = 1,
            Warning = 2,
            Critical = 3,
            Question = 4
        };

        QCustomMessageBox(
            Icon,
            const QString& title,
            const QString& text,
            QWidget* parent);
        ~QCustomMessageBox();

        void AddButton(const QString& name, int result);

    private:
        QScopedPointer<Ui::CustomMessageBoxDialog> m_ui;
        QSignalMapper* m_signalMapper = nullptr;

    private Q_SLOTS:
        void clickedSlot(int result);
    };
} // namespace News
