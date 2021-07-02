/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_FILETYPEUTILS_H
#define CRYINCLUDE_EDITOR_FILETYPEUTILS_H
#pragma once

// returns true if the given file path represents a file
// that can be previewed in Preview mode in Editor (e.g. cgf, cga etc.)
extern bool IsPreviewableFileType(const char* szPath);

#endif // CRYINCLUDE_EDITOR_FILETYPEUTILS_H
