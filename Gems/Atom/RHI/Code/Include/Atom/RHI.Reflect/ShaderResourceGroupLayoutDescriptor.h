/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI.Reflect/Interval.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    //! A "ShaderInput" describes an input into a ShaderResourceGroup. Shader inputs are comprised of
    //! Buffers, Images, Samplers, and Constants. The former three shader inputs each contain an array
    //! of their respective resources. All of the resources in a shader input are identical with respect
    //! to their usage and type. Each of the {Buffer, Image, Sampler} inputs map directly to a variable
    //! definition in the shader source file.
    //!
    //! Constants are a bit different. Each constant input maps to a named constant variable in the
    //! shader resource group's implicit constant buffer. However, instead of a 'resource group' of
    //! constants, the constants occupy disjoint byte regions in an internal constant buffer.
    //!
    //! Each shader input has an id which is used to reflect the input by name.
    enum class ShaderInputType : uint32_t
    {
        Buffer = 0,
        Image,
        Sampler,
        Constant,
        Count
    };

    static const uint32_t ShaderInputTypeCount = static_cast<uint32_t>(ShaderInputType::Count);

    enum class ShaderInputBufferAccess : uint32_t
    {
        Constant = 0,
        Read,
        ReadWrite
    };

    enum class ShaderInputBufferType : uint32_t
    {
        Unknown = 0,
        Constant,
        Structured,
        Typed,
        Raw,
        AccelerationStructure
    };

    static const uint32_t UndefinedRegisterSlot = static_cast<uint32_t>(-1);

    class ShaderInputBufferDescriptor final
    {
    public:
        AZ_TYPE_INFO(ShaderInputBufferDescriptor, "{19D329BD-FCE7-43CC-A376-E2BD43EA5175}");
        static void Reflect(ReflectContext* context);

        ShaderInputBufferDescriptor() = default;
        ShaderInputBufferDescriptor(
            const Name& name,
            ShaderInputBufferAccess access,
            ShaderInputBufferType type,
            uint32_t bufferCount,
            uint32_t strideSize,
            uint32_t registerId,
            uint32_t spaceId);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        //! The name id used to reflect the buffer input.
        Name m_name;

        //! The type of the buffer for all array elements in the buffer input.
        ShaderInputBufferType m_type = ShaderInputBufferType::Unknown;

        //! How the array elements in the buffer input are accessed.
        ShaderInputBufferAccess m_access = ShaderInputBufferAccess::Read;

        //! Number of buffers array elements.
        uint32_t m_count = 0;

        //! Size of each buffer array element.
        uint32_t m_strideSize = 0;
            
        //! Register id of the resource in the SRG.
        //! This is only valid if the platform compiles the SRGs using "spaces".
        //! If not, this same information will be in the PipelineLayoutDescriptor.
        //! Some platforms (like Vulkan) need the register number when creating the
        //! SRG, others need it when creating the PipelineLayout.
        uint32_t m_registerId = UndefinedRegisterSlot;

        //! Logical Register Space that the register id is within.
        //! This is primarily used when an SRG contains one or more unbounded arrays,
        //! as an unbounded array contains all register ids in a register space.
        //! If an SRG doesn't contain any unbounded arrays all resources in it 
        //! will use the same space id.
        uint32_t m_spaceId = UndefinedRegisterSlot;
    };

    enum class ShaderInputImageAccess : uint32_t
    {
        Read = 0,
        ReadWrite
    };

    enum class ShaderInputImageType : uint32_t
    {
        Unknown = 0,
        Image1D,
        Image1DArray,
        Image2D,
        Image2DArray,
        Image2DMultisample,
        Image2DMultisampleArray,
        Image3D,
        ImageCube,
        ImageCubeArray,
        SubpassInput
    };

    class ShaderInputImageDescriptor final
    {
    public:
        AZ_TYPE_INFO(ShaderInputImageDescriptor, "{913DBF3C-5556-4524-B928-174A42516D31}");
        static void Reflect(ReflectContext* context);

        ShaderInputImageDescriptor() = default;
        ShaderInputImageDescriptor(
            const Name& name,
            ShaderInputImageAccess access,
            ShaderInputImageType type,
            uint32_t imageCount,
            uint32_t registerId,
            uint32_t spaceId);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        //! The name id used to reflect the image input.
        Name m_name;

        //! The type of image required for this shader input.
        ShaderInputImageType m_type = ShaderInputImageType::Unknown;

        //! How the array elements in the image input are accessed.
        ShaderInputImageAccess m_access = ShaderInputImageAccess::Read;

        //! Number of images array elements.
        uint32_t m_count = 0;
            
        //! Register id of the resource in the SRG.
        //! This is only valid if the platform compiles the SRGs using "spaces".
        //! If not, this same information will be in the PipelineLayoutDescriptor.
        //! Some platforms (like Vulkan) need the register number when creating the
        //! SRG, others need it when creating the PipelineLayout.
        uint32_t m_registerId = UndefinedRegisterSlot;

        //! Logical Register Space that the register id is within.
        //! This is primarily used when an SRG contains one or more unbounded arrays,
        //! as an unbounded array contains all register ids in a register space.
        //! If an SRG doesn't contain any unbounded arrays all resources in it 
        //! will use the same space id.
        uint32_t m_spaceId = UndefinedRegisterSlot;
    };

    class ShaderInputBufferUnboundedArrayDescriptor final
    {
    public:
        AZ_TYPE_INFO(ShaderInputBufferUnboundedArrayDescriptor, "{7B355E06-DABA-4F49-834E-DEA26691C8DF}");
        static void Reflect(ReflectContext* context);

        ShaderInputBufferUnboundedArrayDescriptor() = default;
        ShaderInputBufferUnboundedArrayDescriptor(
            const Name& name,
            ShaderInputBufferAccess access,
            ShaderInputBufferType type,
            uint32_t strideSize,
            uint32_t registerId,
            uint32_t spaceId);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        //! The name id used to reflect the buffer input.
        Name m_name;

        //! The type of buffer required for this shader input.
        ShaderInputBufferType m_type = ShaderInputBufferType::Unknown;

        //! How the array elements in the unbounded array input are accessed.
        ShaderInputBufferAccess m_access = ShaderInputBufferAccess::Read;

        //! Size of each buffer array element.
        uint32_t m_strideSize = 0;

        //! Register id of the resource in the SRG.
        //! This is only valid if the platform compiles the SRGs using "spaces".
        //! If not, this same information will be in the PipelineLayoutDescriptor.
        //! Some platforms (like Vulkan) need the register number when creating the
        //! SRG, others need it when creating the PipelineLayout.
        uint32_t m_registerId = UndefinedRegisterSlot;

        //! Logical Register Space that the register id is within.
        //! This is primarily used when an SRG contains one or more unbounded arrays,
        //! as an unbounded array contains all register ids in a register space.
        //! If an SRG doesn't contain any unbounded arrays all resources in it 
        //! will use the same space id.
        uint32_t m_spaceId = UndefinedRegisterSlot;
    };

    class ShaderInputImageUnboundedArrayDescriptor final
    {
    public:
        AZ_TYPE_INFO(ShaderInputImageUnboundedArrayDescriptor, "{943E4C4A-E5FE-4993-93D5-EFB67565284B}");
        static void Reflect(ReflectContext* context);

        ShaderInputImageUnboundedArrayDescriptor() = default;
        ShaderInputImageUnboundedArrayDescriptor(
            const Name& name,
            ShaderInputImageAccess access,
            ShaderInputImageType type,
            uint32_t registerId,
            uint32_t spaceId);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        //! The name id used to reflect the image input.
        Name m_name;

        //! The type of image required for this shader input.
        ShaderInputImageType m_type = ShaderInputImageType::Unknown;

        //! How the array elements in the unbounded array input are accessed.
        ShaderInputImageAccess m_access = ShaderInputImageAccess::Read;

        //! Register id of the resource in the SRG.
        //! This is only valid if the platform compiles the SRGs using "spaces".
        //! If not, this same information will be in the PipelineLayoutDescriptor.
        //! Some platforms (like Vulkan) need the register number when creating the
        //! SRG, others need it when creating the PipelineLayout.
        uint32_t m_registerId = UndefinedRegisterSlot;

        //! Logical Register Space that the register id is within.
        //! This is primarily used when an SRG contains one or more unbounded arrays,
        //! as an unbounded array contains all register ids in a register space.
        //! If an SRG doesn't contain any unbounded arrays all resources in it 
        //! will use the same space id.
        uint32_t m_spaceId = UndefinedRegisterSlot;
    };

    class ShaderInputSamplerDescriptor final
    {
    public:
        AZ_TYPE_INFO(ShaderInputSamplerDescriptor, "{F42E989D-002E-42B3-A396-062CB0DB6644}");
        static void Reflect(ReflectContext* context);

        ShaderInputSamplerDescriptor() = default;
        ShaderInputSamplerDescriptor(const Name& name, uint32_t samplerCount, uint32_t registerId, uint32_t spaceId);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        //! The name id used to reflect the sampler input.
        Name m_name;

        //! Number of sampler array elements.
        uint32_t m_count = 0;
            
        //! Register id of the resource in the SRG.
        //! This is only valid if the platform compiles the SRGs using "spaces".
        //! If not, this same information will be in the PipelineLayoutDescriptor.
        //! Some platforms (like Vulkan) need the register number when creating the
        //! SRG, others need it when creating the PipelineLayout.
        uint32_t m_registerId = UndefinedRegisterSlot;

        //! Logical Register Space that the register id is within.
        //! This is primarily used when an SRG contains one or more unbounded arrays,
        //! as an unbounded array contains all register ids in a register space.
        //! If an SRG doesn't contain any unbounded arrays all resources in it 
        //! will use the same space id.
        uint32_t m_spaceId = UndefinedRegisterSlot;
    };

    class ShaderInputConstantDescriptor final
    {
    public:
        AZ_TYPE_INFO(ShaderInputConstantDescriptor, "{C8DC7D2D-CCA0-45AD-9430-52C06B69325C}");
        static void Reflect(ReflectContext* context);

        ShaderInputConstantDescriptor() = default;
        ShaderInputConstantDescriptor(
            const Name& name,
            uint32_t constantByteOffset,
            uint32_t constantByteCount,
            uint32_t registerId,
            uint32_t spaceId);

        //! Returns the 64-bit hash of the binding.
        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        //! The name id used to reflect the constant input.
        Name m_name;

        //! The offset from the start of the constant buffer in bytes.
        uint32_t m_constantByteOffset = 0;

        //! The number of bytes 
        uint32_t m_constantByteCount = 0;
            
        //! Register id of the resource in the SRG.
        //! This is only valid if the platform compiles the SRGs using "spaces".
        //! If not, this same information will be in the PipelineLayoutDescriptor.
        //! Some platforms (like Vulkan) need the register number when creating the
        //! SRG, others need it when creating the PipelineLayout.
        uint32_t m_registerId = UndefinedRegisterSlot;

        //! Logical Register Space that the register id is within.
        //! This is primarily used when an SRG contains one or more unbounded arrays,
        //! as an unbounded array contains all register ids in a register space.
        //! If an SRG doesn't contain any unbounded arrays all resources in it 
        //! will use the same space id.
        uint32_t m_spaceId = UndefinedRegisterSlot;
    };

    class ShaderInputStaticSamplerDescriptor final
    {
    public:
        AZ_TYPE_INFO(ShaderInputStaticSamplerDescriptor, "{A4D3C5AC-1624-4F78-9543-0E37DC93F491}");
        static void Reflect(ReflectContext* context);

        ShaderInputStaticSamplerDescriptor() = default;
        ShaderInputStaticSamplerDescriptor(const Name& name, const SamplerState& samplerState, uint32_t registerId, uint32_t spaceId);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        //! The name id used to reflect the static sampler input.
        Name m_name;
            
        //! The state of this static sampler.
        SamplerState m_samplerState;

        //! Register id of the resource in the SRG.
        //! This is only valid if the platform compiles the SRGs using "spaces".
        //! If not, this same information will be in the PipelineLayoutDescriptor.
        //! Some platforms (like Vulkan) need the register number when creating the
        //! SRG, others need it when creating the PipelineLayout.
        uint32_t m_registerId = UndefinedRegisterSlot;

        //! Logical Register Space that the register id is within.
        //! This is primarily used when an SRG contains one or more unbounded arrays,
        //! as an unbounded array contains all register ids in a register space.
        //! If an SRG doesn't contain any unbounded arrays all resources in it 
        //! will use the same space id.
        uint32_t m_spaceId = UndefinedRegisterSlot;
    };

    //! Returns the string name for the shader input type enum.
    const char* GetShaderInputTypeName(ShaderInputBufferType bufferInputType);
    const char* GetShaderInputTypeName(ShaderInputImageType imageInputType);

    //! Returns the string name for the shader input access enum.
    const char* GetShaderInputAccessName(ShaderInputBufferAccess bufferInputAccess);
    const char* GetShaderInputAccessName(ShaderInputImageAccess imageInputAccess);

    using ShaderInputBufferIndex = Handle<uint32_t, ShaderInputBufferDescriptor>;
    using ShaderInputImageIndex = Handle<uint32_t, ShaderInputImageDescriptor>;
    using ShaderInputBufferUnboundedArrayIndex = Handle<uint32_t, ShaderInputBufferUnboundedArrayDescriptor>;
    using ShaderInputImageUnboundedArrayIndex = Handle<uint32_t, ShaderInputImageUnboundedArrayDescriptor>;
    using ShaderInputSamplerIndex = Handle<uint32_t, ShaderInputSamplerDescriptor>;
    using ShaderInputConstantIndex = Handle<uint32_t, ShaderInputConstantDescriptor>;
    using ShaderInputStaticSamplerIndex = Handle<uint32_t, ShaderInputStaticSamplerDescriptor>;
}
