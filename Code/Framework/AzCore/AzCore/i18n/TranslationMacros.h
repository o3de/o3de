/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// QT_TRANSLATE_NOOP is a Qt macro that marks strings for extraction by lupdate
// but does NOT perform runtime translation. It simply returns the string as-is.
// This header provides a no-op fallback definition for modules that don't depend on Qt
// (such as AzCore), allowing them to mark strings for translation without pulling in Qt headers.
//
// When Qt headers are included (e.g., in AzToolsFramework or Editor), Qt's own definition takes precedence.
#ifndef QT_TRANSLATE_NOOP
    #define QT_TRANSLATE_NOOP(scope, x) (x)
#endif
