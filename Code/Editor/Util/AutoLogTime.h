/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_AUTOLOGTIME_H
#define CRYINCLUDE_EDITOR_UTIL_AUTOLOGTIME_H
#pragma once


class CAutoLogTime
{
public:
    CAutoLogTime(const char* what);
    ~CAutoLogTime();
private:
    const char* m_what;
    int m_t0, m_t1;
};

#endif // CRYINCLUDE_EDITOR_UTIL_AUTOLOGTIME_H
