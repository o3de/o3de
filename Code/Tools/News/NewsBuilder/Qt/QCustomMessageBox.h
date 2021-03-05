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
