/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/RenderStates.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(CullMode);
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(FillMode);
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(DepthWriteMask);
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(StencilOp);
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(BlendFactor);
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(BlendOp);

    void ReflectRenderStateEnums(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            CullModeReflect(*serializeContext);
            FillModeReflect(*serializeContext);
            DepthWriteMaskReflect(*serializeContext);
            StencilOpReflect(*serializeContext);
            BlendFactorReflect(*serializeContext);
            BlendOpReflect(*serializeContext);
        }

        if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            CullModeReflect(*behaviorContext);
            FillModeReflect(*behaviorContext);
            DepthWriteMaskReflect(*behaviorContext);
            StencilOpReflect(*behaviorContext);
            BlendFactorReflect(*behaviorContext);
            BlendOpReflect(*behaviorContext);
        }
    }

    template<typename T>
    void MergeValue(T& resultValue, const T& valueToMerge, const T& invalidValue)
    {
        resultValue = (valueToMerge != invalidValue) ? valueToMerge : resultValue;
    }

    #define RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, prop) MergeValue(result.prop, stateToMerge.prop, invalidState.prop)

    void MergeStateInto(const RasterState& stateToMerge, RasterState& result)
    {
        const RasterState& invalidState = GetInvalidRasterState();
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_fillMode);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_cullMode);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_depthBias);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_depthBiasClamp);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_depthBiasSlopeScale);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_multisampleEnable);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_depthClipEnable);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_conservativeRasterEnable);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_forcedSampleCount);
    }

    void MergeStateInto(const StencilOpState& stateToMerge, StencilOpState& result)
    {
        const StencilOpState& invalidState = GetInvalidStencilOpState();
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_failOp);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_passOp);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_depthFailOp);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_func);
    }

    void MergeStateInto(const StencilState& stateToMerge, StencilState& result)
    {
        const StencilState& invalidState = GetInvalidStencilState();
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_enable);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_readMask);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_writeMask);

        MergeStateInto(stateToMerge.m_backFace, result.m_backFace);
        MergeStateInto(stateToMerge.m_frontFace, result.m_frontFace);
    }

    void MergeStateInto(const DepthState& stateToMerge, DepthState& result)
    {
        const DepthState& invalidState = GetInvalidDepthState();
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_enable);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_writeMask);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_func);
    }

    void MergeStateInto(const DepthStencilState& stateToMerge, DepthStencilState& result)
    {
        MergeStateInto(stateToMerge.m_depth, result.m_depth);
        MergeStateInto(stateToMerge.m_stencil, result.m_stencil);
    }

    void MergeStateInto(const TargetBlendState& stateToMerge, TargetBlendState& result)
    {
        const TargetBlendState& invalidState = GetInvalidTargetBlendState();
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_enable);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_writeMask);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_blendSource);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_blendDest);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_blendOp);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_blendAlphaSource);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_blendAlphaDest);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_blendAlphaOp);
    }

    void MergeStateInto(const BlendState& stateToMerge, BlendState& result)
    {
        const BlendState& invalidState = GetInvalidBlendState();
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_alphaToCoverageEnable);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_independentBlendEnable);
        for (uint32_t i = 0; i < Limits::Pipeline::AttachmentColorCountMax; ++i)
        {
            MergeStateInto(stateToMerge.m_targets[i], result.m_targets[i]);
        }
    }

    void MergeStateInto(const MultisampleState& stateToMerge, MultisampleState& result)
    {
        const MultisampleState& invalidState = GetInvalidMultisampleState();
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_customPositionsCount);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_samples);
        RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_quality);
        for (uint32_t positionIndex = 0; positionIndex < Limits::Pipeline::MultiSampleCustomLocationsCountMax; ++positionIndex)
        {
            RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_customPositions[positionIndex].m_x);
            RENDER_STATES_MERGE_STATE_PROPERTY(result, stateToMerge, invalidState, m_customPositions[positionIndex].m_y);
        }
    }

    void MergeStateInto(const RenderStates& statesToMerge, RenderStates& result)
    {
        MergeStateInto(statesToMerge.m_multisampleState, result.m_multisampleState);
        MergeStateInto(statesToMerge.m_rasterState, result.m_rasterState);
        MergeStateInto(statesToMerge.m_blendState, result.m_blendState);
        MergeStateInto(statesToMerge.m_depthStencilState, result.m_depthStencilState);
    }

    #undef RENDER_STATES_MERGE_STATE_PROPERTY

    const RasterState& GetInvalidRasterState()
    {
        static const RasterState invalidState
        {
            FillMode::Invalid,         // m_fillMode
            CullMode::Invalid,         // m_cullMode
            RenderStates_InvalidInt,   // m_depthBias
            RenderStates_InvalidFloat, // m_depthBiasClamp
            RenderStates_InvalidFloat, // m_depthBiasSlopeScale
            RenderStates_InvalidBool,  // m_multisampleEnable
            RenderStates_InvalidBool,  // m_depthClipEnable
            RenderStates_InvalidBool,  // m_conservativeRasterEnable
            RenderStates_InvalidUInt   // m_forcedSampleCount
        };

        return invalidState;
    }

    const DepthState& GetInvalidDepthState()
    {
        static const DepthState invalidState
        {
            RenderStates_InvalidBool, // m_enable
            DepthWriteMask::Invalid,  // m_writeMask
            ComparisonFunc::Invalid   // m_func
        };

        return invalidState;
    }

    const StencilOpState& GetInvalidStencilOpState()
    {
        static const StencilOpState invalidState
        {
            StencilOp::Invalid,      // m_failOp
            StencilOp::Invalid,      // m_depthFailOp
            StencilOp::Invalid,      // m_passOp
            ComparisonFunc::Invalid  // m_func
        };

        return invalidState;
    }

    const StencilState& GetInvalidStencilState()
    {
        static const StencilState invalidState
        {
            RenderStates_InvalidBool,   // m_enable
            RenderStates_InvalidUInt,   // m_readMask
            RenderStates_InvalidUInt,   // m_writeMask
            GetInvalidStencilOpState(), // m_frontFace
            GetInvalidStencilOpState()  // m_backFace
        };

        return invalidState;
    }

    const DepthStencilState& GetInvalidDepthStencilState()
    {
        static const DepthStencilState invalidState
        {
            GetInvalidDepthState(),     // m_depth
            GetInvalidStencilState()    // m_stencil
        };

        return invalidState;
    }

    const TargetBlendState& GetInvalidTargetBlendState()
    {
        static constexpr TargetBlendState invalidState
        {
            RenderStates_InvalidBool, // m_enable
            RenderStates_InvalidUInt, // m_writeMask
            BlendFactor::Invalid,     // m_blendSource
            BlendFactor::Invalid,     // m_blendDest
            BlendOp::Invalid,         // m_blendOp
            BlendFactor::Invalid,     // m_blendAlphaSource
            BlendFactor::Invalid,     // m_blendAlphaDest
            BlendOp::Invalid          // m_blendAlphaOp
        };

        return invalidState;
    }

    const BlendState& GetInvalidBlendState()
    {
        auto FillTargetBlendState = []()->AZStd::array<TargetBlendState, Limits::Pipeline::AttachmentColorCountMax>
        {
            AZStd::array<TargetBlendState, Limits::Pipeline::AttachmentColorCountMax> targets;
            targets.fill(GetInvalidTargetBlendState());
            return targets;
        };

        static const BlendState invalidState
        {
            RenderStates_InvalidBool, // m_alphaToCoverageEnable
            RenderStates_InvalidBool, // m_independentBlendEnable
            FillTargetBlendState()    // m_targets
        };

        return invalidState;
    }

    const MultisampleState& GetInvalidMultisampleState()
    {
        auto FillInvalidState = []()->MultisampleState
        {
            MultisampleState invalidState;
            SamplePosition invalidSamplePosition;
            // Note SamplePosition has assertion to block invalid values in its non-default constructor.
            invalidSamplePosition.m_x = Limits::Pipeline::MultiSampleCustomLocationGridSize;
            invalidSamplePosition.m_y = Limits::Pipeline::MultiSampleCustomLocationGridSize;
            invalidState.m_customPositions.fill(invalidSamplePosition);
            invalidState.m_customPositionsCount = RenderStates_InvalidUInt;
            invalidState.m_samples = RenderStates_InvalidUInt16;
            invalidState.m_quality = RenderStates_InvalidUInt16;

            return invalidState;
        };

        // Restricted by its constructors, MultisampleState doesnt support argument list construction.
        static const MultisampleState invalidState = FillInvalidState();

        return invalidState;
    }

    const RenderStates& GetInvalidRenderStates()
    {
        static const RenderStates invalidStates
        {
            GetInvalidMultisampleState(), // m_multisampleState
            GetInvalidRasterState(),      // m_rasterState
            GetInvalidBlendState(),       // m_blendState
            GetInvalidDepthStencilState() // m_depthStencilState
        };

        return invalidStates;
    }

    HashValue64 RenderStates::GetHash(HashValue64 seed) const
    {
        return TypeHash64(*this, seed);
    }

    DepthStencilState DepthStencilState::CreateDepth()
    {
        DepthStencilState descriptor;
        descriptor.m_depth.m_func = ComparisonFunc::LessEqual;
        return descriptor;
    }

    DepthStencilState DepthStencilState::CreateReverseDepth()
    {
        DepthStencilState descriptor;
        descriptor.m_depth.m_func = ComparisonFunc::GreaterEqual;
        return descriptor;
    }

    DepthStencilState DepthStencilState::CreateDisabled()
    {
        DepthStencilState descriptor;
        descriptor.m_depth.m_enable = false;
        return descriptor;
    }

    void RasterState::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<RasterState>()
                ->Version(1)
                ->Field("depthBias", &RasterState::m_depthBias)
                ->Field("depthBiasClamp", &RasterState::m_depthBiasClamp)
                ->Field("depthBiasSlopeScale", &RasterState::m_depthBiasSlopeScale)
                ->Field("fillMode", &RasterState::m_fillMode)
                ->Field("cullMode", &RasterState::m_cullMode)
                ->Field("multisampleEnable", &RasterState::m_multisampleEnable)
                ->Field("depthClipEnable", &RasterState::m_depthClipEnable)
                ->Field("conservativeRasterEnable", &RasterState::m_conservativeRasterEnable)
                ->Field("forcedSampleCount", &RasterState::m_forcedSampleCount)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<RasterState>("RasterState", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RasterState::m_depthBias, "Depth Bias", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RasterState::m_depthBiasClamp, "Depth Bias Clamp", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RasterState::m_depthBiasSlopeScale, "Depth Bias Slope Scale", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RasterState::m_fillMode, "Fill Mode", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<FillMode>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RasterState::m_cullMode, "Cull Mode", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<CullMode>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RasterState::m_multisampleEnable, "Multisample Enable", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RasterState::m_depthClipEnable, "Depth Clip Enable", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RasterState::m_conservativeRasterEnable, "Conservative Raster Enable", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RasterState::m_forcedSampleCount, "Forced Sample Count", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RasterState>("RasterState")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Constructor()
                ->Constructor<const RasterState&>()
                ->Property("depthBias", BehaviorValueProperty(&RasterState::m_depthBias))
                ->Property("depthBiasClamp", BehaviorValueProperty(&RasterState::m_depthBiasClamp))
                ->Property("depthBiasSlopeScale", BehaviorValueProperty(&RasterState::m_depthBiasSlopeScale))
                ->Property("fillMode", BehaviorValueProperty(&RasterState::m_fillMode))
                ->Property("cullMode", BehaviorValueProperty(&RasterState::m_cullMode))
                ->Property("multisampleEnable", BehaviorValueProperty(&RasterState::m_multisampleEnable))
                ->Property("depthClipEnable", BehaviorValueProperty(&RasterState::m_depthClipEnable))
                ->Property("conservativeRasterEnable", BehaviorValueProperty(&RasterState::m_conservativeRasterEnable))
                ->Property("forcedSampleCount", BehaviorValueProperty(&RasterState::m_forcedSampleCount))
                ;
        }
    }

    void StencilOpState::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<StencilOpState>()
                ->Version(1)
                ->Field("failOp", &StencilOpState::m_failOp)
                ->Field("depthFailOp", &StencilOpState::m_depthFailOp)
                ->Field("passOp", &StencilOpState::m_passOp)
                ->Field("func", &StencilOpState::m_func)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<StencilOpState>("StencilOpState", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StencilOpState::m_failOp, "Fail Op", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<StencilOp>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StencilOpState::m_depthFailOp, "Depth Fail Op", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<StencilOp>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StencilOpState::m_passOp, "Pass Op", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<StencilOp>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StencilOpState::m_func, "Func", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<ComparisonFunc>())
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<StencilOpState>("StencilOpState")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Constructor()
                ->Constructor<const StencilOpState&>()
                ->Property("failOp", BehaviorValueProperty(&StencilOpState::m_failOp))
                ->Property("depthFailOp", BehaviorValueProperty(&StencilOpState::m_depthFailOp))
                ->Property("passOp", BehaviorValueProperty(&StencilOpState::m_passOp))
                ->Property("func", BehaviorValueProperty(&StencilOpState::m_func))
                ;
        }
    }

    void DepthState::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<DepthState>()
                ->Version(1)
                ->Field("enable", &DepthState::m_enable)
                ->Field("writeMask", &DepthState::m_writeMask)
                ->Field("compareFunc", &DepthState::m_func)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DepthState>("DepthState", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DepthState::m_enable, "Enable", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DepthState::m_writeMask, "Write Mask", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<DepthWriteMask>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DepthState::m_func, "Func", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<ComparisonFunc>())
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DepthState>("DepthState")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Constructor()
                ->Constructor<const DepthState&>()
                ->Property("enable", BehaviorValueProperty(&DepthState::m_enable))
                ->Property("writeMask", BehaviorValueProperty(&DepthState::m_writeMask))
                ->Property("compareFunc", BehaviorValueProperty(&DepthState::m_func))
                ;
        }
    }

    void StencilState::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<StencilState>()
                ->Version(1)
                ->Field("enable", &StencilState::m_enable)
                ->Field("readMask", &StencilState::m_readMask)
                ->Field("writeMask", &StencilState::m_writeMask)
                ->Field("frontFace", &StencilState::m_frontFace)
                ->Field("backFace", &StencilState::m_backFace)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<StencilState>("StencilState", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StencilState::m_enable, "Enable", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StencilState::m_readMask, "Read Mask", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StencilState::m_writeMask, "Write Mask", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StencilState::m_frontFace, "Front Face", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StencilState::m_backFace, "Back Face", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<StencilState>("StencilState")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Constructor()
                ->Constructor<const StencilState&>()
                ->Property("enable", BehaviorValueProperty(&StencilState::m_enable))
                ->Property("readMask", BehaviorValueProperty(&StencilState::m_readMask))
                ->Property("writeMask", BehaviorValueProperty(&StencilState::m_writeMask))
                ->Property("frontFace", BehaviorValueProperty(&StencilState::m_frontFace))
                ->Property("backFace", BehaviorValueProperty(&StencilState::m_backFace))
                ;
        }
    }

    void DepthStencilState::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<DepthStencilState>()
                ->Version(1)
                ->Field("depth", &DepthStencilState::m_depth)
                ->Field("stencil", &DepthStencilState::m_stencil)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DepthStencilState>("DepthStencilState", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DepthStencilState::m_depth, "Depth", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DepthStencilState::m_stencil, "Stencil", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DepthStencilState>("DepthStencilState")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Constructor()
                ->Constructor<const DepthStencilState&>()
                ->Property("depth", BehaviorValueProperty(&DepthStencilState::m_depth))
                ->Property("stencil", BehaviorValueProperty(&DepthStencilState::m_stencil))
                ;
        }
    }

    void TargetBlendState::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<TargetBlendState>()
                ->Version(1)
                ->Field("enable", &TargetBlendState::m_enable)
                ->Field("blendSource", &TargetBlendState::m_blendSource)
                ->Field("blendDest", &TargetBlendState::m_blendDest)
                ->Field("blendOp", &TargetBlendState::m_blendOp)
                ->Field("blendAlphaSource", &TargetBlendState::m_blendAlphaSource)
                ->Field("blendAlphaDest", &TargetBlendState::m_blendAlphaDest)
                ->Field("blendAlphaOp", &TargetBlendState::m_blendAlphaOp)
                ->Field("writeMask", &TargetBlendState::m_writeMask)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TargetBlendState>("TargetBlendState", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TargetBlendState::m_enable, "Enable", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TargetBlendState::m_blendSource, "Blend Source", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<BlendFactor>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TargetBlendState::m_blendDest, "Blend Dest", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<BlendFactor>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TargetBlendState::m_blendOp, "Blend Op", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<BlendOp>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TargetBlendState::m_blendAlphaSource, "Blend Alpha Source", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<BlendFactor>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TargetBlendState::m_blendAlphaDest, "Blend Alpha Dest", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<BlendFactor>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TargetBlendState::m_blendAlphaOp, "Blend Alpha Op", "")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<BlendOp>())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TargetBlendState::m_writeMask, "Write Mask", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<TargetBlendState>("TargetBlendState")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Constructor()
                ->Constructor<const TargetBlendState&>()
                ->Property("enable", BehaviorValueProperty(&TargetBlendState::m_enable))
                ->Property("blendSource", BehaviorValueProperty(&TargetBlendState::m_blendSource))
                ->Property("blendDest", BehaviorValueProperty(&TargetBlendState::m_blendDest))
                ->Property("blendOp", BehaviorValueProperty(&TargetBlendState::m_blendOp))
                ->Property("blendAlphaSource", BehaviorValueProperty(&TargetBlendState::m_blendAlphaSource))
                ->Property("blendAlphaDest", BehaviorValueProperty(&TargetBlendState::m_blendAlphaDest))
                ->Property("blendAlphaOp", BehaviorValueProperty(&TargetBlendState::m_blendAlphaOp))
                ->Property("writeMask", BehaviorValueProperty(&TargetBlendState::m_writeMask))
                ;
        }
    }

    void BlendState::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<BlendState>()
                ->Version(1)
                ->Field("alphaToCoverageEnable", &BlendState::m_alphaToCoverageEnable)
                ->Field("independentBlendEnable", &BlendState::m_independentBlendEnable)
                ->Field("targets", &BlendState::m_targets)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BlendState>("BlendState", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BlendState::m_alphaToCoverageEnable, "Alpha To Coverage Enable", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BlendState::m_independentBlendEnable, "Independent Blend Enable", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BlendState::m_targets, "Targets", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<BlendState>("BlendState")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Constructor()
                ->Constructor<const BlendState&>()
                ->Property("alphaToCoverageEnable", BehaviorValueProperty(&BlendState::m_alphaToCoverageEnable))
                ->Property("independentBlendEnable", BehaviorValueProperty(&BlendState::m_independentBlendEnable))
                ->Property("targets", BehaviorValueProperty(&BlendState::m_targets))
                ;
        }
    }

    void SamplePosition::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<SamplePosition>()
                ->Version(1)
                ->Field("x", &SamplePosition::m_x)
                ->Field("y", &SamplePosition::m_y)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SamplePosition>("SamplePosition", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SamplePosition::m_x, "X", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SamplePosition::m_y, "Y", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SamplePosition>("SamplePosition")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Constructor()
                ->Constructor<const SamplePosition&>()
                ->Property("x", BehaviorValueProperty(&SamplePosition::m_x))
                ->Property("y", BehaviorValueProperty(&SamplePosition::m_y))
                ;
        }
    }

    void MultisampleState::Reflect(ReflectContext* context)
    {
        SamplePosition::Reflect(context);
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<MultisampleState>()
                ->Version(1)
                ->Field("samples", &MultisampleState::m_samples)
                ->Field("quality", &MultisampleState::m_quality)
                ->Field("customPositions", &MultisampleState::m_customPositions)
                ->Field("customPositionsCount", &MultisampleState::m_customPositionsCount)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MultisampleState>("MultisampleState", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultisampleState::m_samples, "Samples", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultisampleState::m_quality, "Quality", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultisampleState::m_customPositions, "Custom Positions", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultisampleState::m_customPositionsCount, "Custom Positions Count", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MultisampleState>("MultisampleState")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Constructor()
                ->Constructor<const MultisampleState&>()
                ->Property("samples", BehaviorValueProperty(&MultisampleState::m_samples))
                ->Property("quality", BehaviorValueProperty(&MultisampleState::m_quality))
                ->Property("customPositions", BehaviorValueProperty(&MultisampleState::m_customPositions))
                ->Property("customPositionsCount", BehaviorValueProperty(&MultisampleState::m_customPositionsCount))
                ;
        }
    }

    void RenderStates::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<RenderStates>()
                ->Version(1)
                ->Field("rasterState", &RenderStates::m_rasterState)
                ->Field("multisampleState", &RenderStates::m_multisampleState)
                ->Field("depthStencilState", &RenderStates::m_depthStencilState)
                ->Field("blendState", &RenderStates::m_blendState)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<RenderStates>("RenderStates", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RenderStates::m_rasterState, "Raster State", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RenderStates::m_multisampleState, "Multisample State", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RenderStates::m_depthStencilState, "DepthStencil State", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RenderStates::m_blendState, "Blend State", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RenderStates>("RenderStates")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Constructor()
                ->Constructor<const RenderStates&>()
                ->Property("rasterState", BehaviorValueProperty(&RenderStates::m_rasterState))
                ->Property("multisampleState", BehaviorValueProperty(&RenderStates::m_multisampleState))
                ->Property("depthStencilState", BehaviorValueProperty(&RenderStates::m_depthStencilState))
                ->Property("blendState", BehaviorValueProperty(&RenderStates::m_blendState))
                ;
        }
    }

    bool RenderStates::operator == (const RenderStates& rhs) const
    {
        return (memcmp(this, &rhs, sizeof(RenderStates)) == 0);
    }

    bool RasterState::operator == (const RasterState& rhs) const
    {
        return (memcmp(this, &rhs, sizeof(RasterState)) == 0);
    }

    bool StencilOpState::operator == (const StencilOpState& rhs) const
    {
        return (memcmp(this, &rhs, sizeof(StencilOpState)) == 0);
    }

    bool DepthState::operator == (const DepthState& rhs) const
    {
        return (memcmp(this, &rhs, sizeof(DepthState)) == 0);
    }

    bool StencilState::operator == (const StencilState& rhs) const
    {
        return (memcmp(this, &rhs, sizeof(StencilState)) == 0);
    }

    bool DepthStencilState::operator == (const DepthStencilState& rhs) const
    {
        return (memcmp(this, &rhs, sizeof(DepthStencilState)) == 0);
    }

    bool TargetBlendState::operator == (const TargetBlendState& rhs) const
    {
        return (memcmp(this, &rhs, sizeof(TargetBlendState)) == 0);
    }

    bool BlendState::operator == (const BlendState& rhs) const
    {
        return (memcmp(this, &rhs, sizeof(BlendState)) == 0);
    }
}
