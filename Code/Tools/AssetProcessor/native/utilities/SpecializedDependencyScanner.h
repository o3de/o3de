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
#pragma once
#include <AzCore/Math/Crc.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace IO
    {
        class GenericStream;
    }
}

namespace AssetProcessor
{
    class PotentialDependencies;
    class SpecializedDependencyScanner : public AZStd::enable_shared_from_this<SpecializedDependencyScanner>
    {
    public:
        virtual ~SpecializedDependencyScanner() {}

        virtual bool ScanFileForPotentialDependencies(AZ::IO::GenericStream& fileStream, PotentialDependencies& potentialDependencies, int maxScanIteration) = 0;
        virtual bool DoesScannerMatchFileData(AZ::IO::GenericStream& fileStream) = 0;
        virtual bool DoesScannerMatchFileExtension(const AZStd::string& fullPath) = 0;

        virtual AZStd::string GetVersion() const = 0;
        virtual AZStd::string GetName() const = 0;

        virtual AZ::Crc32 GetScannerCRC() const = 0;
    };
}
