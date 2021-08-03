/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
