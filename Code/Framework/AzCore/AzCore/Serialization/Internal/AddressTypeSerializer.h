/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/AZStdContainers.inl>
namespace AZ
{
    inline namespace DataPatchInternal
    {
        /**
        * Custom serializer for our address type, as we want to be more space efficient and not store every element of the container
        * separately.
        */
        class AddressTypeSerializer
            : public AZ::Internal::AZBinaryData
        {
        public:

            AddressTypeElement LoadAddressElementFromPath(const AZStd::string& pathElement) const;

        private:
            bool Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian = false) override;

            void LoadElementFinalize(AddressTypeElement& addressElement, const AZStd::string& pathElement, const AZStd::string& version) const;
            void LoadLegacyElement(AddressTypeElement& addressElement, const AZStd::string& pathElement, const size_t pathDelimLength) const;

            size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian = false) override;

            size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian) override;

            size_t TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override;

            bool CompareValueData(const void* lhs, const void* rhs) override;
        };
    }
}   // namespace AZ
