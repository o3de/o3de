/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/UnitTest/TestTypes.h>

#include <Compression/DecompressionInterfaceAPI.h>
#include <Clients/DecompressionRegistrarImpl.h>

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

namespace CompressionTest
{
    static constexpr Compression::CompressionAlgorithmId GetTestCompressionAlgorithmId()
    {
        return Compression::CompressionAlgorithmId(static_cast<AZ::u32>(AZStd::hash<AZStd::string_view>{}("TestCompressor")));
    }

    class TestDecompressor
        : public Compression::IDecompressionInterface
    {
    public:
        TestDecompressor() = default;

        //! Retrieves the 32-bit compression algorithm ID associated with this interface
        Compression::CompressionAlgorithmId GetCompressionAlgorithmId() const override
        {
            return GetTestCompressionAlgorithmId();
        }

        AZStd::string_view GetCompressionAlgorithmName() const override
        {
            return "TestCompressor";
        }

        //! Compresses the uncompressed data into the compressed buffer
        //! @return a CompressionResultData instance to indicate if compression operation has succeeded
        [[nodiscard]] Compression::DecompressionResultData DecompressBlock(
            [[maybe_unused]] AZStd::span<AZStd::byte> uncompressedBuffer,
            [[maybe_unused]] const AZStd::span<const AZStd::byte>& compressedData,
            [[maybe_unused]] const Compression::DecompressionOptions& compressionOptions = {}) const override
        {
            return Compression::DecompressionResultData{};
        }

    };
    class DecompressionRegistrarFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:
        DecompressionRegistrarFixture()
        {
            m_decompressionRegistrar = AZStd::make_unique<Compression::DecompressionRegistrarImpl>();
            Compression::DecompressionRegistrar::Register(m_decompressionRegistrar.get());

            // Register the lz4 compressor with the compression registrar
            if (auto compressionRegistrar = Compression::DecompressionRegistrar::Get();
                compressionRegistrar != nullptr)
            {
                auto compressionAlgorithmId = GetTestCompressionAlgorithmId();
                auto compressorLz4 = AZStd::make_unique<TestDecompressor>();
                auto registerOutcome = compressionRegistrar->RegisterDecompressionInterface(
                    compressionAlgorithmId,
                    AZStd::move(compressorLz4));

                EXPECT_TRUE(registerOutcome);
            }
        }

        ~DecompressionRegistrarFixture()
        {
            if (auto compressionRegistrar = Compression::DecompressionRegistrar::Get();
                compressionRegistrar != nullptr)
            {
                auto compressionAlgorithmId = GetTestCompressionAlgorithmId();
                bool unregisterOutcome = compressionRegistrar->UnregisterDecompressionInterface(
                    compressionAlgorithmId);

                EXPECT_TRUE(unregisterOutcome);
            }

            Compression::DecompressionRegistrar::Unregister(m_decompressionRegistrar.get());
        }

    protected:
        AZStd::unique_ptr<Compression::DecompressionRegistrarInterface> m_decompressionRegistrar;
    };

    TEST_F(DecompressionRegistrarFixture, CompressorRegistration_Succeeds)
    {
        Compression::DecompressionRegistrarInterface* decompressionRegistrar = Compression::DecompressionRegistrar::Get();
        ASSERT_NE(nullptr, decompressionRegistrar);
        Compression::IDecompressionInterface* decompressionInterface = decompressionRegistrar->FindDecompressionInterface(GetTestCompressionAlgorithmId());
        ASSERT_NE(nullptr, decompressionInterface);
    }
}
