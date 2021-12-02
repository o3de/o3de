//---------------------------------------------------------------------------------------
// Main header file for TressFX
// This will eventually contain the C interface to all functionality.
//
// In the meantime, users need to use the individual headers for the components they 
// require.
//-------------------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once 
//#ifndef AMD_TRESSFX_H
//#define AMD_TRESSFX_H

#define AMD_TRESSFX_VERSION_MAJOR                    4
#define AMD_TRESSFX_VERSION_MINOR                    0
#define AMD_TRESSFX_VERSION_PATCH                    0

#include <TressFX/AMD_Types.h>

#ifndef TRESSFX_ASSERT
#include <assert.h>
#define TRESSFX_ASSERT assert
#endif

// Max number of bones in a skeleton
#ifndef AMD_TRESSFX_MAX_NUM_BONES
    #define AMD_TRESSFX_MAX_NUM_BONES 512
#endif

// Max number of hair render groups (bump up as needed)
#ifndef AMD_TRESSFX_MAX_HAIR_GROUP_RENDER
    #define AMD_TRESSFX_MAX_HAIR_GROUP_RENDER 16
#endif

//#endif // AMD_TRESSFX_H
