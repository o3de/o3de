/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/set.h>

namespace AssetProcessor
{
    class LfsPointerFileValidator
    {
    public:
        LfsPointerFileValidator();

        //! Check whether the specified file is an LFS pointer file.
        //! @param filePath Path to the file to check.
        //! @return Return true if the file is an LFS pointer file.
        bool IsLfsPointerFile(const AZStd::string& filePath);

        //! Retrieve the LFS pointer file path patterns.
        //! @return List of LFS pointer file path patterns.
        AZStd::set<AZStd::string> GetLfsPointerFilePathPatterns();

    private:
        //! Check whether the file content matches the LFS pointer file content rules.
        //! @param filePath Path to the file to check.
        //! @return Return true if the file content matches all the LFS pointer file content rules.
        bool CheckLfsPointerFileContent(const AZStd::string& filePath);

        //! Check whether the specified file path matches any of the known LFS pointer file path patterns.
        //! @param filePath Path to the file to check.
        //! @return Return true if the file matches any of the known LFS pointer file path patterns.
        bool CheckLfsPointerFilePathPattern(const AZStd::string& filePath);

        //! Read the .gitattributes file under the specified directory to retrieve the LFS pointer file path patterns.
        //! @param directory Directory to find the .gitattributes file.
        void ParseGitAttributesFile(const AZStd::string& directory);

        //! Collect the known LFS pointer file path patterns.
        void CollectLfsPointerFilePathPatterns();

        AZStd::set<AZStd::string> m_lfsPointerFilePatterns; //!< List of the known LFS pointer file path patterns.

    };
}
