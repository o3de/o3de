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

#pragma once

#if defined(WIN32)
  #include <objbase.h>
  #include <mmsyscom.h>
  #include <d3d11.h>
  #include <dxgi.h>
  #include <dxgi1_2.h>
#endif

#if !defined(D3D_OK)
#define D3D_OK 0
#endif

#if !defined(S_OK)
#define S_OK 0
#endif
