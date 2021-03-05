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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_ZIPDIR_ZIPFILEFORMAT_INFO_H
#define CRYINCLUDE_CRYCOMMONTOOLS_ZIPDIR_ZIPFILEFORMAT_INFO_H
#pragma once

#include "ZipFileFormat.h"

STRUCT_INFO_BEGIN(ZipFile::CDREnd)
STRUCT_VAR_INFO(lSignature, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(nDisk, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nCDRStartDisk, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(numEntriesOnDisk, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(numEntriesTotal, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(lCDRSize, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(lCDROffset, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(nCommentLength, TYPE_INFO(ZipFile::ushort))
STRUCT_INFO_END(ZipFile::CDREnd)

STRUCT_INFO_BEGIN(ZipFile::CDREnd_ZIP64)
STRUCT_VAR_INFO(lSignature, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(nExtDataLength, TYPE_INFO(ZipFile::uint64))
STRUCT_VAR_INFO(nVersionMadeBy, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nVersionNeeded, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nDisk, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(nCDRStartDisk, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(numEntriesOnDisk, TYPE_INFO(ZipFile::uint64))
STRUCT_VAR_INFO(numEntriesTotal, TYPE_INFO(ZipFile::uint64))
STRUCT_VAR_INFO(lCDRSize, TYPE_INFO(ZipFile::uint64))
STRUCT_VAR_INFO(lCDROffset, TYPE_INFO(ZipFile::uint64))
STRUCT_INFO_END(ZipFile::CDREnd_ZIP64)

STRUCT_INFO_BEGIN(ZipFile::CDRLocator_ZIP64)
STRUCT_VAR_INFO(lSignature, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(nCDR64StartDisk, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(lCDR64EndOffset, TYPE_INFO(ZipFile::uint64))
STRUCT_VAR_INFO(nDisks, TYPE_INFO(ZipFile::ulong))
STRUCT_INFO_END(ZipFile::CDRLocator_ZIP64)

STRUCT_INFO_BEGIN(ZipFile::DataDescriptor)
STRUCT_VAR_INFO(lCRC32, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(lSizeCompressed, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(lSizeUncompressed, TYPE_INFO(ZipFile::ulong))
STRUCT_INFO_END(ZipFile::DataDescriptor)

STRUCT_INFO_BEGIN(ZipFile::DataDescriptor_ZIP64)
STRUCT_VAR_INFO(lCRC32, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(lSizeCompressed, TYPE_INFO(ZipFile::uint64))
STRUCT_VAR_INFO(lSizeUncompressed, TYPE_INFO(ZipFile::uint64))
STRUCT_INFO_END(ZipFile::DataDescriptor_ZIP64)

STRUCT_INFO_BEGIN(ZipFile::CDRFileHeader)
STRUCT_VAR_INFO(lSignature, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(nVersionMadeBy, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nVersionNeeded, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nFlags, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nMethod, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nLastModTime, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nLastModDate, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(desc, TYPE_INFO(ZipFile::DataDescriptor))
STRUCT_VAR_INFO(nFileNameLength, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nExtraFieldLength, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nFileCommentLength, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nDiskNumberStart, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nAttrInternal, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(lAttrExternal, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(lLocalHeaderOffset, TYPE_INFO(ZipFile::ulong))
STRUCT_INFO_END(ZipFile::CDRFileHeader)

STRUCT_INFO_BEGIN(ZipFile::LocalFileHeader)
STRUCT_VAR_INFO(lSignature, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(nVersionNeeded, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nFlags, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nMethod, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nLastModTime, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nLastModDate, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(desc, TYPE_INFO(ZipFile::DataDescriptor))
STRUCT_VAR_INFO(nFileNameLength, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(nExtraFieldLength, TYPE_INFO(ZipFile::ushort))
STRUCT_INFO_END(ZipFile::LocalFileHeader)

STRUCT_INFO_BEGIN(ZipFile::ExtraFieldHeader)
STRUCT_VAR_INFO(headerID, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(dataSize, TYPE_INFO(ZipFile::ushort))
STRUCT_INFO_END(ZipFile::ExtraFieldHeader)

STRUCT_INFO_BEGIN(ZipFile::ExtraNTFSHeader)
STRUCT_VAR_INFO(reserved, TYPE_INFO(ZipFile::ulong))
STRUCT_VAR_INFO(attrTag, TYPE_INFO(ZipFile::ushort))
STRUCT_VAR_INFO(attrSize, TYPE_INFO(ZipFile::ushort))
STRUCT_INFO_END(ZipFile::ExtraNTFSHeader)

STRUCT_INFO_BEGIN(ZipFile::ExtraZIP64Data)
STRUCT_VAR_INFO(lSizeUncompressed, TYPE_INFO(ZipFile::uint64))
STRUCT_VAR_INFO(lSizeCompressed, TYPE_INFO(ZipFile::uint64))
STRUCT_VAR_INFO(lLocalHeaderOffset, TYPE_INFO(ZipFile::uint64))
STRUCT_VAR_INFO(nDiskNumberStart, TYPE_INFO(ZipFile::ulong))
STRUCT_INFO_END(ZipFile::ExtraZIP64Data)


#endif // CRYINCLUDE_CRYCOMMONTOOLS_ZIPDIR_ZIPFILEFORMAT_INFO_H
