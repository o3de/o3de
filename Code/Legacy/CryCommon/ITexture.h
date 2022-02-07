/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>

// Texture formats
enum ETEX_Format : AZ::u8
{
    eTF_Unknown = 0,
    eTF_R8G8B8A8 = 2, // may be saved into file

    eTF_A8 = 4,
    eTF_R8,
    eTF_R8S,
    eTF_R16,
    eTF_R16F,
    eTF_R32F,
    eTF_R8G8,
    eTF_R8G8S,
    eTF_R16G16,

    eTF_MaxFormat               // unused, must be always the last in the list
};

struct SDepthTexture;

//////////////////////////////////////////////////////////////////////
// Texture object interface
class ITexture
{
protected:
    virtual ~ITexture() {}
public:
    virtual int AddRef() = 0;
    virtual int Release() = 0;
    virtual int ReleaseForce() = 0;

    virtual const char* GetName() const = 0;
};

//=========================================================================================
