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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_COLLADAWRITER_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_COLLADAWRITER_H
#pragma once


#include <string>

class IExportSource;
class IExportContext;
class ProgressRange;
class IXMLSink;

class ColladaWriter
{
public:
    static bool Write(IExportSource* source, IExportContext* context, IXMLSink* sink, ProgressRange& progressRange);
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_COLLADAWRITER_H
