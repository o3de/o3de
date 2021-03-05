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

#if !defined(Q_MOC_RUN)
#include <native/utilities/ApplicationServer.h>
#endif

/** This Class is responsible for listening and getting new connections
 */
class BatchApplicationServer
    : public ApplicationServer
{
    Q_OBJECT
public:
    explicit BatchApplicationServer(QObject* parent = 0);
    ~BatchApplicationServer() override;

    bool startListening(unsigned short port = 0) override;
};
