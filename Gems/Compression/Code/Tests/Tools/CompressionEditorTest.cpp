/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/UnitTest/TestTypes.h>

#include <Compression/CompressionInterfaceAPI.h>
#include <Tools/CompressionRegistrarImpl.h>

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

namespace CompressionTest
{
    static constexpr Compression::CompressionAlgorithmId GetTestCompressionAlgorithmId()
    {
        return Compression::CompressionAlgorithmId(static_cast<AZ::u32>(AZStd::hash<AZStd::string_view>{}("TestCompressor")));
    }

    class TestCompressor
        : public Compression::ICompressionInterface
    {
    public:
        TestCompressor() = default;

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
        [[nodiscard]] Compression::CompressionResultData CompressBlock(
            [[maybe_unused]] AZStd::span<AZStd::byte> compressedBuffer,
            [[maybe_unused]] const AZStd::span<const AZStd::byte>& uncompressedData,
            [[maybe_unused]] const Compression::CompressionOptions& compressionOptions = {}) const override
        {
            return Compression::CompressionResultData{};
        }

        [[nodiscard]] size_t CompressBound([[maybe_unused]] size_t uncompressedBufferSize) const override
        {
            return 0;
        }
    };
    class CompressionRegistrarFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:
        CompressionRegistrarFixture()
        {
            m_compressionRegistrar = AZStd::make_unique<Compression::CompressionRegistrarImpl>();
            Compression::CompressionRegistrar::Register(m_compressionRegistrar.get());

            // Register the lz4 compressor with the compression registrar
            if (auto compressionRegistrar = Compression::CompressionRegistrar::Get();
                compressionRegistrar != nullptr)
            {
                auto compressionAlgorithmId = GetTestCompressionAlgorithmId();
                auto compressorLz4 = AZStd::make_unique<TestCompressor>();
                auto registerOutcome = compressionRegistrar->RegisterCompressionInterface(
                    compressionAlgorithmId,
                    AZStd::move(compressorLz4));

                EXPECT_TRUE(registerOutcome);
            }
        }

        ~CompressionRegistrarFixture()
        {
            if (auto compressionRegistrar = Compression::CompressionRegistrar::Get();
                compressionRegistrar != nullptr)
            {
                auto compressionAlgorithmId = GetTestCompressionAlgorithmId();
                bool unregisterOutcome = compressionRegistrar->UnregisterCompressionInterface(
                    compressionAlgorithmId);

                EXPECT_TRUE(unregisterOutcome);
            }

            Compression::CompressionRegistrar::Unregister(m_compressionRegistrar.get());
        }

    protected:
        AZStd::unique_ptr<Compression::CompressionRegistrarInterface> m_compressionRegistrar;
    };

    TEST_F(CompressionRegistrarFixture, CompressorRegistration_Succeeds)
    {
        Compression::CompressionRegistrarInterface* compressionRegistrar = Compression::CompressionRegistrar::Get();
        ASSERT_NE(nullptr, compressionRegistrar);
        Compression::ICompressionInterface* compressionInterface = compressionRegistrar->FindCompressionInterface(GetTestCompressionAlgorithmId());
        ASSERT_NE(nullptr, compressionInterface);
    }
}
