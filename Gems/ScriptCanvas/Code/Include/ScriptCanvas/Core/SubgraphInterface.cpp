/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include "SubgraphInterfaceUtility.h"
#include "SubgraphInterface.h"

namespace SubgraphInterfaceCpp
{
    const char* s_outRequirementMessage 
        = "Any immediate execution in without at least 2 declared executions outs triggered by it does not need to be in the map."
        "  Just expose the function to AZ::BehaviorContext (which can include a return value).";

    enum Version
    {
        AddNamespacePath = 0,
        AddActivityParsing,
        AddChildStarts,
        AddExecutionCharacteristics,
        Functions_2_0,
        AddConstructionParameterRequirement,
        AddConstructionParameterRequirementForDependencies,
        // add your entry above
        Current
    };

    [[maybe_unused]] const size_t k_maxTabs = 20;

    AZ_INLINE const char* GetTabs(size_t tabs)
    {
        AZ_Assert(tabs <= k_maxTabs, "invalid argument to GetTabs");

        static const char* const k_tabs[] =
        {
            "", // 0
            "\t",
            "\t\t",
            "\t\t\t",
            "\t\t\t\t",
            "\t\t\t\t\t", // 5
            "\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t", // 10
            "\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t", // 15
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t", // 20
        };

        return k_tabs[tabs];
    }
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        bool In::operator==(const In& rhs) const
        {
            // written out explicitly to aid breakpoint placement

            if (isPure != rhs.isPure)
            {
                return false;
            }

            if (!AZ::StringFunc::Equal(displayName.c_str(), rhs.displayName.c_str()))
            {
                return false;
            }

            if (!AZ::StringFunc::Equal(parsedName.c_str(), rhs.parsedName.c_str()))
            {
                return false;
            }

            if (!(inputs == rhs.inputs))
            {
                return false;
            }

            if (!(outs == rhs.outs))
            {
                return false;
            }

            if (!(sourceID == rhs.sourceID))
            {
                return false;
            }

            return true;
        }

        bool Input::operator==(const Input& rhs) const
        {
            // written out explicitly to aid breakpoint placement

            if (!AZ::StringFunc::Equal(displayName.c_str(), rhs.displayName.c_str()))
            {
                return false;
            }

            if (!AZ::StringFunc::Equal(parsedName.c_str(), rhs.parsedName.c_str()))
            {
                return false;
            }

            if (!(datum.GetType() == rhs.datum.GetType()))
            {
                return false;
            }

            if (!(sourceID == rhs.sourceID))
            {
                return false;
            }

            return true;
        }

        bool Out::operator==(const Out& rhs) const
        {
            // written out explicitly to aid breakpoint placement

            if (!AZ::StringFunc::Equal(displayName.c_str(), rhs.displayName.c_str()))
            {
                return false;
            }

            if (!AZ::StringFunc::Equal(parsedName.c_str(), rhs.parsedName.c_str()))
            {
                return false;
            }

            if (!(outputs == rhs.outputs))
            {
                return false;
            }

            if (!(returnValues == rhs.returnValues))
            {
                return false;
            }

            if (!OutIdIsEqual(sourceID, rhs.sourceID))
            {
                return false;
            }

            return true;
        }

        bool Output::operator==(const Output& rhs) const
        {
            // written out explicitly to aid breakpoint placement

            if (!AZ::StringFunc::Equal(displayName.c_str(), rhs.displayName.c_str()))
            {
                return false;
            }

            if (!AZ::StringFunc::Equal(parsedName.c_str(), rhs.parsedName.c_str()))
            {
                return false;
            }

            if (!(type == rhs.type))
            {
                return false;
            }

            if (!(sourceID == rhs.sourceID))
            {
                return false;
            }

            return true;
        }

        void SubgraphInterface::AddIn(In&& in)
        {
            m_ins.push_back(in);
        }

        void SubgraphInterface::AddLatent(Out&& out)
        {
            m_latents.push_back(out);
        }

        bool SubgraphInterface::AddOutKey(const AZStd::string& name)
        {
            const AZ::Crc32 key(name);

            if (AZStd::find(m_outKeys.begin(), m_outKeys.end(), key) == m_outKeys.end())
            {
                m_outKeys.push_back(key);
                return true;
            }
            else
            {
                return false;
            }
        }

        const In* SubgraphInterface::FindIn(const FunctionSourceId& sourceID) const
        {
            auto iter = AZStd::find_if(m_ins.begin(), m_ins.end(), [&sourceID](const In& in) { return in.sourceID == sourceID; });
            return iter != m_ins.end() ? iter : nullptr;
        }

        const Out* SubgraphInterface::FindLatent(const FunctionSourceId& sourceID) const
        {
            auto iter = AZStd::find_if(m_latents.begin(), m_latents.end(), [&sourceID](const Out& latent) { return latent.sourceID == sourceID; });
            return iter != m_latents.end() ? iter : nullptr;
        }

        ExecutionCharacteristics SubgraphInterface::GetExecutionCharacteristics() const
        {
            return m_executionCharacteristics;
        }

        const In& SubgraphInterface::GetIn(size_t index) const
        {
            return m_ins[index];
        }

        const In* SubgraphInterface::GetIn(const AZStd::string& inName) const
        {
            return FindInByNameNoError(inName, const_cast<Ins&>(m_ins));
        }

        const Ins& SubgraphInterface::GetIns() const
        {
            return m_ins;
        }

        size_t SubgraphInterface::GetInCount() const
        {
            return m_ins.size();
        }

        size_t SubgraphInterface::GetInCountNotPure() const
        {
            return AZStd::count_if(m_ins.begin(), m_ins.end(), [](const auto& in) { return !in.isPure; });
        }

        size_t SubgraphInterface::GetInCountPure() const
        {
            return AZStd::count_if(m_ins.begin(), m_ins.end(), [](const auto& in) { return in.isPure; });
        }

        const Inputs* SubgraphInterface::GetInput(const AZStd::string& inName) const
        {
            if (auto in = FindIn(inName))
            {
                return &in->inputs;
            }
            else
            {
                return nullptr;
            }
        }

        const Out& SubgraphInterface::GetLatentOut(size_t index) const
        {
            return m_latents[index];
        }

        size_t SubgraphInterface::GetLatentOutCount() const
        {
            return m_latents.size();
        }

        const Outputs* SubgraphInterface::GetLatentOutput(const AZStd::string& latentName) const
        {
            if (auto latent = FindLatentOut(latentName))
            {
                return &(latent->outputs);
            }

            return nullptr;
        }

        const Outs& SubgraphInterface::GetLatentOuts() const
        {
            return m_latents;
        }

        LexicalScope SubgraphInterface::GetLexicalScope() const
        {
            return GetLexicalScope(IsMarkedPure());
        }

        LexicalScope SubgraphInterface::GetLexicalScope(bool isSourcePure) const
        {
            if (isSourcePure)
            {
                return LexicalScope{ LexicalScopeType::Namespace , m_namespacePath };
            }
            else
            {
                return LexicalScope{ LexicalScopeType::Variable, {} };
            }
        }

        LexicalScope SubgraphInterface::GetLexicalScope(const In& in) const
        {
            return GetLexicalScope(in.isPure);
        }

        AZStd::string SubgraphInterface::GetName() const
        {
            AZ_Error("ScriptCanvas", !m_namespacePath.empty(), "Interface must have at least one name");
            return !m_namespacePath.empty() ? m_namespacePath.back() : "error, empty interface name";
        }

        const NamespacePath& SubgraphInterface::GetNamespacePath() const
        {
            return m_namespacePath;
        }

        const Out* SubgraphInterface::GetOut(const AZStd::string& inName, const AZStd::string& outName) const
        {
            if (auto out = FindImmediateOut(inName, outName))
            {
                return out;
            }

            return nullptr;
        }

        const AZStd::vector<AZ::Crc32>& SubgraphInterface::GetOutKeys() const
        {
            return m_outKeys;
        }

        const Outputs* SubgraphInterface::GetOutput(const AZStd::string& inName, const AZStd::string& outName) const
        {
            if (auto out = FindImmediateOut(inName, outName))
            {
                return &out->outputs;
            }

            return nullptr;
        }

        const Outs* SubgraphInterface::GetOuts(const AZStd::string& inName) const
        {
            if (auto in = FindIn(inName))
            {
                return &in->outs;
            }

            return nullptr;
        }

        SubgraphInterface::SubgraphInterface(Ins&& ins)
            : m_ins(ins)
        {
            Parse();
        }

        SubgraphInterface::SubgraphInterface(Ins&& ins, Outs&& latents)
            : m_ins(ins)
            , m_latents(latents)
        {
            Parse();
        }

        SubgraphInterface::SubgraphInterface(Outs&& latents)
            : m_latents(latents)
        {
            Parse();
        }

        const Out* SubgraphInterface::FindImmediateOut(const AZStd::string& inName, const AZStd::string& outName) const
        {
            if (auto in = FindIn(inName))
            {
                auto iter = AZStd::find_if(in->outs.begin(), in->outs.end(), [&outName](const Out& out) { return out.displayName == outName; });

                if (iter != in->outs.end())
                {
                    return iter;
                }
                else
                {
                    AZ_Error("ScriptCanvas", false, "No out named: %s with in named: %s", outName.data(), inName.data());
                }
            }

            return nullptr;
        }

        const In* SubgraphInterface::FindIn(const AZStd::string& inName) const
        {
            return FindInByName(inName, *const_cast<Ins*>(&m_ins));
        }

        const Out* SubgraphInterface::FindLatentOut(const AZStd::string& latentName) const
        {
            auto iter = AZStd::find_if(m_latents.begin(), m_latents.end(), [&latentName](const Out& latent) { return latent.displayName == latentName; });

            if (iter != m_latents.end())
            {
                return iter;
            }
            else
            {
                AZ_Error("ScriptCanvas", false, "No latent named: %s", latentName.data());
                return nullptr;
            }
        }

        bool SubgraphInterface::IsActiveDefaultObject() const
        {
            return m_isActiveDefaultObject;
        }

        bool SubgraphInterface::IsUserNodeable() const
        {
            if (IsMarkedPure())
            {
                return false;
            }

            if (IsUserVariable())
            {
                return false;
            }

            if (GetInCountNotPure() == 0 && GetLatentOutCount() == 0)
            {
                return false;
            }

            return true;
        }

        bool SubgraphInterface::IsUserVariable() const
        {
            return m_isUserVariable;
        }

        AZ::Outcome<bool> SubgraphInterface::IsBranch(const AZStd::string& inName) const
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

        bool SubgraphInterface::IsLatent() const
        {
            return !m_latents.empty();
        }

        bool SubgraphInterface::IsMarkedPure() const
        {
            return m_executionCharacteristics == ExecutionCharacteristics::Pure;
        }

        bool SubgraphInterface::IsParsedPure() const
        {
            if (IsLatent())
            {
                return false;
            }

            if (HasBranches())
            {
                return false;
            }

            if (!m_areAllChildrenPure)
            {
                return false;
            }

            if (m_requiresConstructionParameters && (!m_hasOnGraphStart && GetInCount() != 0))
            {
                return false;
            }

            return true;
        }

        bool SubgraphInterface::HasAnyFunctionality() const
        {
            // \todo restore default object addition when ndoes can define an variable, as well
            return /*IsActiveDefaultObject() || */ HasPublicFunctionality();
        }

        bool SubgraphInterface::HasBranches() const
        {
            for (const auto& in : m_ins)
            {
                if (in.outs.size() > 1)
                {
                    return true;
                }
            }

            return false;
        }

        bool SubgraphInterface::HasIn(const FunctionSourceId& sourceID) const
        {
            return ((!IsUserVariable()) && (IsFunctionSourceIdNodeable(sourceID) || IsFunctionSourceIdObject(sourceID)))
                || FindIn(sourceID) != nullptr;
        }

        bool SubgraphInterface::HasInput(const VariableId& sourceID) const
        {
            for (auto& in : m_ins)
            {
                if (AZStd::find_if(in.inputs.begin(), in.inputs.end(), [&sourceID](const Input& input) { return input.sourceID == sourceID; }) != in.inputs.end())
                {
                    return true;
                }
            }

            for (auto& out : m_latents)
            {
                if (AZStd::find_if(out.returnValues.begin(), out.returnValues.end(), [&sourceID](const Input& returnValue) { return returnValue.sourceID == sourceID; }) != out.returnValues.end())
                {
                    return true;
                }
            }

            return false;
        }

        bool SubgraphInterface::HasLatent(const FunctionSourceId& sourceID) const
        {
            return AZStd::find_if(m_latents.begin(), m_latents.end(), [&sourceID](const Out& latent) { return latent.sourceID == sourceID; }) != m_latents.end();
        }

        bool SubgraphInterface::HasOnGraphStart() const
        {
            return m_hasOnGraphStart;
        }

        bool SubgraphInterface::HasOut(const FunctionSourceId& sourceID) const
        {
            for (auto& in : m_ins)
            {
                if (AZStd::find_if(in.outs.begin(), in.outs.end(), [&sourceID](const Out& out) { return out.sourceID == sourceID; }) != in.outs.end())
                {
                    return true;
                }
            }

            return false;
        }

        bool SubgraphInterface::HasOutput(const VariableId& sourceID) const
        {
            for (auto& in : m_ins)
            {
                for (auto& out : in.outs)
                {
                    if (AZStd::find_if(out.outputs.begin(), out.outputs.end(), [&sourceID](const Output& output) { return output.sourceID == sourceID; }) != out.outputs.end())
                    {
                        return true;
                    }
                }
            }

            for (auto& out : m_latents)
            {
                if (AZStd::find_if(out.outputs.begin(), out.outputs.end(), [&sourceID](const Output& output) { return output.sourceID == sourceID; }) != out.outputs.end())
                {
                    return true;
                }
            }

            return false;
        }

        bool SubgraphInterface::HasPublicFunctionality() const
        {
            return !(m_ins.empty() && m_latents.empty());
        }

        void SubgraphInterface::MarkActiveDefaultObject()
        {
            m_isActiveDefaultObject = true;
        }

        void SubgraphInterface::MarkRequiresConstructionParameters()
        {
            m_requiresConstructionParameters = true;
        }

        void SubgraphInterface::MarkRequiresConstructionParametersForDependencies()
        {
            m_requiresConstructionParametersForDependencies = true;
        }

        void SubgraphInterface::MarkUserVariable()
        {
            m_isUserVariable = true;
        }

        void SubgraphInterface::MarkExecutionCharacteristics(ExecutionCharacteristics characteristics)
        {
            m_executionCharacteristics = characteristics;
        }

        void SubgraphInterface::MarkOnGraphStart()
        {
            m_hasOnGraphStart = true;
            m_isActiveDefaultObject = true;
        }

        void SubgraphInterface::MergeExecutionCharacteristics(const SubgraphInterface& dependency)
        {
            m_hasOnGraphStart = m_hasOnGraphStart || dependency.HasOnGraphStart();
            m_isActiveDefaultObject = m_isActiveDefaultObject || dependency.IsActiveDefaultObject();
            m_areAllChildrenPure = m_areAllChildrenPure && dependency.IsMarkedPure();
            m_requiresConstructionParametersForDependencies = m_requiresConstructionParametersForDependencies || dependency.RequiresConstructionParameters();
        }

        In* SubgraphInterface::ModIn(const FunctionSourceId& sourceID)
        {
            return const_cast<In*>(FindIn(sourceID));
        }

        bool SubgraphInterface::operator==(const SubgraphInterface& rhs) const
        {
            if (m_isUserVariable != rhs.m_isUserVariable)
            {
                return false;
            }

            if (m_areAllChildrenPure != rhs.m_areAllChildrenPure)
            {
                return false;
            }

            if (m_isActiveDefaultObject != rhs.m_isActiveDefaultObject)
            {
                return false;
            }

            if (m_hasOnGraphStart != rhs.m_hasOnGraphStart)
            {
                return false;
            }

            if (m_requiresConstructionParameters != rhs.m_requiresConstructionParameters)
            {
                return false;
            }

            if (m_requiresConstructionParametersForDependencies != rhs.m_requiresConstructionParametersForDependencies)
            {
                return false;
            }

            if (m_executionCharacteristics != rhs.m_executionCharacteristics)
            {
                return false;
            }

            if (!(m_ins == rhs.m_ins))
            {
                return false;
            }

            if (!(m_latents == rhs.m_latents))
            {
                return false;
            }

            if (!(m_outKeys == rhs.m_outKeys))
            {
                return false;
            }

            if (!(IsNamespacePathEqual(m_namespacePath, rhs.m_namespacePath)))
            {
                return false;
            }

            return true;
        }

        // Populates the list of out keys
        AZ::Outcome<void, AZStd::string> SubgraphInterface::Parse()
        {
            m_outKeys.clear();

            for (const auto& in : m_ins)
            {
                for (const auto& out : in.outs)
                {
                    if (!AddOutKey(out.displayName))
                    {
                        return AZ::Failure(AZStd::string::format("Out %s was already in the list", out.displayName.c_str()));
                    }
                }
            }

            for (const auto& latent : m_latents)
            {
                if (!AddOutKey(latent.displayName))
                {
                    return AZ::Failure(AZStd::string::format("Out %s was already in the list", latent.displayName.c_str()));
                }
            }

            return AZ::Success();
        }

        void SubgraphInterface::Reflect(AZ::ReflectContext* refectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(refectContext))
            {
                serializeContext->Class<Input>()
                    ->Field("displayName", &Input::displayName)
                    ->Field("parsedName", &Input::parsedName)
                    ->Field("datum", &Input::datum)
                    ->Field("sourceID", &Input::sourceID)
                    ;

                serializeContext->Class<Output>()
                    ->Field("displayName", &Output::displayName)
                    ->Field("parsedName", &Output::parsedName)
                    ->Field("type", &Output::type)
                    ->Field("sourceID", &Output::sourceID)
                    ;

                serializeContext->Class<Out>()
                    ->Version(1)
                    ->Field("displayName", &Out::displayName)
                    ->Field("parsedName", &Out::parsedName)
                    ->Field("outputs", &Out::outputs)
                    ->Field("returnValues", &Out::returnValues)
                    ->Field("sourceID", &Out::sourceID)
                    ;

                serializeContext->Class<In>()
                    ->Version(1)
                    ->Field("displayName", &In::displayName)
                    ->Field("parsedName", &In::parsedName)
                    ->Field("inputs", &In::inputs)
                    ->Field("outs", &In::outs)
                    ->Field("isPure", &In::isPure)
                    ->Field("sourceID", &In::sourceID)
                    ;

                serializeContext->Class<SubgraphInterface>()
                    ->Version(SubgraphInterfaceCpp::Version::Current)
                    ->Field("areAllChildrenPure", &SubgraphInterface::m_areAllChildrenPure)
                    ->Field("hasOnGraphStart", &SubgraphInterface::m_hasOnGraphStart)
                    ->Field("isActiveDefaultObject", &SubgraphInterface::m_isActiveDefaultObject)
                    ->Field("ins", &SubgraphInterface::m_ins)
                    ->Field("latents", &SubgraphInterface::m_latents)
                    ->Field("outKeys", &SubgraphInterface::m_outKeys)
                    ->Field("namespacePath", &SubgraphInterface::m_namespacePath)
                    ->Field("executionCharacteristics", &SubgraphInterface::m_executionCharacteristics)
                    ->Field("requiresConstructionParameters", &SubgraphInterface::m_requiresConstructionParameters)
                    ->Field("requiresConstructionParametersForDependencies", &SubgraphInterface::m_requiresConstructionParametersForDependencies)
                    ;
            }
        }

        bool SubgraphInterface::RequiresConstructionParameters() const
        {
            return m_requiresConstructionParameters || m_requiresConstructionParametersForDependencies;
        }

        bool SubgraphInterface::RequiresConstructionParametersForDependencies() const
        {
            return m_requiresConstructionParametersForDependencies;
        }

        void SubgraphInterface::SetNamespacePath(const NamespacePath& namespacePath)
        {
            m_namespacePath = namespacePath;
        }

        AZStd::string SubgraphInterface::ToExecutionString() const
        {
            AZStd::string result;

            for (const auto& in : m_ins)
            {
                result += "\n";
                result += "In: ";
                result += in.displayName;
                result += "\n";

                for (const auto& out : in.outs)
                {
                    result += "\tOut: ";
                    result += out.displayName;
                    result += "\n";
                }
            }

            for (const auto& out : m_latents)
            {
                result += "Latent: ";
                result += out.displayName;
                result += "\n";
            }

            return result;
        }

        template<typename T>
        AZStd::string EntryPerLine(const T& values, size_t tabs)
        {
            AZStd::string result;

            for (const auto& value : values)
            {
                result += "\n";
                result += ToString(value, tabs);
            }

            return result;
        }

        const char* YorN(bool value)
        {
            return value ? "Y" : "N";
        }

        AZStd::string ToString(const In& in, size_t tabs)
        {
            AZStd::string result = AZStd::string::format("In: %s, Pure: %s", in.displayName.c_str(), YorN(in.isPure));

            if (!in.inputs.empty())
            {
                result += AZStd::string::format("%sInputs:", SubgraphInterfaceCpp::GetTabs(tabs + 1));
                result += ToString(in.inputs, tabs + 2);
            }

            if (!in.outs.empty())
            {
                result += AZStd::string::format("%sOuts:", SubgraphInterfaceCpp::GetTabs(tabs + 1));
                result += ToString(in.outs, false, tabs + 2);
            }

            return result;
        }

        AZStd::string ToString(const Ins& ins, size_t tabs)
        {
            return EntryPerLine(ins, tabs);
        }

        AZStd::string ToString(const Input& input, size_t tabs)
        {
            return AZStd::string::format("%sInput: %s, Type: %s", SubgraphInterfaceCpp::GetTabs(tabs), input.displayName.c_str(), Data::GetName(input.datum.GetType()).c_str());
        }

        AZStd::string ToString(const Inputs& inputs, size_t tabs)
        {
            return EntryPerLine(inputs, tabs);
        }

        AZStd::string ToString(const Out& out, bool isLatent, size_t tabs)
        {
            AZStd::string result = AZStd::string::format("%s%s: %s", SubgraphInterfaceCpp::GetTabs(tabs), isLatent ? "Latent" : "Out", out.displayName.c_str());

            if (!out.outputs.empty())
            {
                result += AZStd::string::format("%sOutputs:", SubgraphInterfaceCpp::GetTabs(tabs + 1));
                result += ToString(out.outputs, tabs + 2);
            }

            if (!out.returnValues.empty())
            {
                result += AZStd::string::format("%sReturn Values:", SubgraphInterfaceCpp::GetTabs(tabs + 1));
                result += ToString(out.returnValues, tabs + 2);
            }

            return result;
        }

        AZStd::string ToString(const Outs& outs, bool isLatent, size_t tabs)
        {
            AZStd::string result;

            for (const auto& value : outs)
            {
                result += "\n";
                result += ToString(value, isLatent, tabs);
            }

            return result;
        }

        AZStd::string ToString(const Output& output, size_t tabs)
        {
            return AZStd::string::format("%sOutput: %s, Type: %s", SubgraphInterfaceCpp::GetTabs(tabs), output.displayName.c_str(), Data::GetName(output.type).c_str());
        }

        AZStd::string ToString(const Outputs& outputs, size_t tabs)
        {
            return EntryPerLine(outputs, tabs);
        }

        AZStd::string ToString(const SubgraphInterface& subgraphInterface)
        {
            AZStd::string result("\n");
            result += subgraphInterface.GetName();
            result += "\n";
            result += AZStd::string::format("Is Active By Default: %s\n", YorN(subgraphInterface.IsActiveDefaultObject()));
            result += AZStd::string::format("Is Latent: %s\n", YorN(subgraphInterface.IsLatent()));
            result += AZStd::string::format("Is Pure: %s\n", YorN(subgraphInterface.GetExecutionCharacteristics() == ExecutionCharacteristics::Pure));
            result += AZStd::string::format("Is User Nodeable: %s\n", YorN(subgraphInterface.IsUserNodeable()));
            result += AZStd::string::format("Has Any Functionality: %s\n", YorN(subgraphInterface.HasAnyFunctionality()));
            result += AZStd::string::format("Has On Graph Start: %s\n", YorN(subgraphInterface.HasOnGraphStart()));

            const auto& ins = subgraphInterface.GetIns();
            if (!ins.empty())
            {
                result += "Ins:";
                result += ToString(ins, 1);
            }

            const auto& latents = subgraphInterface.GetLatentOuts();
            if (!latents.empty())
            {
                result += "Latents:";
                result += ToString(latents, 1);
            }

            result += "\n";

            return result;
        }
        
        SubgraphInterfacePtrConst SubgraphInterfaceSystem::GetMap(const FunctionSourceId& nodeTypeId) const
        {
            auto iter = m_mapsByNodeType.find(nodeTypeId);
            return iter != m_mapsByNodeType.end() ? iter->second : nullptr;
        }

        bool SubgraphInterfaceSystem::IsSimple(const FunctionSourceId& nodeTypeId) const
        {
            return m_mapsByNodeType.find(nodeTypeId) == m_mapsByNodeType.end();
        }

        bool SubgraphInterfaceSystem::RegisterMap(const FunctionSourceId& nodeTypeId, SubgraphInterfacePtrConst executionMap)
        {
            if (m_mapsByNodeType.find(nodeTypeId) != m_mapsByNodeType.end())
            {
                AZ_Warning("ScriptCanvas", false, "Node type %s: is already registered", nodeTypeId.ToString<AZStd::string>().data());
                return false;
            }

            m_mapsByNodeType.insert({ nodeTypeId, executionMap });
            return true;
        }
    }
} 
