/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "native/utilities/ApplicationManagerBase.h"
#endif

namespace AssetProcessor
{
    extern const char ExcludeMetaDataFiles[];
}

class BatchApplicationManager
    : public ApplicationManagerBase
{
    Q_OBJECT
public:
    explicit BatchApplicationManager(int* argc, char*** argv, QObject* parent = 0);
    virtual ~BatchApplicationManager();

    void Destroy() override;
    bool Activate() override;

    ////////////////////////////////////////////////////
    ///MessageInfoBus::Listener interface///////////////
    void OnErrorMessage(const char* error) override;
    ///////////////////////////////////////////////////

    bool InitApplicationServer() override;

private:
    void Reflect() override;
    const char* GetLogBaseName() override;
    RegistryCheckInstructions PopupRegistryProblemsMessage(QString warningText) override;
    void InitSourceControl() override;
    void InitFileStateCache() override;

    void MakeActivationConnections() override;

    bool GetShouldExitOnIdle() const override { return true; }

    void TryScanProductDependencies() override;
    void TryHandleFileRelocation() override;
};
