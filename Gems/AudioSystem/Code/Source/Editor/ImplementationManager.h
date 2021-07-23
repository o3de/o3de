/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <QObject>
#endif

namespace AudioControls
{
    class IAudioSystemEditor;
}

//-----------------------------------------------------------------------------------------------//
class CImplementationManager
    : public QObject
{
    Q_OBJECT

public:
    CImplementationManager();

    bool LoadImplementation();
    void Release();
    AudioControls::IAudioSystemEditor* GetImplementation();

signals:
    void ImplementationChanged();
};
