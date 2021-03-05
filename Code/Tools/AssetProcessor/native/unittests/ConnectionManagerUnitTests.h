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
