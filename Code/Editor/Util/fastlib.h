/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

AZ_FORCE_INLINE int RoundFloatToInt(float fValue)
{
    return (int)(fValue + 0.5f);
}

AZ_FORCE_INLINE int FloatToIntRet(float fValue)
{
    return (int)(fValue + 0.5f);
}

AZ_FORCE_INLINE int ftoi(float fValue)
{
    return (int)(fValue);
}

AZ_FORCE_INLINE unsigned int ifloor(float fValue)
{
    return ftoi(floor(fValue));
}
