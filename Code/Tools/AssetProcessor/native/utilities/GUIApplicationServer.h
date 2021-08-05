/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <native/utilities/ApplicationServer.h>
#endif

/** This Class is responsible for listening and getting new connections
 */
class GUIApplicationServer
    : public ApplicationServer
{
    Q_OBJECT
public:
    explicit GUIApplicationServer(QObject* parent = 0);
    ~GUIApplicationServer() override;

    bool startListening(unsigned short port = 0) override;
};
