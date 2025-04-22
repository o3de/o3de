/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/PlatformDef.h>

// It is vital that at least some Qt header is pulled in to this header, so that
// QT_VERSION is defined and the editor gets the correct set of overloads in the
// IXML interface. This also disambiguates the GUID/REFGUID situation in Guid.h
#include <QUuid>

#include <platform.h>

#include "ProjectDefines.h"

#ifdef NOMINMAX
#include "Cry_Math.h"
#endif //NOMINMAX

// Resource includes
#include "Resource.h"


#include <Include/SandboxAPI.h>
#include <Include/EditorCoreAPI.h>

//////////////////////////////////////////////////////////////////////////
// Simple type definitions.
//////////////////////////////////////////////////////////////////////////
#include "BaseTypes.h"

// Which cfg file to use.
#define EDITOR_CFG_FILE "editor.cfg"

//////////////////////////////////////////////////////////////////////////
// C runtime lib includes
//////////////////////////////////////////////////////////////////////////
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cfloat>
#include <climits>

/////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////
#include <memory>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <functional>

#undef rand // remove cryengine protection against wrong rand usage

/////////////////////////////////////////////////////////////////////////////
// VARIOUS MACROS AND DEFINES
/////////////////////////////////////////////////////////////////////////////
#ifdef new
#undef new
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)          { if (p) { delete (p);       (p) = nullptr; } \
}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)    { if (p) { delete[] (p);     (p) = nullptr; } \
}
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)         { if (p) { (p)->Release();   (p) = nullptr; } \
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <platform.h>
#include <Cry_Math.h>
#include <Cry_Geo.h>
#include <Range.h>
#include <StlUtils.h>

#include <smartptr.h>
#define TSmartPtr _smart_ptr

#define TOOLBAR_TRANSPARENT_COLOR QColor(0xC0, 0xC0, 0xC0)

/////////////////////////////////////////////////////////////////////////////
// Interfaces ///////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <IRenderer.h>
#include <CryFile.h>
#include <ISystem.h>
#include <IIndexedMesh.h>
#include <IXml.h>
#include <IMovieSystem.h>

//////////////////////////////////////////////////////////////////////////
// Commonly used Editor includes.
//////////////////////////////////////////////////////////////////////////

// Utility classes.
#include "Util/EditorUtils.h"
#include "Util/FileEnum.h"
#include <Editor/Util/EditorUtils.h>
#include <CryCommon/Cry_GeoIntersect.h>
#include "Util/AffineParts.h"

// Xml support.
#include "Util/XmlArchive.h"
#include "Util/XmlTemplate.h"

// Utility classes.
#include "Util/RefCountBase.h"
#include "Util/MemoryBlock.h"
#include "Util/PathUtil.h"

// Main Editor interface definition.
#include "IEditor.h"

// Log file access
#include "LogFile.h"

// Prevent macros to override certain function names
#ifdef GetObject
#undef GetObject
#endif

#ifdef max
#undef max
#undef min
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef RemoveDirectory
#undef RemoveDirectory
#endif

#ifdef CopyFile
#undef CopyFile
#endif

#ifdef GetUserName
#undef GetUserName
#endif

#ifdef LoadCursor
#undef LoadCursor
#endif
