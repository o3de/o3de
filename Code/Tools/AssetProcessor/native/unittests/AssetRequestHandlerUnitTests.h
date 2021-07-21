/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#if !defined(Q_MOC_RUN)
#include "UnitTestRunner.h"
#endif

namespace AssetProcessor
{
    class AssetRequestHandlerUnitTests
        : public UnitTestRun
    {
        Q_OBJECT
    public:
        AssetRequestHandlerUnitTests();
        virtual void StartTest() override;
    };
}
