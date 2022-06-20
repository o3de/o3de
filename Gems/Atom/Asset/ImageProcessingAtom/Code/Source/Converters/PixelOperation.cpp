/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/std/smart_ptr/make_shared.h>

#include <Processing/ImageObjectImpl.h>
#include <Processing/ImageConvert.h>
#include <Processing/PixelFormatInfo.h>
#include <Converters/PixelOperation.h>

///////////////////////////////////////////////////////////////////////////////////
//functions for maintaining alpha coverage.

namespace ImageProcessingAtom
{
    //convertion: all data type supported by pixel channel <=> float
    float U8ToF32(uint8 in)
    {
        return in / 255.f;
    }

    uint8 F32ToU8(float in)
    {
        return static_cast<uint8>(round(AZ::GetClamp<float>(in, 0.f, 1.f) * 255.f));
    }

    float U16ToF32(uint16 in)
    {
        return in / 65535.f;
    }

    uint16 F32ToU16(float in)
    {
        return static_cast<uint16>(round(AZ::GetClamp<float>(in, 0.f, 1.f) * 65535.f));
    }

    float HalfToF32(SHalf in)
    {
        return in;
    }

    SHalf F32ToHalf(float in)
    {
        return SHalf(in);
    }

    //stucture for RGBE pixel format
    struct RgbE
    {
        static const int RGB9E5_EXPONENT_BITS = 5;
        static const int RGB9E5_MANTISSA_BITS = 9;
        static const int RGB9E5_EXP_BIAS = 15;
        static const int RGB9E5_MAX_VALID_BIASED_EXP = 31;
        static const int MAX_RGB9E5_EXP = (RGB9E5_MAX_VALID_BIASED_EXP - RGB9E5_EXP_BIAS);
        static const int RGB9E5_MANTISSA_VALUES = (1 << RGB9E5_MANTISSA_BITS);
        static const int MAX_RGB9E5_MANTISSA = (RGB9E5_MANTISSA_VALUES - 1);

        static float MAX_RGB9E5;

        unsigned int r : 9;
        unsigned int g : 9;
        unsigned int b : 9;
        unsigned int e : 5;

        static int log2(float x)
        {
            int bitfield = *((int*)(&x));
            bitfield &= ~0x80000000;

            return ((bitfield >> 23) - 127);
        }

        void GetRGBF(float& outR, float& outG, float& outB) const
        {
            int exponent = e - RGB9E5_EXP_BIAS - RGB9E5_MANTISSA_BITS;
            float scale = powf(2.0f, static_cast<float>(exponent));
            outR = r * scale;
            outG = g * scale;
            outB = b * scale;
        }

        void SetRGBF(const float& inR, const float& inG, const float& inB)
        {
            float rf = AZStd::GetMax(0.0f, AZStd::GetMin(inR, MAX_RGB9E5));
            float gf = AZStd::GetMax(0.0f, AZStd::GetMin(inG, MAX_RGB9E5));
            float bf = AZStd::GetMax(0.0f, AZStd::GetMin(inB, MAX_RGB9E5));
            float mf = AZStd::GetMax(rf, AZStd::GetMax(gf, bf));

            e = AZStd::GetMax(0, log2(mf) + (RGB9E5_EXP_BIAS + 1));

            int exponent = e - RGB9E5_EXP_BIAS - RGB9E5_MANTISSA_BITS;
            float scale = powf(2.0f, static_cast<float>(exponent));

            r = AZStd::GetMin(511, (int)floorf(rf / scale + 0.5f));
            g = AZStd::GetMin(511, (int)floorf(gf / scale + 0.5f));
            b = AZStd::GetMin(511, (int)floorf(bf / scale + 0.5f));
        }
    };

    float RgbE::MAX_RGB9E5 = (((float)MAX_RGB9E5_MANTISSA) / RGB9E5_MANTISSA_VALUES * (1 << MAX_RGB9E5_EXP));

    //ePixelFormat_R8G8B8A8
    class PixelOperationR8G8B8A8
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint8* data = buf;
            r = U8ToF32(data[0]);
            g = U8ToF32(data[1]);
            b = U8ToF32(data[2]);
            a = U8ToF32(data[3]);
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, const float& b, const float& a) override
        {
            uint8* data = buf;
            data[0] = F32ToU8(r);
            data[1] = F32ToU8(g);
            data[2] = F32ToU8(b);
            data[3] = F32ToU8(a);
        }
    };

    //ePixelFormat_R8G8B8X8
    class PixelOperationR8G8B8X8
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint8* data = buf;
            r = U8ToF32(data[0]);
            g = U8ToF32(data[1]);
            b = U8ToF32(data[2]);
            a = 1.f;
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, const float& b, [[maybe_unused]] const float& a) override
        {
            uint8* data = buf;
            data[0] = F32ToU8(r);
            data[1] = F32ToU8(g);
            data[2] = F32ToU8(b);
            data[3] = 0xff;
        }
    };

    //ePixelFormat_B8G8R8A8
    class PixelOperationB8G8R8A8
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint8* data = buf;
            r = U8ToF32(data[2]);
            g = U8ToF32(data[1]);
            b = U8ToF32(data[0]);
            a = U8ToF32(data[3]);
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, const float& b, const float& a) override
        {
            uint8* data = buf;
            data[0] = F32ToU8(b);
            data[1] = F32ToU8(g);
            data[2] = F32ToU8(r);
            data[3] = F32ToU8(a);
        }
    };


    //ePixelFormat_R8G8B8
    class PixelOperationR8G8B8
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint8* data = buf;
            r = U8ToF32(data[0]);
            g = U8ToF32(data[1]);
            b = U8ToF32(data[2]);
            a = 1.0f;
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, const float& b, [[maybe_unused]] const float& a) override
        {
            uint8* data = buf;
            data[0] = F32ToU8(r);
            data[1] = F32ToU8(g);
            data[2] = F32ToU8(b);
        }
    };

    //ePixelFormat_B8G8R8
    class PixelOperationB8G8R8
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint8* data = buf;
            r = U8ToF32(data[2]);
            g = U8ToF32(data[1]);
            b = U8ToF32(data[0]);
            a = 1.0f;
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, const float& b, [[maybe_unused]] const float& a) override
        {
            uint8* data = buf;
            data[0] = F32ToU8(b);
            data[1] = F32ToU8(g);
            data[2] = F32ToU8(r);
        }
    };

    //ePixelFormat_R8G8
    class PixelOperationR8G8
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint8* data = buf;
            r = U8ToF32(data[0]);
            g = U8ToF32(data[1]);
            b = 0.f;
            a = 1.f;
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, [[maybe_unused]] const float& b, [[maybe_unused]] const float& a) override
        {
            uint8* data = buf;
            data[0] = F32ToU8(r);
            data[1] = F32ToU8(g);
        }
    };

    //ePixelFormat_R8
    class PixelOperationR8
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint8* data = buf;
            r = U8ToF32(data[0]);
            g = r;
            b = r;
            a = 1.f;
        }

        void SetRGBA(uint8* buf, const float& r, [[maybe_unused]] const float& g, [[maybe_unused]] const float& b, [[maybe_unused]] const float& a) override
        {
            uint8* data = buf;
            data[0] = F32ToU8(r);
        }
    };

    //ePixelFormat_A8
    class PixelOperationA8
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint8* data = buf;
            a = U8ToF32(data[0]);
            //save alpha information to rgb too. useful for preview.
            r = a;
            g = a;
            b = a;
        }

        void SetRGBA(uint8* buf, [[maybe_unused]] const float& r, [[maybe_unused]] const float& g, [[maybe_unused]] const float& b, const float& a) override
        {
            uint8* data = buf;
            data[0] = F32ToU8(a);
        }
    };

    //ePixelFormat_R16G16B16A16
    class PixelOperationR16G16B16A16
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint16* data = (uint16*)(buf);
            r = U16ToF32(data[0]);
            g = U16ToF32(data[1]);
            b = U16ToF32(data[2]);
            a = U16ToF32(data[3]);
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, const float& b, const float& a) override
        {
            uint16* data = (uint16*)(buf);
            data[0] = F32ToU16(r);
            data[1] = F32ToU16(g);
            data[2] = F32ToU16(b);
            data[3] = F32ToU16(a);
        }
    };

    //ePixelFormat_R16G16
    class PixelOperationR16G16
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint16* data = (uint16*)(buf);
            r = U16ToF32(data[0]);
            g = U16ToF32(data[1]);
            b = 0.f;
            a = 1.f;
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, [[maybe_unused]] const float& b, [[maybe_unused]] const float& a) override
        {
            uint16* data = (uint16*)(buf);
            data[0] = F32ToU16(r);
            data[1] = F32ToU16(g);
        }
    };

    //ePixelFormat_R16
    class PixelOperationR16
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const uint16* data = (uint16*)(buf);
            r = U16ToF32(data[0]);
            g = r;
            b = r;
            a = 1.f;
        }

        void SetRGBA(uint8* buf, const float& r, [[maybe_unused]] const float& g, [[maybe_unused]] const float& b, [[maybe_unused]] const float& a) override
        {
            uint16* data = (uint16*)(buf);
            data[0] = F32ToU16(r);
        }
    };

    //ePixelFormat_R9G9B9E5
    class PixelOperationR9G9B9E5
        : public IPixelOperation
    {
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const RgbE* data = (RgbE*)(buf);
            data->GetRGBF(r, g, b);
            a = 1.f;
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, const float& b, [[maybe_unused]] const float& a) override
        {
            RgbE* data = (RgbE*)(buf);
            data->SetRGBF(r, g, b);
        }
    };

    //ePixelFormat_R32G32B32A32F
    class PixelOperationR32G32B32A32F
        : public IPixelOperation
    {
    public:
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const float* data = (float*)(buf);
            r = data[0];
            g = data[1];
            b = data[2];
            a = data[3];
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, const float& b, const float& a) override
        {
            float* data = (float*)(buf);
            data[0] = r;
            data[1] = g;
            data[2] = b;
            data[3] = a;
        }
    };

    //ePixelFormat_R32G32F
    class PixelOperationR32G32F
        : public IPixelOperation
    {
    public:
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const float* data = (float*)(buf);
            r = data[0];
            g = data[1];
            b = 0.f;
            a = 1.f;
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, [[maybe_unused]] const float& b, [[maybe_unused]] const float& a) override
        {
            float* data = (float*)(buf);
            data[0] = r;
            data[1] = g;
        }
    };

    //ePixelFormat_R32F
    class PixelOperationR32F
        : public IPixelOperation
    {
    public:
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const float* data = (float*)(buf);
            r = data[0];
            g = r;
            b = r;
            a = 1.f;
        }

        void SetRGBA(uint8* buf, const float& r, [[maybe_unused]] const float& g, [[maybe_unused]] const float& b, [[maybe_unused]] const float& a) override
        {
            float* data = (float*)(buf);
            data[0] = r;
        }
    };

    //ePixelFormat_R16G16B16A16F
    class PixelOperationR16G16B16A16F
        : public IPixelOperation
    {
    public:
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const SHalf* data = (SHalf*)(buf);
            r = data[0];
            g = data[1];
            b = data[2];
            a = data[3];
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, const float& b, const float& a) override
        {
            SHalf* data = (SHalf*)(buf);
            data[0] = SHalf(r);
            data[1] = SHalf(g);
            data[2] = SHalf(b);
            data[3] = SHalf(a);
        }
    };

    //ePixelFormat_R16G16F
    class PixelOperationR16G16F
        : public IPixelOperation
    {
    public:
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const SHalf* data = (SHalf*)(buf);
            r = data[0];
            g = data[1];
            b = 0.f;
            a = 1.f;
        }

        void SetRGBA(uint8* buf, const float& r, const float& g, [[maybe_unused]] const float& b, [[maybe_unused]] const float& a) override
        {
            SHalf* data = (SHalf*)(buf);
            data[0] = SHalf(r);
            data[1] = SHalf(g);
        }
    };

    //ePixelFormat_R16F
    class PixelOperationR16F
        : public IPixelOperation
    {
    public:
        void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) override
        {
            const SHalf* data = (SHalf*)(buf);
            r = data[0];
            g = r;
            b = r;
            a = 1.f;
        }

        void SetRGBA(uint8* buf, const float& r, [[maybe_unused]] const float& g, [[maybe_unused]] const float& b, [[maybe_unused]] const float& a) override
        {
            SHalf* data = (SHalf*)(buf);
            data[0] = SHalf(r);
        }
    };

    IPixelOperationPtr CreatePixelOperation(EPixelFormat pixelFmt)
    {
        switch (pixelFmt)
        {
        case ePixelFormat_R8G8B8A8:
            return AZStd::make_shared<PixelOperationR8G8B8A8>();
        case ePixelFormat_R8G8B8X8:
            return AZStd::make_shared<PixelOperationR8G8B8X8>();
        case ePixelFormat_B8G8R8A8:
            return AZStd::make_shared<PixelOperationB8G8R8A8>();
        case ePixelFormat_B8G8R8:
            return AZStd::make_shared<PixelOperationB8G8R8>();
        case ePixelFormat_R8G8B8:
            return AZStd::make_shared<PixelOperationR8G8B8>();
        case ePixelFormat_R8G8:
            return AZStd::make_shared<PixelOperationR8G8>();
        case ePixelFormat_R8:
            return AZStd::make_shared<PixelOperationR8>();
        case ePixelFormat_A8:
            return AZStd::make_shared<PixelOperationA8>();
        case ePixelFormat_R16G16B16A16:
            return AZStd::make_shared<PixelOperationR16G16B16A16>();
        case ePixelFormat_R16G16:
            return AZStd::make_shared<PixelOperationR16G16>();
        case ePixelFormat_R16:
            return AZStd::make_shared<PixelOperationR16>();
        case ePixelFormat_R9G9B9E5:
            return AZStd::make_shared<PixelOperationR9G9B9E5>();
        case ePixelFormat_R32G32B32A32F:
            return AZStd::make_shared<PixelOperationR32G32B32A32F>();
        case ePixelFormat_R32G32F:
            return AZStd::make_shared<PixelOperationR32G32F>();
        case ePixelFormat_R32F:
            return AZStd::make_shared<PixelOperationR32F>();
        case ePixelFormat_R16G16B16A16F:
            return AZStd::make_shared<PixelOperationR16G16B16A16F>();
        case ePixelFormat_R16G16F:
            return AZStd::make_shared<PixelOperationR16G16F>();
        case ePixelFormat_R16F:
            return AZStd::make_shared<PixelOperationR16F>();
        default:
            AZ_Assert(false, "This function should be only called for uncompressed pixel format");
            break;
        }
        return nullptr;
    }
} // namespace ImageProcessingAtom
