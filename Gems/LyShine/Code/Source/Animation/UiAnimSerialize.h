/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/SerializeContext.h>

namespace UiAnimSerialize
{
    //! Define Animation types for the AZ Serialize system
    void ReflectUiAnimTypes(AZ::SerializeContext* context);
}
