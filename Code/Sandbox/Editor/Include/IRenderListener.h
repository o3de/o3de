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
