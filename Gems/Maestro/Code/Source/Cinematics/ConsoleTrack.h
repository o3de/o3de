/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_CONSOLETRACK_H
#define CRYINCLUDE_CRYMOVIE_CONSOLETRACK_H
#pragma once

//forward declarations.
#include "IMovieSystem.h"
#include "AnimTrack.h"
#include "AnimKey.h"

/** EntityTrack contains entity keys, when time reach event key, it fires script event or start animation etc...
*/
class CConsoleTrack
    : public TAnimTrack<IConsoleKey>
{
public:
    AZ_CLASS_ALLOCATOR(CConsoleTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CConsoleTrack, "{5D61289C-DE66-40E6-8C2D-A6CBF41A6EF4}", IAnimTrack);

    //////////////////////////////////////////////////////////////////////////
    // Overrides of IAnimTrack.
    //////////////////////////////////////////////////////////////////////////
    void GetKeyInfo(int key, const char*& description, float& duration);
    void SerializeKey(IConsoleKey& key, XmlNodeRef& keyNode, bool bLoading);

    static void Reflect(AZ::ReflectContext* context);
};

#endif // CRYINCLUDE_CRYMOVIE_CONSOLETRACK_H
