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

#ifndef CRYINCLUDE_CRYCOMMON_TARRAY_H
#define CRYINCLUDE_CRYCOMMON_TARRAY_H
#pragma once


#include <ILog.h>
#include <Cry_Math.h>
#include <AzCore/IO/FileIO.h>

#ifndef CLAMP
#define CLAMP(X, mn, mx) ((X) < (mn) ? (mn) : ((X) < (mx) ? (X) : (mx)))
#endif

#ifndef SATURATE
#define SATURATE(X) clamp_tpl(X, 0.f, 1.f)
#endif

#ifndef SATURATEB
#define SATURATEB(X) CLAMP(X, 0, 255)
#endif

#ifndef LERP
#define LERP(A, B, Alpha) ((A) + (Alpha) * ((B)-(A)))
#endif

// Safe memory freeing
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)          { if (p) { delete (p);       (p) = NULL; } \
}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)    { if (p) { delete[] (p);     (p) = NULL; } \
}
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)         { if (p) { (p)->Release();   (p) = NULL; } \
}
#endif

#ifndef SAFE_RELEASE_FORCE
#define SAFE_RELEASE_FORCE(p)           { if (p) { (p)->ReleaseForce();  (p) = NULL; } \
}
#endif

#endif // CRYINCLUDE_CRYCOMMON_TARRAY_H
