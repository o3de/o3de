/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

class CAutoLogTime
{
public:
    CAutoLogTime(const char* what);
    ~CAutoLogTime();
private:
    const char* m_what;
    AZ::s64 m_t0, m_t1;
};
