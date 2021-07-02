/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArticleErrorView.h"
#include "NewsShared/Qt/ui_ArticleErrorView.h"

using namespace News;

ArticleErrorView::ArticleErrorView(
    QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::ArticleErrorViewWidget())
{
    m_ui->setupUi(this);
}

ArticleErrorView::~ArticleErrorView(){}

#include "NewsShared/Qt/moc_ArticleErrorView.cpp"
