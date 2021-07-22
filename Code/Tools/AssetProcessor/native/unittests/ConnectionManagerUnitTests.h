/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CONNECTIONMANAGERUNITTEST_H
#define CONNECTIONMANAGERUNITTEST_H

#if !defined(Q_MOC_RUN)
#include "UnitTestRunner.h"
#endif

class ConnectionManager;

class ConnectionManagerUnitTest
    : public UnitTestRun
{
    Q_OBJECT
public:
    explicit ConnectionManagerUnitTest();
    virtual void StartTest() override;
    virtual int UnitTestPriority() const override;
    virtual ~ConnectionManagerUnitTest();


Q_SIGNALS:
    void RunFirstPhaseofUnitTestsForConnectionManager();
    void ConnectionRemoved();
    void AllConnectionsRemoved();

public Q_SLOTS:

    void RunFirstPartOfUnitTestsForConnectionManager();
    void RunSecondPartOfUnitTestsForConnectionManager();
    void RunThirdPartOfUnitTestsForConnectionManager();
    void RunFourthPartOfUnitTestsForConnectionManager();

    void ConnectionDeleted(unsigned int connId);

private:
    void UpdateConnectionManager();
    ConnectionManager* m_connectionManager;
};

#endif // CONNECTIONMANAGERUNITTEST_H
