/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PropertyIntCtrlCommonTests.h"
#include <AzToolsFramework/UI/PropertyEditor/PropertyIntSpinCtrl.hxx>

namespace UnitTest
{
    using namespace AzToolsFramework;

    template <typename ValueType>
    using PropertySpinCtrlFixture = PropertyCtrlFixture<ValueType, PropertyIntSpinCtrl, IntSpinBoxHandler>;

    TYPED_TEST_CASE(PropertySpinCtrlFixture, IntegerPrimtitiveTestConfigs);

    TYPED_TEST(PropertySpinCtrlFixture, PropertySpinCtrlHandlersCreated)
    {
        this->PropertyCtrlHandlersCreated();
    }
    
    TYPED_TEST(PropertySpinCtrlFixture, PropertySpinCtrlWidgetsCreated)
    {
        this->PropertyCtrlWidgetsCreated();
    }
    
    TYPED_TEST(PropertySpinCtrlFixture, SpinBoxWidget_Minimum_ExpectQtWidgetLimits_Min)
    {
        this->Widget_Minimum_ExpectQtWidgetLimits_Min();
    }
    
    TYPED_TEST(PropertySpinCtrlFixture, SpinBoxWidget_Maximum_ExpectQtWidgetLimits_Max)
    {
        EXPECT_EQ(this->m_widget->maximum(), AzToolsFramework::QtWidgetLimits<TypeParam>::Max());
    }

    TYPED_TEST(PropertySpinCtrlFixture, SpinBoxHandlerMinMaxLimit_ModifyHandler_ExpectSuccessAndValidRangeLimitToolTipString)
    {
        this->HandlerMinMaxLimit_ModifyHandler_ExpectSuccessAndValidRangeLimitToolTipString();
    }

    TYPED_TEST(PropertySpinCtrlFixture, SpinBoxHandlerMinMaxLessLimit_ModifyHandler_ExpectSuccessAndValidLessLimitToolTipString)
    {
        this->HandlerMinMaxLessLimit_ModifyHandler_ExpectSuccessAndValidLessLimitToolTipString();
    }

    struct PropertyEditorHandler
        : public AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler
    {
        PropertyEditorHandler()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusConnect();
        }

        ~PropertyEditorHandler()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusDisconnect();
        }

        // AzToolsFramework::PropertyEditorGUIMessages::Bus overrides ...
        void RequestWrite([[maybe_unused]] QWidget* editorGUI) override
        {
            m_requestWriteCallCount++;
        }

        void RequestRefresh([[maybe_unused]] PropertyModificationRefreshLevel level) override
        {
        }

        void AddElementsToParentContainer(
            [[maybe_unused]] QWidget* editorGUI,
            [[maybe_unused]] size_t numElements,
            [[maybe_unused]] const InstanceDataNode::FillDataClassCallback& fillDataCallback) override
        {
        }

        void RequestPropertyNotify([[maybe_unused]] QWidget* editorGUI) override
        {
        }

        void OnEditingFinished([[maybe_unused]]QWidget* editorGUI) override
        {
            m_onEditingFinishedCallCount++;
        }

        int m_requestWriteCallCount = 0;
        int m_onEditingFinishedCallCount = 0;
    };

    TYPED_TEST(PropertySpinCtrlFixture, SpinBoxWidgetValueChangedInvokesPropertyEditorGUIMessages)
    {
        // setup the event handler
        PropertyEditorHandler eventHandler;

        // trigger the QT signal
        this->EmitWidgetValueChanged();

        // there should be at least 1 call to RequestWrite.
        EXPECT_GT(eventHandler.m_requestWriteCallCount, 0);
    }

    TYPED_TEST(PropertySpinCtrlFixture, SpinBoxWidgetEditingFinishedInvokesPropertyEditorGUIMessages)
    {
        // setup the event handler
        PropertyEditorHandler eventHandler;

        // trigger the QT signal
        this->EmitWidgetEditingFinished();

        // there should be at least 1 call to OnEditingFinished.
        EXPECT_GT(eventHandler.m_onEditingFinishedCallCount, 0);
    }
} // namespace UnitTest
