/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SlotExecutionMap.h"

#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include "SubgraphInterfaceUtility.h"

namespace SlotExecutionMapCpp
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::SlotExecution;

    template<typename t_Container, typename t_UnaryPredicate>
    typename t_Container::const_iterator find_if(const t_Container& con, t_UnaryPredicate pred)
    {
        return AZStd::find_if(begin(con), end(con), pred);
    }

    template<typename t_Container, typename t_UnaryPredicate>
    typename t_Container::iterator find_if(t_Container& con, t_UnaryPredicate pred)
    {
        return AZStd::find_if(begin(con), end(con), pred);
    }

    In* FindInBySlotIdNoError(SlotId slotId, Ins& ins)
    {
        auto iter = find_if(ins, [&slotId](const In& in) { return in.slotId == slotId; });
        return iter != ins.end() ? iter : nullptr;
    }
    
    In* FindInBySlotId(SlotId inSlotId, Ins& ins)
    {
        if (auto in = FindInBySlotIdNoError(inSlotId, ins))
        {
            return in;
        }
        else
        {
            AZ_Error("ScriptCanvas", false, "No Execution In Slot with Id: %s", inSlotId.ToString().data());
            return nullptr;
        }
    }

    template<typename T>
    AZStd::vector<SlotId> ToSlotIds(const T& source)
    {
        AZStd::vector<SlotId> slotIds;
        slotIds.reserve(source.size());

        for (const auto& entry : source)
        {
            slotIds.push_back(entry.slotId);
        }

        return slotIds;
    }
}

namespace ScriptCanvas
{
    namespace SlotExecution
    {          
        using namespace SlotExecutionMapCpp;

        Input::Input(const SlotId& slotId)
            : slotId(slotId)
        {}

        Input::Input(const SlotId& slotId, const VariableId& interfaceSourceID)
            : slotId(slotId)
            , interfaceSourceId(interfaceSourceID)
        {}

        Output::Output(const SlotId& slotId)
            : slotId(slotId)
        {}

        Output::Output(const SlotId& slotId, const VariableId& interfaceSourceID)
            : slotId(slotId)
            , interfaceSourceId(interfaceSourceID)
        {}

        InputSlotIds ToInputSlotIds(const Inputs& source)
        {
             return SlotExecutionMapCpp::ToSlotIds(source);
        }

        OutputSlotIds ToOutputSlotIds(const Outputs& source)
        {
            return SlotExecutionMapCpp::ToSlotIds(source);
        }

        Map::Map(Ins&& ins)
            : m_ins(AZStd::move(ins))
        {}

        Map::Map(Ins&& ins, Outs&& latents)
            : m_ins(AZStd::move(ins))
            , m_latents(AZStd::move(latents))
        {}

        Map::Map(Outs&& latents)
            : m_latents(AZStd::move(latents))
        {}

        const Out* Map::FindImmediateOut([[maybe_unused]] SlotId out, [[maybe_unused]] bool errorOnFailure) const
        {
            const Out* found = nullptr;
            for (const auto& in : m_ins)
            {
                auto iter = find_if(in.outs, [&out](const Out& candidate) { return candidate.slotId == out; });

                if (iter != in.outs.end())
                {
                    AZ_Error("ScriptCanvas", !found, "This Out should only be possible in one In");
                    found = iter;
                }
            }

            AZ_Error("ScriptCanvas", found != nullptr || !errorOnFailure, "No out named: %s", out.ToString().data());
            return found;
        }

        const Out* Map::FindImmediateOut([[maybe_unused]] SlotId slotId, [[maybe_unused]] SlotId outName, [[maybe_unused]] bool errorOnFailure) const
        {
            if (auto in = FindInFromSlotId(slotId))
            {
                auto iter = find_if(in->outs, [&outName](const Out& out) { return out.slotId == outName; });

                if (iter != in->outs.end())
                {
                    return iter;
                }
                else
                {
                    AZ_Error("ScriptCanvas", !errorOnFailure, "No out named: %s with in named: %s", outName.ToString().data(), slotId.ToString().data());
                }
            }

            return nullptr;
        }

        const In* Map::FindInFromInputSlot(const SlotId& slotID) const
        {
            auto iter = find_if
                ( m_ins
                , [&](const auto& in)
                {
                    return find_if
                        ( in.inputs
                        , [&slotID](const Input& input)
                        {
                            return input.slotId == slotID;
                        }) != in.inputs.end();
                });

            return iter != m_ins.end() ? iter : nullptr;
        }

        const Out* Map::FindOutFromOutputSlot(const SlotId& slotID) const
        {
            for (const In& in : m_ins)
            {
                for (const Out& out : in.outs)
                {
                    for (const Output& output : out.outputs)
                    {
                        if (output.slotId == slotID)
                        {
                            return &out;
                        }
                    }
                }
            }

            for (const Out& latent : m_latents)
            {
                for (const Output& output : latent.outputs)
                {
                    if (output.slotId == slotID)
                    {
                        return &latent;
                    }
                }
            }

            return nullptr;
        }

        const In* Map::FindInFromSlotId(SlotId slotId) const
        {
            return FindInBySlotId(slotId, *const_cast<Ins*>(&m_ins));
        }

        const Out* Map::FindLatentOut(SlotId latentName, [[maybe_unused]] bool errorOnFailure) const
        {
            auto iter = find_if(m_latents, [&latentName](const Out& latent) { return latent.slotId == latentName; });

            if (iter != m_latents.end())
            {
                return iter;
            }
            else
            {
                AZ_Error("ScriptCanvas", !errorOnFailure, "No latent named: %s", latentName.ToString().data());
                return nullptr;
            }
        }

        SlotId Map::FindInputSlotIdBySource(VariableId inputSourceId, Grammar::FunctionSourceId inSourceId) const
        {
            // Look up match input from Ins
            auto inIter = find_if(m_ins, [&inSourceId](const In& in)
                { return in.interfaceSourceId == inSourceId; }
            );
            if (inIter != m_ins.end())
            {
                auto inputIter = find_if(inIter->inputs, [&inputSourceId](const Input& input)
                    { return input.interfaceSourceId == inputSourceId; }
                );
                if (inputIter != inIter->inputs.end())
                {
                    return inputIter->slotId;
                }
            }

            // Look up match input from Latents
            auto latentIter = find_if(m_latents, [&inSourceId](const Out& out)
                { return out.interfaceSourceId == inSourceId; }
            );
            if (latentIter != m_latents.end())
            {
                auto inputIter = find_if(latentIter->returnValues.values, [&inputSourceId](const Input& input)
                    { return input.interfaceSourceId == inputSourceId; }
                );
                if (inputIter != latentIter->returnValues.values.end())
                {
                    return inputIter->slotId;
                }
            }

            return SlotId{};
        }

        SlotId Map::FindInSlotIdBySource(Grammar::FunctionSourceId sourceId) const
        {
            auto iter = find_if
                ( m_ins
                , [&](const auto& in)
                {
                    return in.interfaceSourceId == sourceId;
                });

            return iter != m_ins.end() ? iter->slotId : SlotId{};
        }

        SlotId Map::FindLatentSlotIdBySource(Grammar::FunctionSourceId sourceId) const
        {
            auto iter = find_if
                ( m_latents
                , [&sourceId](const Out& out)
                {
                    return out.interfaceSourceId == sourceId;
                });

            return iter != m_latents.end() ? iter->slotId : SlotId{};
        }

        SlotId Map::FindOutputSlotIdBySource(VariableId sourceId) const
        {
            for (const auto& in : m_ins)
            {
                for (const auto& out : in.outs)
                {
                    auto iter = find_if
                        ( out.outputs
                        , [&sourceId](const Output& output)
                        {
                            return output.interfaceSourceId == sourceId;
                        });

                    if (iter != out.outputs.end())
                    {
                        return iter->slotId;
                    }
                }
            }

            for (const auto& out : m_latents)
            {
                auto iter = find_if
                    ( out.outputs
                    , [&sourceId](const Output& output)
                    {
                        return output.interfaceSourceId == sourceId;
                    });

                if (iter != out.outputs.end())
                {
                    return iter->slotId;
                }
            }

            return SlotId{};
        }

        SlotId Map::FindOutSlotIdBySource(Grammar::FunctionSourceId inSourceID, Grammar::FunctionSourceId outSourceId) const
        {
            auto iterIn = find_if
                ( m_ins
                , [&inSourceID](const auto& in)
                {
                    return in.interfaceSourceId == inSourceID;
                });

            if (iterIn != m_ins.end())
            {
                const auto& in = *iterIn;
                auto iter = find_if
                    ( in.outs
                    , [&outSourceId](const Out& out)
                    {
                        return Grammar::OutIdIsEqual(out.interfaceSourceId, outSourceId);
                    });

                if (iter != in.outs.end())
                {
                    return iter->slotId;
                }
            }

            return SlotId{};
        }

        const In* Map::GetIn(size_t index) const
        {
            return index < m_ins.size() ? &m_ins[index] : nullptr;
        }

        const In* Map::GetIn(SlotId inName) const
        {
            return FindInBySlotIdNoError(inName, const_cast<Ins&>(m_ins));
        }
        
        const Inputs* Map::GetInput(SlotId slotId) const
        {
            if (auto in = FindInFromSlotId(slotId))
            {
                return &in->inputs;
            }
            else
            {
                return nullptr;
            }
        }

        const Ins& Map::GetIns() const
        {
            return m_ins;
        }

        const Out* Map::GetLatent(SlotId latentName) const
        {
            return FindLatentOut(latentName);
        }

        const Outputs* Map::GetLatentOutput(SlotId latentName) const
        {
            if (auto latent = FindLatentOut(latentName))
            {
                return &(latent->outputs);
            }
            
            return nullptr;
        }

        const Outs& Map::GetLatents() const
        {
            return m_latents;
        }

        const Out* Map::GetOut(SlotId out) const
        {
            return FindImmediateOut(out);
        }

        const Out* Map::GetOut(SlotId inName, SlotId outName) const
        {
            if (auto out = FindImmediateOut(inName, outName))
            {
                return out;
            }

            return nullptr;
        }

        const Outputs* Map::GetOutput(SlotId outId) const
        {
            if (auto out = FindImmediateOut(outId))
            {
                return &out->outputs;
            }
            
            return nullptr;
        }

        const Outputs* Map::GetOutput(SlotId inName, SlotId outName) const
        {
            if (auto out = FindImmediateOut(inName, outName))
            {
                return &out->outputs;
            }
            
            return nullptr;
        }

        const Outs* Map::GetOuts(SlotId slotId) const
        {
            if (auto in = FindInFromSlotId(slotId))
            {
                return &in->outs;
            }
            
            return nullptr;
        }

        const Inputs* Map::GetReturnValues(SlotId , SlotId outSlotId) const
        {
            if (auto out = FindLatentOut(outSlotId, false))
            {
                return &out->returnValues.values;
            }

            if (auto out = FindImmediateOut(outSlotId, outSlotId))
            {
                return &out->returnValues.values;
            }

            return nullptr;
        }

        const Inputs* Map::GetReturnValuesByOut(SlotId outSlotId) const
        {
            if (auto out = FindLatentOut(outSlotId, false))
            {
                return &out->returnValues.values;
            }

            if (auto out = FindImmediateOut(outSlotId))
            {
                return &out->returnValues.values;
            }

            return nullptr;
        }
        
        AZ::Outcome<bool> Map::IsBranch(SlotId inName) const
        {
            if (auto in = GetIn(inName))
            {
                return AZ::Success(in->IsBranch());
            }
            else
            {
                return AZ::Failure();
            }          
        }

        bool Map::IsEmpty() const
        {
            return m_ins.empty() && m_latents.empty();
        }

        bool Map::IsLatent() const
        {
            return !m_latents.empty();
        }

        AZStd::string Map::ToExecutionString() const
        {
            AZStd::string result;

            for (const auto& in : m_ins)
            {
                result += "\n";
                result += "In: ";
                result += in.parsedName;
                result += "\n";

                for (const auto& out : in.outs)
                {
                    result += "\tOut: ";
                    result += out.name;
                    result += "\n";
                }
            }

            for (const auto& out : m_latents)
            {
                result += "Latent: ";
                result += out.name;
                result += "\n";
            }

            return result;
        }

        void Map::Reflect(AZ::ReflectContext* refectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(refectContext))
            {
                serializeContext->Class<Output>()
                    ->Version(0)
                    ->Field("_slotId", &Output::slotId)
                    ->Field("_interfaceSourceId", &Output::interfaceSourceId)
                    ;

                serializeContext->Class<Input>()
                    ->Version(0)
                    ->Field("_slotId", &Input::slotId)
                    ->Field("_interfaceSourceId", &Input::interfaceSourceId)
                    ;

                serializeContext->Class<Return>()
                    ->Version(1)
                    ->Field("_values", &Return::values)
                    ;

                serializeContext->Class<Out>()
                    ->Version(1)
                    ->Field("_slotId", &Out::slotId)
                    ->Field("_name", &Out::name)
                    ->Field("_outputs", &Out::outputs)
                    ->Field("_returnValues", &Out::returnValues)
                    ->Field("_interfaceSourceId", &Out::interfaceSourceId)
                    ;
            
                serializeContext->Class<In>()
                    ->Version(1)
                    ->Field("_slotId", &In::slotId)
                    ->Field("_inputs", &In::inputs)
                    ->Field("_outs", &In::outs)
                    ->Field("_parsedName", &In::parsedName)
                    ->Field("_interfaceSourceId", &In::interfaceSourceId)
                    ;

                serializeContext->Class<Map>()
                    ->Version(1)
                    ->Field("ins", &Map::m_ins)
                    ->Field("latents", &Map::m_latents)
                    ;
            }
        }
    }
} 
