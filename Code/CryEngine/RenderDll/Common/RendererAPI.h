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

// Description : Main header included by every file in Renderer.

#pragma once

#if defined(RENDERER_API)
#error RENDERER_API should only be defined in this header
#endif

#if defined(RENDERER_EXPORTS)
    #define RENDERER_API __declspec(dllexport)
    #define RENDERER_API_EXTERN
#else
    #define RENDERER_API __declspec(dllimport)
    #define RENDERER_API_EXTERN extern
#endif

