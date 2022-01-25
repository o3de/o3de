/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QLabel>
#include <QWidget>
#include <QVector>
#include <QStringList>
#endif

namespace O3DE::ProjectManager
{
    struct Tag
    {
        QString text;
        QString id;
    };

    // Single tag
    class TagWidget
        : public QLabel
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit TagWidget(const Tag& id, QWidget* parent = nullptr);
        ~TagWidget() = default;

    signals:
        void TagClicked(const Tag& tag);

    protected:
        void mousePressEvent(QMouseEvent* event) override;

    private:
        Tag m_tag;
    };

    // Widget containing multiple tags, automatically wrapping based on the size
    class TagContainerWidget
        : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit TagContainerWidget(QWidget* parent = nullptr);
        ~TagContainerWidget() = default;

        void Update(const QVector<Tag>& tags);
        void Update(const QStringList& tags);

    signals:
        void TagClicked(const Tag& tag);

    private:
        void Clear();
    };
} // namespace O3DE::ProjectManager
