/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include "RemoteConsole.h"

#ifdef USE_REMOTE_CONSOLE
    #include "RemoteConsole_impl.inl"
#else
    #include "RemoteConsole_none.inl"
#endif
