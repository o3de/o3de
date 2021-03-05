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

#include "PreferencesWindow.h"

#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <QHBoxLayout>
#include <QStackedWidget>


namespace EMStudio
{
    PreferencesWindow::PreferencesWindow(QWidget* parent)
        : QDialog(parent)
    {
    }

    PreferencesWindow::~PreferencesWindow()
    {
        m_categories.clear();
    }

    void PreferencesWindow::Init()
    {
        setWindowTitle("Preferences");
        setSizeGripEnabled(false);

        m_categorySegmentBar = new AzQtComponents::SegmentBar(this);
        m_categorySegmentBar->setOrientation(Qt::Orientation::Vertical);
        connect(m_categorySegmentBar, &AzQtComponents::SegmentBar::currentChanged, this, &PreferencesWindow::OnTabChanged);

        m_stackedWidget = new QStackedWidget();
        m_stackedWidget->setMinimumSize(700, 480);

        QHBoxLayout* horizontalLayout = new QHBoxLayout();
        horizontalLayout->addWidget(m_categorySegmentBar);
        horizontalLayout->addWidget(m_stackedWidget);
        setLayout(horizontalLayout);

        m_categorySegmentBar->setCurrentIndex(0);
    }

    void PreferencesWindow::AddCategory(QWidget* widget, const char* categoryName)
    {
        const int tabIndex = m_categorySegmentBar->addTab(categoryName);
        m_stackedWidget->addWidget(widget);

        Category* category = new Category();
        category->m_name = categoryName;
        category->m_widget = widget;
        category->m_tabIndex = tabIndex;

        m_categories.emplace_back(category);
    }

    AzToolsFramework::ReflectedPropertyEditor* PreferencesWindow::AddCategory(const char* categoryName)
    {
        const int tabIndex = m_categorySegmentBar->addTab(categoryName);

        AzToolsFramework::ReflectedPropertyEditor* propertyWidget = aznew AzToolsFramework::ReflectedPropertyEditor(this);
        propertyWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        m_stackedWidget->addWidget(propertyWidget);

        Category* category = new Category();
        category->m_name = categoryName;
        category->m_widget = propertyWidget;
        category->m_propertyWidget = propertyWidget;
        category->m_tabIndex = tabIndex;

        m_categories.emplace_back(category);
        return propertyWidget;
    }

    PreferencesWindow::Category* PreferencesWindow::FindCategoryByName(const char* categoryName) const
    {
        for (const AZStd::unique_ptr<Category>& category : m_categories)
        {
            if (category->m_name == categoryName)
            {
                return category.get();
            }
        }

        return nullptr;
    }

    AzToolsFramework::ReflectedPropertyEditor* PreferencesWindow::FindPropertyWidgetByName(const char* categoryName) const
    {
        Category* category = FindCategoryByName(categoryName);
        if (!category)
        {
            return nullptr;
        }

        return category->m_propertyWidget;
    }

    void PreferencesWindow::OnTabChanged(int newTabIndex)
    {
        m_stackedWidget->setCurrentIndex(newTabIndex);
    }
} // namespace EMStudio
