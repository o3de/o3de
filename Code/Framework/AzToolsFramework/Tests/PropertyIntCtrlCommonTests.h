/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/UI/PropertyEditor/PropertyIntCtrlCommon.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzCore/std/typetraits/is_signed.h>
#include "IntegerPrimtitiveTestConfig.h"
#include <QApplication>
#include <sstream>

namespace UnitTest
{
    template<typename ValueType, template <typename> class HandlerType>
    class IntrCtrlHandlerAPI 
        : public HandlerType<ValueType>
    {
        using Parent = HandlerType<ValueType>;
        using Parent::CreateGUI;
        using Parent::ConsumeAttribute;
        using Parent::ReadValuesIntoGUI;
        using Parent::WriteGUIValuesIntoProperty;
        using Parent::ModifyTooltip;
    public:
        QWidget* CreateGUI(QWidget* pParent)
        {
            return Parent::CreateGUI(pParent);
        }

        void ConsumeAttribute(IntrCtrlHandlerAPI* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
        {
            Parent::ConsumeAttribute(GUI, attrib, attrValue, debugName);
        }

        bool ReadValuesIntoGUI(size_t index, IntrCtrlHandlerAPI* GUI, const ValueType& instance, AzToolsFramework::InstanceDataNode* node)
        {
            return Parent::ReadValuesIntoGUI(index, GUI, instance, node);
        }

        void WriteGUIValuesIntoProperty(size_t index, IntrCtrlHandlerAPI* GUI, ValueType& instance, AzToolsFramework::InstanceDataNode* node)
        {
            Parent::WriteGUIValuesIntoProperty(index, GUI, instance, node);
        }

        bool ModifyTooltip(QWidget* widget, QString& toolTipString)
        {
            return Parent::ModifyTooltip(widget, toolTipString);
        }
    };

    template<typename ValueType, typename WidgetType, template <typename> class HandlerType>
    struct PropertyCtrlFixture
        : public ToolsApplicationFixture<>
    {
        using HandlerAPI = IntrCtrlHandlerAPI<ValueType, HandlerType>;

        void SetUpEditorFixtureImpl() override
        {
            // note: must set a widget as the active window and add widgets
            // as children to ensure focus in/out events fire correctly
            m_dummyWidget = AZStd::make_unique<QWidget>();
            QApplication::setActiveWindow(m_dummyWidget.get());

            m_handler = AZStd::make_unique<HandlerAPI>();
            m_widget = static_cast<WidgetType*>(m_handler->CreateGUI(m_dummyWidget.get()));
        }

        void TearDownEditorFixtureImpl() override
        {
            QApplication::setActiveWindow(nullptr);
            m_dummyWidget.reset();
            m_handler.reset();
        }

        static void SetWidgetRangeToNonExtremeties(WidgetType* widget)
        {
            widget->setMinimum(widget->minimum() + 1);
            widget->setMaximum(widget->maximum() - 1);
        }

        void PropertyCtrlHandlersCreated()
        {
            using ::testing::Ne;

            EXPECT_THAT(m_handler, Ne(nullptr));
        }

        void PropertyCtrlWidgetsCreated()
        {
            using ::testing::Ne;

            EXPECT_THAT(m_widget, Ne(nullptr));
        }

        void Widget_Minimum_ExpectQtWidgetLimits_Min()
        {
            EXPECT_EQ(m_widget->minimum(), AzToolsFramework::QtWidgetLimits<ValueType>::Min());
        }

        void Widget_Maximum_ExpectQtWidgetLimits_Max()
        {
            EXPECT_EQ(m_widget->maximum(), AzToolsFramework::QtWidgetLimits<ValueType>::Max());
        }

        void HandlerMinMaxLimit_ModifyHandler_ExpectSuccessAndValidRangeLimitToolTipString()
        {
            // Given a widget
            auto& widget = m_widget;
            auto& handler = m_handler;
            QString tooltip;

            // Retrieve the tooltip string for this widget
            auto success = handler->ModifyTooltip(widget, tooltip);

            const QString minString = QLocale().toString(widget->minimum());
            const QString maxString = QLocale().toString(widget->maximum());
            const AZStd::string expected = AZStd::string::format("[%s, %s]", minString.toStdString().c_str(), maxString.toStdString().c_str());

            // Expect the operation to be successful and a valid limit tooltip string generated
            EXPECT_TRUE(success);
            EXPECT_STREQ(tooltip.toStdString().c_str(), expected.c_str());
        }

        void HandlerMinMaxLessLimit_ModifyHandler_ExpectSuccessAndValidLessLimitToolTipString()
        {
            // Given a widget
            auto& widget = m_widget;
            auto& handler = m_handler;
            QString tooltip;

            // That is not at the extremeties of the type range limit
            SetWidgetRangeToNonExtremeties(widget);

            // Retrieve the tooltip string for this widget
            auto success = handler->ModifyTooltip(widget, tooltip);

            const QString minString = QLocale().toString(widget->minimum());
            const QString maxString = QLocale().toString(widget->maximum());

            const AZStd::string expected = AZStd::string::format("[%s, %s]", minString.toStdString().c_str(), maxString.toStdString().c_str());

            // Expect the operation to be successful and a valid less than limit tooltip string generated
            EXPECT_TRUE(success);
            EXPECT_STREQ(tooltip.toStdString().c_str(), expected.c_str());
        }

        void EmitWidgetValueChanged()
        {
            emit m_widget->valueChanged(ValueType(0));
        }

        void EmitWidgetEditingFinished()
        {
            emit m_widget->editingFinished();
        }

        AZStd::unique_ptr<QWidget> m_dummyWidget;
        AZStd::unique_ptr<HandlerAPI> m_handler;
        WidgetType* m_widget;
    };
} // namespace UnitTest
