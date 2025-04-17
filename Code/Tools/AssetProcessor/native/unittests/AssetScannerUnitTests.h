/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif
#include <QObject>

namespace UnitTest
{
    class AssetScannerUnitTest
        : public QObject
        , public LeakDetectionFixture
    {
        Q_OBJECT
    public:
    };
} // namespace UnitTest
