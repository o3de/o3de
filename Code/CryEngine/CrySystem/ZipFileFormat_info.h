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

#ifndef CRYINCLUDE_CRYSYSTEM_ZIPFILEFORMAT_INFO_H
#define CRYINCLUDE_CRYSYSTEM_ZIPFILEFORMAT_INFO_H
#pragma once

#include "ZipFileFormat.h"

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Waddress-of-packed-member"
#endif

STRUCT_INFO_BEGIN(ZipFile::CDREnd)
VAR_INFO(lSignature)
VAR_INFO(nDisk)
VAR_INFO(nCDRStartDisk)
VAR_INFO(numEntriesOnDisk)
VAR_INFO(numEntriesTotal)
VAR_INFO(lCDRSize)
VAR_INFO(lCDROffset)
VAR_INFO(nCommentLength)
STRUCT_INFO_END(ZipFile::CDREnd)

STRUCT_INFO_BEGIN(ZipFile::DataDescriptor)
VAR_INFO(lCRC32)
VAR_INFO(lSizeCompressed)
VAR_INFO(lSizeUncompressed)
STRUCT_INFO_END(ZipFile::DataDescriptor)

STRUCT_INFO_BEGIN(ZipFile::CDRFileHeader)
VAR_INFO(lSignature)
VAR_INFO(nVersionMadeBy)
VAR_INFO(nVersionNeeded)
VAR_INFO(nFlags)
VAR_INFO(nMethod)
VAR_INFO(nLastModTime)
VAR_INFO(nLastModDate)
VAR_INFO(desc)
VAR_INFO(nFileNameLength)
VAR_INFO(nExtraFieldLength)
VAR_INFO(nFileCommentLength)
VAR_INFO(nDiskNumberStart)
VAR_INFO(nAttrInternal)
VAR_INFO(lAttrExternal)
VAR_INFO(lLocalHeaderOffset)
STRUCT_INFO_END(ZipFile::CDRFileHeader)

STRUCT_INFO_BEGIN(ZipFile::LocalFileHeader)
VAR_INFO(lSignature)
VAR_INFO(nVersionNeeded)
VAR_INFO(nFlags)
VAR_INFO(nMethod)
VAR_INFO(nLastModTime)
VAR_INFO(nLastModDate)
VAR_INFO(desc)
VAR_INFO(nFileNameLength)
VAR_INFO(nExtraFieldLength)
STRUCT_INFO_END(ZipFile::LocalFileHeader)

#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

#endif // CRYINCLUDE_CRYSYSTEM_ZIPFILEFORMAT_INFO_H
