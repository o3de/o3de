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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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

    static void Reflect(AZ::SerializeContext* serializeContext);
};

#endif // CRYINCLUDE_CRYMOVIE_CONSOLETRACK_H
