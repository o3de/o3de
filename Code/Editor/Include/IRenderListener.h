/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Interface for rendering custom 3D elements in the main
//               render viewport. Particularly usefull for debug geometries.


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IRENDERLISTENER_H
#define CRYINCLUDE_EDITOR_INCLUDE_IRENDERLISTENER_H
#pragma once


struct DisplayContext;

struct IRenderListener
    : public IUnknown
{
    DEFINE_UUID(0x8D52F857, 0x1027, 0x4346, 0xAC, 0x7B, 0xF6, 0x20, 0xDA, 0x7C, 0xCE, 0x42)

    virtual void Render(DisplayContext& rDisplayContext) = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IRENDERLISTENER_H
