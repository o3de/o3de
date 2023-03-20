/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

ACTION_TEMPLATE(ReturnMalloc,
                HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_0_VALUE_PARAMS()) {
    T* value = static_cast<T*>(malloc(sizeof(T)));
    *value = T{};
    return value;
}
ACTION_TEMPLATE(ReturnMalloc,
                HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_1_VALUE_PARAMS(p0)) {
    T* value = static_cast<T*>(malloc(sizeof(T)));
    *value = T{ p0 };
    return value;
}
ACTION_TEMPLATE(ReturnMalloc,
                HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_2_VALUE_PARAMS(p0, p1)) {
    T* value = static_cast<T*>(malloc(sizeof(T)));
    *value = T{ p0, p1 };
    return value;
}
ACTION_TEMPLATE(ReturnMalloc,
                HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_3_VALUE_PARAMS(p0, p1, p2)) {
    T* value = static_cast<T*>(malloc(sizeof(T)));
    *value = T{ p0, p1, p2 };
    return value;
}
ACTION_TEMPLATE(ReturnMalloc,
                HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_4_VALUE_PARAMS(p0, p1, p2, p3)) {
    T* value = static_cast<T*>(malloc(sizeof(T)));
    *value = T{ p0, p1, p2, p3 };
    return value;
}
ACTION_TEMPLATE(ReturnMalloc,
                HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_5_VALUE_PARAMS(p0, p1, p2, p3, p4)) {
    T* value = static_cast<T*>(malloc(sizeof(T)));
    *value = T{ p0, p1, p2, p3, p4 };
    return value;
}
ACTION_TEMPLATE(ReturnMalloc,
                HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_6_VALUE_PARAMS(p0, p1, p2, p3, p4, p5)) {
    T* value = static_cast<T*>(malloc(sizeof(T)));
    *value = T{ p0, p1, p2, p3, p4, p5 };
    return value;
}
ACTION_TEMPLATE(ReturnMalloc,
                HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_7_VALUE_PARAMS(p0, p1, p2, p3, p4, p5, p6)) {
    T* value = static_cast<T*>(malloc(sizeof(T)));
    *value = T{ p0, p1, p2, p3, p4, p5, p6 };
    return value;
}
