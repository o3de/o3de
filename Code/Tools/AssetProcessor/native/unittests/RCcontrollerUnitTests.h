/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef RCCONTROLLERUNITTESTS_H
#define RCCONTROLLERUNITTESTS_H

#if !defined(Q_MOC_RUN)
#include "UnitTestRunner.h"
#include "native/resourcecompiler/rccontroller.h"
#endif


class RCcontrollerUnitTests
    : public UnitTestRun
{
    Q_OBJECT
public:
    virtual void StartTest() override;

    RCcontrollerUnitTests();
    void Reset();

Q_SIGNALS:
    void RcControllerPrepared();
public Q_SLOTS:
    void PrepareRCController();
    void RunRCControllerTests();


private:
    AssetProcessor::RCController m_rcController;
    AssetBuilderSDK::AssetBuilderDesc m_assetBuilderDesc;
};

#endif // RCCONTROLLERUNITTESTS_H
