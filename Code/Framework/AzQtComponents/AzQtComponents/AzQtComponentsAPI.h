/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

/**
 * \mainpage
 * 
 * Using this library, UI developers can build their own tools and
 * extensions for Open 3D Engine, while maintaining a coherent and standardized UI experience.
 * This custom library provides new and extended widgets, and includes a set of styles and
 * user interaction patterns that are applied on top of the Qt framework - the C++ library
 * that Open 3D Engine relies on for its UI. The library can be extended to support your own customizations and modifications. 
 * 
 * With this UI 2.0 API reference guide, we're working towards offering a full and comprehensive
 * API refernce for all tools developers that are extending Open 3D Engine. The API reference
 * is intended for C++ programmers building tools. For UX designers looking to understand
 * the best patterns and practices when making a tool to comfortably integrate with
 * the Open 3D Engine editor, see the [Tools UI Developer's Guide](https://www.o3de.org/docs/tools-ui/).
 */
#include <AzCore/PlatformDef.h>

#if defined(AZ_QT_COMPONENTS_STATIC)
    // if we're statically linking, then we don't need to export or import symbols
    #define AZ_QT_COMPONENTS_API
#elif defined(AZ_QT_COMPONENTS_EXPORT_SYMBOLS)
    #define AZ_QT_COMPONENTS_API AZ_DLL_EXPORT
#else
    #define AZ_QT_COMPONENTS_API AZ_DLL_IMPORT
#endif

namespace AzQtComponents
{
    constexpr const char* HasSearchAction = "HasSearchAction";
    constexpr const char* HasError = "HasError";
    constexpr const char* ClearAction = "_q_qlineeditclearaction";
    constexpr const char* ClearToolButton = "ClearToolButton";
    constexpr const char* ErrorToolButton = "ErrorToolButton";
    constexpr const char* SearchToolButton = "SearchToolButton";
    constexpr const char* StoredClearButtonState = "_storedClearButtonState";
    constexpr const char* StoredHoverAttributeState = "_storedWaHoverState";
    constexpr const char* ErrorMessage = "_errorMessage";
    constexpr const char* ErrorIconEnabled = "_errorIconEnabled";
    constexpr const char* Validator = "Validator";
    constexpr const char* HasPopupOpen = "HasPopupOpen";
    constexpr const char* HasExternalError = "HasExternalError";
    constexpr const char* NoMargins = "NoMargins";
    constexpr const char* ValidDropTarget = "ValidDropTarget";
    constexpr const char* InvalidDropTarget = "InvalidDropTarget";
}

