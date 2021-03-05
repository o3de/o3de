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
