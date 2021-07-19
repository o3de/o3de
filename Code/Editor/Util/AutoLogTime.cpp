/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "AutoLogTime.h"

CAutoLogTime::CAutoLogTime(const char* what)
{
    m_what = what;
    CLogFile::FormatLine("---- Start: %s", m_what);
    m_t0 = GetTickCount();
}

CAutoLogTime::~CAutoLogTime()
{
    m_t1 = GetTickCount();
    CLogFile::FormatLine("---- End: %s (%d seconds)", m_what, (m_t1 - m_t0) / 1000);
}
