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

#ifndef CRYINCLUDE_CRYMOVIE_COMMENTTRACK_H
#define CRYINCLUDE_CRYMOVIE_COMMENTTRACK_H
#pragma once


#include "IMovieSystem.h"
#include "AnimTrack.h"

class CCommentTrack
    : public TAnimTrack<ICommentKey>
{
public:
    AZ_CLASS_ALLOCATOR(CCommentTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CCommentTrack, "{A28FE42D-5B42-4E47-9813-4290D275D5A9}", IAnimTrack);

    //-----------------------------------------------------------------------------
    //!
    CCommentTrack();

    //-----------------------------------------------------------------------------
    //! IAnimTrack Method Overriding.
    //-----------------------------------------------------------------------------
    virtual void GetKeyInfo(int key, const char*& description, float& duration);
    virtual void SerializeKey(ICommentKey& key, XmlNodeRef& keyNode, bool bLoading);

    //-----------------------------------------------------------------------------
    //!
    void ValidateKeyOrder() { CheckValid(); }

    static void Reflect(AZ::SerializeContext* serializeContext);
};

#endif // CRYINCLUDE_CRYMOVIE_COMMENTTRACK_H
