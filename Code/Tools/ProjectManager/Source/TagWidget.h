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
#include <QLabel>
#include <QStringList>
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace O3DE::ProjectManager
{
    // Single tag
    class TagWidget
        : public QLabel
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit TagWidget(const QString& text, QWidget* parent = nullptr);
        ~TagWidget() = default;
    };

    // Widget containing multiple tags, automatically wrapping based on the size
    class TagContainerWidget
        : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit TagContainerWidget(QWidget* parent = nullptr);
        ~TagContainerWidget() = default;

        void Update(const QStringList& tags);

    private:
        QVBoxLayout* m_layout = nullptr;
        QWidget* m_widget = nullptr;
    };
} // namespace O3DE::ProjectManager
