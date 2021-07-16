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
} // namespace UnitTest
