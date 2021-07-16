/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// 'The problem is that with this line, "slots" is a keyword by default in Qt.' https://stackoverflow.com/questions/23068700/embedding-python3-in-qt-5
#pragma push_macro("slots")
#undef slots

#include <Python.h>

#pragma pop_macro("slots")
