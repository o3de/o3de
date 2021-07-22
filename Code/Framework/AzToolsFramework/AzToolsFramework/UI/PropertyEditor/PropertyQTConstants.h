/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef PROPERTY_QT_CONSTANTS
#define PROPERTY_QT_CONSTANTS

#include <AzCore/base.h>

#pragma once

// Provides a shared set of values used by properties as they initialize QT components.
namespace AzToolsFramework
{
    static const int PropertyQTConstant_DefaultHeight = 18;
    static const int PropertyQTConstant_MinimumWidth = 30;

    static const int PropertyQTConstant_LeftMargin = 2;
    static const int PropertyQTConstant_TopMargin = 2;

    // A slight right margin keeps the properties from bumping against the right
    // side of the window. This is especially useful for sliders, to avoid accidental
    // window resizing when someone wishes to manipulate the slider.
    static const int PropertyQTConstant_RightMargin = 4;
    static const int PropertyQTConstant_BottomMargin = 2;

    //! String for displaying inifinity
    static const char* PropertyQTConstant_InfinityString = "INF";
};

#endif
