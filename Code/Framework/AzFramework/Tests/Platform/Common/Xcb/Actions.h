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
