/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EventManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeParameterNode, AnimGraphAllocator)

    static const char* sParameterNamesMember = "parameterNames";

    BlendTreeParameterNode::BlendTreeParameterNode()
        : AnimGraphNode()
    {
        // since this node dynamically sets the ports we don't really pre-create anything
        // The reinit function handles that
    }


    BlendTreeParameterNode::~BlendTreeParameterNode()
    {
    }


    void BlendTreeParameterNode::Reinit()
    {
        // Sort the parameter name mask in the way the parameters are stored in the anim graph.
        SortParameterNames(m_animGraph, m_parameterNames);

        // Iterate through the parameter name mask and find the corresponding cached value parameter indices.
        // This expects the parameter names to be sorted in the way the parameters are stored in the anim graph.
        m_parameterIndices.clear();
        for (const AZStd::string& parameterName : m_parameterNames)
        {
            const AZ::Outcome<size_t> parameterIndex = m_animGraph->FindValueParameterIndexByName(parameterName);
            // during removal of parameters, we could end up with a parameter that was removed until the node gets the mask updated
            if (parameterIndex.IsSuccess())
            {
                m_parameterIndices.push_back(static_cast<uint32>(parameterIndex.GetValue()));
            }
        }

        RemoveInternalAttributesForAllInstances();

        if (m_parameterIndices.empty())
        {
            // Parameter mask is empty, add ports for all parameters.
            const ValueParameterVector& valueParameters = m_animGraph->RecursivelyGetValueParameters();
            const uint32 valueParameterCount = static_cast<uint32>(valueParameters.size());
            InitOutputPorts(valueParameterCount);

            for (uint32 i = 0; i < valueParameterCount; ++i)
            {
                const ValueParameter* parameter = valueParameters[i];
                SetOutputPortName(static_cast<uint32>(i), parameter->GetName().c_str());

                m_outputPorts[i].m_portId = i;
                m_outputPorts[i].ClearCompatibleTypes();
                m_outputPorts[i].m_compatibleTypes[0] = parameter->GetType();
                if (GetTypeSupportsFloat(parameter->GetType()))
                {
                    m_outputPorts[i].m_compatibleTypes[1] = MCore::AttributeFloat::TYPE_ID;
                }
            }
        }
        else
        {
            // Parameter mask not empty, only add ports for the parameters in the mask.
            const size_t parameterCount = m_parameterIndices.size();
            InitOutputPorts(static_cast<uint32>(parameterCount));

            for (size_t i = 0; i < parameterCount; ++i)
            {
                const ValueParameter* parameter = m_animGraph->FindValueParameter(m_parameterIndices[i]);
                SetOutputPortName(static_cast<uint32>(i), parameter->GetName().c_str());

                m_outputPorts[i].m_portId = static_cast<uint32>(i);
                m_outputPorts[i].ClearCompatibleTypes();
                m_outputPorts[i].m_compatibleTypes[0] = parameter->GetType();
                if (GetTypeSupportsFloat(parameter->GetType()))
                {
                    m_outputPorts[i].m_compatibleTypes[1] = MCore::AttributeFloat::TYPE_ID;
                }
            }
        }

        InitInternalAttributesForAllInstances();

        AnimGraphNode::Reinit();
        SyncVisualObject();
    }


    bool BlendTreeParameterNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* BlendTreeParameterNode::GetPaletteName() const
    {
        return "Parameters";
    }


    // get the palette category
    AnimGraphObject::ECategory BlendTreeParameterNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // the main process method of the final node
    void BlendTreeParameterNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);

        if (m_parameterIndices.empty())
        {
            // output all anim graph instance parameter values into the output ports
            const uint32 numParameters = static_cast<uint32>(m_outputPorts.size());
            for (uint32 i = 0; i < numParameters; ++i)
            {
                GetOutputValue(animGraphInstance, i)->InitFrom(animGraphInstance->GetParameterValue(i));
            }
        }
        else
        {
            // output only the parameter values that have been selected in the parameter mask
            const size_t parameterCount = m_parameterIndices.size();
            for (size_t i = 0; i < parameterCount; ++i)
            {
                GetOutputValue(animGraphInstance, static_cast<AZ::u32>(i))->InitFrom(animGraphInstance->GetParameterValue(m_parameterIndices[i]));
            }
        }
    }


    // get the parameter index based on the port number
    uint32 BlendTreeParameterNode::GetParameterIndex(size_t portNr) const
    {
        // check if the parameter mask is empty
        if (m_parameterIndices.empty())
        {
            return static_cast<uint32>(portNr);
        }

        // get the mapped parameter index in case the given port is valid
        if (portNr < m_parameterIndices.size())
        {
            return m_parameterIndices[portNr];
        }

        // return failure, the input port is not in range
        return MCORE_INVALIDINDEX32;
    }


    bool BlendTreeParameterNode::GetTypeSupportsFloat(uint32 parameterType)
    {
        switch (parameterType)
        {
        case MCore::AttributeBool::TYPE_ID:
        case MCore::AttributeInt32::TYPE_ID:
            return true;
        default:
            // MCore::AttributeFloat is not considered because float->float conversion is not required
            return false;
        }
    }


    void BlendTreeParameterNode::SortParameterNames(AnimGraph* animGraph, AZStd::vector<AZStd::string>& outParameterNames)
    {
        // Iterate over all value parameters in the anim graph in the order they are stored.
        size_t currentIndex = 0;
        const size_t parameterCount = animGraph->GetNumValueParameters();
        for (size_t i = 0; i < parameterCount; ++i)
        {
            const ValueParameter* parameter = animGraph->FindValueParameter(i);

            // Check if the parameter is part of the parameter mask.
            auto parameterIterator = AZStd::find(outParameterNames.begin(), outParameterNames.end(), parameter->GetName());
            if (parameterIterator != outParameterNames.end())
            {
                // We found the parameter in the parameter mask. Swap the found element position with the current parameter index.
                // Increase the current parameter index as we found another parameter that got inserted.
                AZStd::iter_swap(outParameterNames.begin() + currentIndex, parameterIterator);
                currentIndex++;
            }
        }
    }


    const AZStd::vector<AZ::u32>& BlendTreeParameterNode::GetParameterIndices() const
    {
        return m_parameterIndices;
    }


    AZ::Color BlendTreeParameterNode::GetVisualColor() const
    {
        return AZ::Color(0.59f, 0.59f, 0.59f, 1.0f);
    }


    void BlendTreeParameterNode::AddParameter(const AZStd::string& parameterName)
    {
        m_parameterNames.emplace_back(parameterName);
        Reinit();
    }


    void BlendTreeParameterNode::SetParameters(const AZStd::vector<AZStd::string>& parameterNames)
    {
        m_parameterNames = parameterNames;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    void BlendTreeParameterNode::SetParameters(const AZStd::string& parameterNamesWithSemicolons)
    {
        AZStd::vector<AZStd::string> parameterNames;
        AzFramework::StringFunc::Tokenize(parameterNamesWithSemicolons.c_str(), parameterNames, ";", false, true);

        SetParameters(parameterNames);
    }


    AZStd::string BlendTreeParameterNode::ConstructParameterNamesString() const
    {
        return ConstructParameterNamesString(m_parameterNames);
    }


    AZStd::string BlendTreeParameterNode::ConstructParameterNamesString(const AZStd::vector<AZStd::string>& parameterNames)
    {
        AZStd::string result;

        for (const AZStd::string& parameterName : parameterNames)
        {
            if (!result.empty())
            {
                result += ';';
            }

            result += parameterName;
        }

        return result;
    }


    AZStd::string BlendTreeParameterNode::ConstructParameterNamesString(const AZStd::vector<AZStd::string>& parameterNames, const AZStd::vector<AZStd::string>& excludedParameterNames)
    {
        AZStd::string result;

        for (const AZStd::string& parameterName : parameterNames)
        {
            if (AZStd::find(excludedParameterNames.begin(), excludedParameterNames.end(), parameterName) == excludedParameterNames.end())
            {
                if (!result.empty())
                {
                    result += ';';
                }

                result += parameterName;
            }
        }

        return result;
    }


    void BlendTreeParameterNode::RemoveParameterByName(const AZStd::string& parameterName)
    {
        m_parameterNames.erase(AZStd::remove(m_parameterNames.begin(), m_parameterNames.end(), parameterName), m_parameterNames.end());
        if (m_animGraph)
        {
            Reinit();
        }
    }


    void BlendTreeParameterNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeParameterNode, AnimGraphNode>()
            ->Version(1)
            ->Field(sParameterNamesMember, &BlendTreeParameterNode::m_parameterNames)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeParameterNode>("Parameters", "Parameter node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("AnimGraphParameterMask"), &BlendTreeParameterNode::m_parameterNames, "Mask", "The visible and available of the node.")
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
        ;
    }

    AZStd::vector<AZStd::string> BlendTreeParameterNode::GetParameters() const
    {
        return m_parameterNames;
    }

    AnimGraph* BlendTreeParameterNode::GetParameterAnimGraph() const
    {
        return GetAnimGraph();
    }

    void BlendTreeParameterNode::ParameterMaskChanged(const AZStd::vector<AZStd::string>& newParameterMask)
    {
        if (newParameterMask.empty())
        {
            AZStd::vector<AZStd::string> newOutputPorts;
            const ValueParameterVector& valueParameters = GetAnimGraph()->RecursivelyGetValueParameters();
            for (const ValueParameter* valueParameter : valueParameters)
            {
                newOutputPorts.emplace_back(valueParameter->GetName());
            }
            GetEventManager().OnOutputPortsChanged(this, newOutputPorts, sParameterNamesMember, newParameterMask);
        }
        else
        {
            AZStd::vector<AZStd::string> newOutputPorts = newParameterMask;
            SortAndRemoveDuplicates(GetAnimGraph(), newOutputPorts);
            GetEventManager().OnOutputPortsChanged(this, newOutputPorts, sParameterNamesMember, newOutputPorts);
        }

        // Clear the history stack when the UI triggers a mask change.
        while (!m_deletedParameterNames.empty())
        {
            m_deletedParameterNames.pop();
        }
    }

    void BlendTreeParameterNode::AddRequiredParameters(AZStd::vector<AZStd::string>& parameterNames) const
    {
        // Add all connected parameters
        for (const AnimGraphNode::Port& port : GetOutputPorts())
        {
            if (port.m_connection)
            {
                parameterNames.emplace_back(port.GetNameString());
            }
        }

        SortAndRemoveDuplicates(GetAnimGraph(), parameterNames);
    }

    void BlendTreeParameterNode::ParameterAdded(const AZStd::string& newParameterName)
    {
        AZStd::vector<AZStd::string> newOutputPorts;
        const ValueParameterVector& valueParameters = GetAnimGraph()->RecursivelyGetValueParameters();
        for (const ValueParameter* valueParameter : valueParameters)
        {
            newOutputPorts.emplace_back(valueParameter->GetName());
        }

        // If new parameter matches the last deleted parameter, we add it back to the parameter mask.
        if (!m_deletedParameterNames.empty() && newParameterName == m_deletedParameterNames.top())
        {
            m_parameterNames.push_back(newParameterName);
            SortAndRemoveDuplicates(GetAnimGraph(), m_parameterNames); // make sure the mask is sorted correctly.
            m_deletedParameterNames.pop();
            GetEventManager().OnOutputPortsChanged(this, m_parameterNames, sParameterNamesMember, m_parameterNames);
        }
        else
        {
            if (m_parameterNames.empty())
            {
                // We don't use the parameter mask and show all of them. Pass an empty vector as serialized member value
                // so that the parameter mask won't be adjusted in the callbacks.
                GetEventManager().OnOutputPortsChanged(this, newOutputPorts, sParameterNamesMember, AZStd::vector<AZStd::string>());
            }
            else
            {
                GetEventManager().OnOutputPortsChanged(this, m_parameterNames, sParameterNamesMember, m_parameterNames);
            }
        }
    }

    void BlendTreeParameterNode::ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName)
    {
        // Check if the renamed parameter is part of the mask and rename
        // the mask entry in this case.
        bool parameterMaskChanged = false;
        AZStd::vector<AZStd::string> newOutputPortNames = m_parameterNames;
        for (AZStd::string& outputPortName : newOutputPortNames)
        {
            if (outputPortName == oldParameterName)
            {
                outputPortName = newParameterName;
                parameterMaskChanged = true;
                break;
            }
        }

        // Rename the actual output ports in all cases
        // (also when the parameter mask is empty and showing all parameters).
        const size_t numOutputPorts = m_outputPorts.size();
        for (size_t i = 0; i < numOutputPorts; ++i)
        {
            AnimGraphNode::Port& outputPort = m_outputPorts[i];
            if (outputPort.GetNameString() == oldParameterName)
            {
                SetOutputPortName(static_cast<AZ::u32>(i), newParameterName.c_str());
            }
        }

        if (parameterMaskChanged)
        {
            GetEventManager().OnOutputPortsChanged(this, newOutputPortNames, sParameterNamesMember, newOutputPortNames);
        }
    }

    void BlendTreeParameterNode::ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange)
    {
        AZ_UNUSED(beforeChange);

        // Check if any of the indices have changed
        // If we are looking at all the parameters, then something changed
        if (m_parameterNames.empty())
        {
            AZStd::vector<AZStd::string> newOutputPorts;
            const ValueParameterVector& valueParameters = GetAnimGraph()->RecursivelyGetValueParameters();
            for (const ValueParameter* valueParameter : valueParameters)
            {
                newOutputPorts.emplace_back(valueParameter->GetName());
            }
            // Keep the member variable as it is (thats why we pass m_parameterNames)
            GetEventManager().OnOutputPortsChanged(this, newOutputPorts, sParameterNamesMember, m_parameterNames);
        }
        else
        {
            // if not, we have to see if for all the parameters, the index is maintained between the before and after
            const bool somethingChanged = [&afterChange, this] {
                const size_t afterChangeParameterCount = afterChange.size();
                const size_t numOutputPorts = GetOutputPorts().size();
                for (size_t outputPort = 0; outputPort < numOutputPorts; ++outputPort)
                {
                    // two arrays are maintained, m_parameterIndices and m_parameterNames. Both have to be checked.
                    // GetParameterIndex ensures we're using the correct mapping
                    const size_t valueParameterIndex = GetParameterIndex(outputPort);
                    if (valueParameterIndex >= afterChangeParameterCount || afterChange[valueParameterIndex]->GetName() != m_parameterNames[outputPort])
                    {
                        return true;
                    }
                }
                return false;
            }();
            if (somethingChanged)
            {
                // The list of parameters is the same, we just need to re-sort it
                AZStd::vector<AZStd::string> newParameterNames = m_parameterNames;
                SortAndRemoveDuplicates(GetAnimGraph(), newParameterNames);
                GetEventManager().OnOutputPortsChanged(this, newParameterNames, sParameterNamesMember, newParameterNames);
            }
        }
    }

    void BlendTreeParameterNode::ParameterRemoved(const AZStd::string& oldParameterName)
    {
        // Stores the name of the parameter we just removed, in case we want to add them back to the mask later.
        if (AZStd::find(m_parameterNames.begin(), m_parameterNames.end(), oldParameterName) != m_parameterNames.end())
        {
            m_deletedParameterNames.push(oldParameterName);
        }

        // This may look unnatural, but the method ParameterOrderChanged deals with this as well, we just need to pass an empty before the change
        // and the current parameters after the change
        ParameterOrderChanged(ValueParameterVector(), GetAnimGraph()->RecursivelyGetValueParameters());
    }

} // namespace EMotionFX

