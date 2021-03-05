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

// Header for adding a watermark to an exe, which can then be set
// by the external CryWaterMark program. To use, simply write:
//
// WATERMARKDATA(__blah);
//
// anywhere in the global scope in the program


#ifndef CRYINCLUDE_CRYSYSTEM_CRYWATERMARK_H
#define CRYINCLUDE_CRYSYSTEM_CRYWATERMARK_H
#pragma once


#define NUMMARKWORDS 10
#define WATERMARKDATA(name) unsigned int name[] = { 0xDEBEFECA, 0xFABECEDA, 0xADABAFBE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// (the name is such that you can have multiple watermarks in one exe, don't use
// names like "watermark" just incase you accidentally give out an exe with
// debug information).

#endif // CRYINCLUDE_CRYSYSTEM_CRYWATERMARK_H
