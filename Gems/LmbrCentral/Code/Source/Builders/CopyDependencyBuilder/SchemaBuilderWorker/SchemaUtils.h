/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Asset/XmlSchemaAsset.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace CopyDependencyBuilder
{
    bool SourceFileDependsOnSchema(const AzFramework::XmlSchemaAsset& schemaAsset, const AZStd::string& sourceFilePath);
}
