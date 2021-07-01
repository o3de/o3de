/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/Component.h>

// QT
AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
#include <QtWidgets/QWidget>
AZ_POP_DISABLE_WARNING
#include <QtCore/QEvent>
#include <QtWidgets/QLabel>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollArea>

#if defined(AZ_PLATFORM_APPLE_OSX) || defined(AZ_PLATFORM_LINUX)
typedef void* HWND;
typedef void* HMODULE;
typedef quint32 DWORD;
#define _MAX_PATH 260
#define MAX_PATH 260
#endif
