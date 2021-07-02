/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Dummy font implementation (dedicated server)

#include <AtomLyIntegration/AtomFont/AtomFont_precompiled.h>

#if defined(USE_NULLFONT)

#include <AtomLyIntegration/AtomFont//AtomNullFont.h>

AZ::AtomNullFFont AZ::AtomNullFont::NullFFont;

#endif // USE_NULLFONT
