/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

namespace Ui
{
    class ArticleErrorViewWidget;
}

namespace AzQtComponents
{
    class ExtendedLabel;
}

namespace News
{
    class ArticleErrorView
        : public QWidget
    {
        Q_OBJECT
    public:
        ArticleErrorView(QWidget* parent);
        ~ArticleErrorView();

    private:
        QScopedPointer<Ui::ArticleErrorViewWidget> m_ui;
    };
}
