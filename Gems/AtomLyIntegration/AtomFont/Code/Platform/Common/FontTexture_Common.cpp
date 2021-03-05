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

#include <AtomLyIntegration/AtomFont/AtomFont_precompiled.h>

#if !defined(USE_NULLFONT_ALWAYS)
#include <AtomLyIntegration/AtomFont/FontTexture.h>

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::WriteToFile([[maybe_unused]] const string& fileName)
{
    return 1;
}

#endif
