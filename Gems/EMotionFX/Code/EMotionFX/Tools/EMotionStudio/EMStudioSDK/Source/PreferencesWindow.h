/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioConfig.h>
#include <AzQtComponents/Components/Widgets/SegmentBar.h>
#include <QDialog>
#endif

QT_FORWARD_DECLARE_CLASS(QStackedWidget)

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace EMStudio
{
    class EMSTUDIO_API PreferencesWindow
        : public QDialog
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit PreferencesWindow(QWidget* parent = nullptr);
        virtual ~PreferencesWindow();

        void Init();

        AzToolsFramework::ReflectedPropertyEditor* AddCategory(const char* categoryName);
        void AddCategory(QWidget* widget, const char* categoryName);

        AzToolsFramework::ReflectedPropertyEditor* FindPropertyWidgetByName(const char* categoryName) const;

    public slots:
        void OnTabChanged(int newTabIndex);

    private:
        struct Category
        {
            QWidget* m_widget = nullptr;
            AzToolsFramework::ReflectedPropertyEditor* m_propertyWidget = nullptr;
            int m_tabIndex = -1;
            AZStd::string m_name;
        };

        Category* FindCategoryByName(const char* categoryName) const;
        
        AZStd::vector<AZStd::unique_ptr<Category>> m_categories;
        QStackedWidget* m_stackedWidget = nullptr;
        AzQtComponents::SegmentBar* m_categorySegmentBar = nullptr;
    };
} // namespace EMStudio
