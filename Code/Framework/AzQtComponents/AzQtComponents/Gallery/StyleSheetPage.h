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
#include <QWidget>
#include <QScopedPointer>
#endif

namespace Ui
{
    class StyleSheetPage;
}

namespace Example
{
    /* Example::Widget is used to demonstrate StyleHelpers::repolishWhenPropertyChanges.
     *
     * StyleHelpers::repolishWhenPropertyChanges is used to ensure that style sheet rules that
     * depend on the property of a widget are correctly updated when that property changes.
     *
     * See also: ExampleWidget.qss
     */
    class Widget : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(bool drawSimple READ drawSimple WRITE setDrawSimple NOTIFY drawSimpleChanged)
        Q_PROPERTY(bool tearEnabled READ tearEnabled WRITE setTearEnabled NOTIFY tearEnabledChanged)
    public:
        explicit Widget(QWidget* parent = nullptr);
        ~Widget() override;

        bool drawSimple() const { return m_drawSimple; }
        bool tearEnabled() const { return m_tearEnabled; }

    public Q_SLOTS:
        void setDrawSimple(bool drawSimple);
        void setTearEnabled(bool tearEnabled);

    Q_SIGNALS:
        void drawSimpleChanged(bool drawSimple);
        void tearEnabledChanged(bool tearEnabled);

    private:
        bool m_drawSimple;
        bool m_tearEnabled;
    };
}

class StyleSheetPage : public QWidget
{
    Q_OBJECT

public:
    explicit StyleSheetPage(QWidget* parent = nullptr);
    ~StyleSheetPage() override;

private:
    QScopedPointer<Ui::StyleSheetPage> ui;
};
