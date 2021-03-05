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

#pragma once

#include <AzCore/Interface/Interface.h>

#include <QVariantMap>

using PythonWorkerRequestId = int;

//! Interface to signal the python worker output
class PythonWorkerEventsInterface
{
protected:
    PythonWorkerEventsInterface() = default;
    virtual ~PythonWorkerEventsInterface() = default;
    PythonWorkerEventsInterface(PythonWorkerEventsInterface&&) = delete;
    PythonWorkerEventsInterface& operator=(PythonWorkerEventsInterface&&) = delete;

public:
    AZ_RTTI(PythonWorkerEventsInterface, "{60C83A5A-B8DD-4B98-B8C6-DC2F5914D7C4}");

    // if any OnOutput returns true, it means the command was handled and the worker won't process any further
    virtual bool OnPythonWorkerOutput(PythonWorkerRequestId requestId, const QString& key, const QVariant& value) = 0;
};

//! Interface to send the python worker requests
class PythonWorkerRequestsInterface
{
protected:
    PythonWorkerRequestsInterface() = default;
    virtual ~PythonWorkerRequestsInterface() = default;
    PythonWorkerRequestsInterface(PythonWorkerRequestsInterface&&) = delete;
    PythonWorkerRequestsInterface& operator=(PythonWorkerRequestsInterface&&) = delete;

public:
    AZ_RTTI(PythonWorkerRequestsInterface, "{B0293028-3575-408E-8CE3-D1B7F3C59A6C}");

    virtual PythonWorkerRequestId AllocateRequestId() = 0;
    virtual void ExecuteAsync(PythonWorkerRequestId requestId, const char* command, const QVariantMap& args = QVariantMap{}) = 0;
    virtual bool IsStarted() = 0;
};

