/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in
// C++17. Use std::allocator_traits instead of accessing these members directly. You can define
// _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received
// this warning.
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/memory/stl/AWSString.h>
AZ_POP_DISABLE_WARNING
#include <AzCore/Android/Utils.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>

namespace AWSNativeSDKInit
{
    namespace Platform
    {
        void CopyCaCertBundle()
        {
            AZStd::vector<char> contents;
            AZStd::string certificatePath = "@products@/certificates/aws/cacert.pem";
            AZStd::string publicStoragePath = AZ::Android::Utils::GetAppPublicStoragePath();
            publicStoragePath.append("/certificates/aws/cacert.pem");

            AZ::IO::FileIOBase* fileBase = AZ::IO::FileIOBase::GetInstance();

            if (!fileBase->Exists(certificatePath.c_str()))
            {
                AZ_Error("AWSNativeSDKInit", false, "Certificate File(%s) does not exist.\n", certificatePath.c_str());
            }

            AZ::IO::HandleType fileHandle;
            AZ::IO::Result fileResult = fileBase->Open(certificatePath.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle);

            if (!fileResult)
            {
                AZ_Error("AWSNativeSDKInit", false, "Failed to open certificate file with result %i\n", fileResult.GetResultCode());
#if defined(CARBONATED)
                return;
#endif
            }

            AZ::u64 fileSize = 0;
            fileBase->Size(fileHandle, fileSize);

            if (fileSize == 0)
            {
                AZ_Error("AWSNativeSDKInit", false, "Given empty file(%s) as the certificate bundle.\n", certificatePath.c_str());
#if defined(CARBONATED)
                fileBase->Close(fileHandle);
                return;
#endif
            }

            contents.resize(fileSize + 1);
            fileResult = fileBase->Read(fileHandle, contents.data(), fileSize);
#if defined(CARBONATED)
            fileBase->Close(fileHandle);
#endif
            if (!fileResult)
            {
                AZ_Error(
                    "AWSNativeSDKInit", false, "Failed to read from the certificate bundle(%s) with result code(%i).\n", certificatePath.c_str(),
                    fileResult.GetResultCode());
#if defined(CARBONATED)
                return;
#endif
            }

            AZ_Printf("AWSNativeSDKInit", "Certificate bundle is read successfully from %s", certificatePath.c_str());

            AZ::IO::HandleType outFileHandle;
#if defined(CARBONATED)
            AZ::IO::Result outFileResult = fileBase->Open(publicStoragePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath, outFileHandle);
#else
            AZ::IO::Result outFileResult = fileBase->Open(publicStoragePath.c_str(), AZ::IO::OpenMode::ModeWrite, outFileHandle);
#endif

            if (!outFileResult)
            {
#if defined(CARBONATED)
                AZ_Error("AWSNativeSDKInit", false, "Failed to open the certificate bundle in %s with result %i\n", publicStoragePath.c_str(), fileResult.GetResultCode());
                return;
#else
                AZ_Error("AWSNativeSDKInit", false, "Failed to open the certificate bundle with result %i\n", fileResult.GetResultCode());
#endif
            }

            AZ::IO::Result writeFileResult = fileBase->Write(outFileHandle, contents.data(), fileSize);
            if (!writeFileResult)
            {
#if defined(CARBONATED)
                AZ_Error("AWSNativeSDKInit", false, "Failed to write the certificate bundle in %s with result %i\n", publicStoragePath.c_str(), writeFileResult.GetResultCode());
#else
                AZ_Error("AWSNativeSDKInit", false, "Failed to write the certificate bundle with result %i\n", writeFileResult.GetResultCode());
#endif
            }

#if defined(CARBONATED)
            // Removed - closed earlier
#else
            fileBase->Close(fileHandle);
#endif
            fileBase->Close(outFileHandle);

            AZ_Printf("AWSNativeSDKInit", "Certificate bundle successfully copied to %s", publicStoragePath.c_str());
        }
    } // namespace Platform
}
