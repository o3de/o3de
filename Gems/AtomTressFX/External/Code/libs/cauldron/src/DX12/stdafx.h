// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <SDKDDKVer.h>

#define NOMINMAX    
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <wrl.h>

// C RunTime Header Files
#include <malloc.h>
#include <tchar.h>
#include <cassert>

// GFX API
#include <D3D12.h>
#include <D3Dcompiler.h>
#include "..\d3d12x\d3dx12.h"

// math API
#include <DirectXMath.h>
using namespace DirectX;

#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <limits>
#include <algorithm>
#include <mutex>

#include "..\common\Misc\Error.h"
#include "base\UserMarkers.h"
#include "base\Helper.h"
#include "..\common\misc\misc.h"


// TODO: reference additional headers your program requires here
