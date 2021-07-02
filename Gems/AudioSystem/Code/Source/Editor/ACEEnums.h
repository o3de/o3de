/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <qnamespace.h>

enum EItemType
{
    eIT_FOLDER = 0,
    eIT_AUDIO_CONTROL,
    eIT_INVALID,
};

enum EDataRole
{
    eDR_TYPE = Qt::UserRole + 1,
    eDR_ID,
    eDR_MODIFIED,
};

enum EMiddlewareDataRole
{
    eMDR_TYPE = Qt::UserRole + 1,
    eMDR_ID,
    eMDR_LOCALIZED,
    eMDR_CONNECTED,
};
