/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PropertyIntCtrlCommonTests.h"
#include <AzToolsFramework/UI/PropertyEditor/PropertyIntSliderCtrl.hxx>

namespace UnitTest
{
    using namespace AzToolsFramework;

    template <typename ValueType>
    using PropertySliderCtrlFixture = PropertyCtrlFixture<ValueType, PropertyIntSliderCtrl, IntSliderHandler>;

    TYPED_TEST_CASE(PropertySliderCtrlFixture, IntegerPrimtitiveTestConfigs);

    TYPED_TEST(PropertySliderCtrlFixture, PropertySliderCtrlHandlersCreated)
    {
        this->PropertyCtrlHandlersCreated();
    }

    TYPED_TEST(PropertySliderCtrlFixture, PropertySliderCtrlWidgetsCreated)
    {
        this->PropertyCtrlWidgetsCreated();
    }

    TYPED_TEST(PropertySliderCtrlFixture, SliderWidget_Minimum_ExpectQtWidgetLimits_Min)
    {
        this->Widget_Minimum_ExpectQtWidgetLimits_Min();
    }

    TYPED_TEST(PropertySliderCtrlFixture, SliderWidget_Maximum_ExpectQtWidgetLimits_Max)
    {
        EXPECT_EQ(this->m_widget->maximum(), AzToolsFramework::QtWidgetLimits<TypeParam>::Max());
    }

    TYPED_TEST(PropertySliderCtrlFixture, SliderHandlerMinMaxLimit_ModifyHandler_ExpectSuccessAndValidRangeLimitToolTipString)
    {
        this->HandlerMinMaxLimit_ModifyHandler_ExpectSuccessAndValidRangeLimitToolTipString();
    }

    TYPED_TEST(PropertySliderCtrlFixture, SliderHandlerMinMaxLessLimit_ModifyHandler_ExpectSuccessAndValidLessLimitToolTipString)
    {
        this->HandlerMinMaxLessLimit_ModifyHandler_ExpectSuccessAndValidLessLimitToolTipString();
    }
} // namespace UnitTest
