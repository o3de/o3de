/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioSystemEditor_wwise.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>

#include <ACETypes.h>
#include <AudioSystemControl_wwise.h>
#include <Common_wwise.h>

#include <QDir>

void InitWwiseResources()
{
    Q_INIT_RESOURCE(EditorWwise);
}

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    TImplControlType TagToType(const AZStd::string_view tag)
    {
        if (tag == Audio::WwiseXmlTags::WwiseEventTag)
        {
            return eWCT_WWISE_EVENT;
        }
        else if (tag == Audio::WwiseXmlTags::WwiseRtpcTag)
        {
            return eWCT_WWISE_RTPC;
        }
        else if (tag == Audio::WwiseXmlTags::WwiseAuxBusTag)
        {
            return eWCT_WWISE_AUX_BUS;
        }
        else if (tag == Audio::WwiseXmlTags::WwiseFileTag)
        {
            return eWCT_WWISE_SOUND_BANK;
        }
        else if (tag == Audio::WwiseXmlTags::WwiseSwitchTag)
        {
            return eWCT_WWISE_SWITCH_GROUP;
        }
        else if (tag == Audio::WwiseXmlTags::WwiseStateTag)
        {
            return eWCT_WWISE_GAME_STATE_GROUP;
        }
        return eWCT_INVALID;
    }

    //-------------------------------------------------------------------------------------------//
    const AZStd::string_view TypeToTag(const TImplControlType type)
    {
        switch (type)
        {
        case eWCT_WWISE_EVENT:
            return Audio::WwiseXmlTags::WwiseEventTag;
        case eWCT_WWISE_RTPC:
            return Audio::WwiseXmlTags::WwiseRtpcTag;
        case eWCT_WWISE_SWITCH:
            return Audio::WwiseXmlTags::WwiseValueTag;
        case eWCT_WWISE_AUX_BUS:
            return Audio::WwiseXmlTags::WwiseAuxBusTag;
        case eWCT_WWISE_SOUND_BANK:
            return Audio::WwiseXmlTags::WwiseFileTag;
        case eWCT_WWISE_GAME_STATE:
            return Audio::WwiseXmlTags::WwiseValueTag;
        case eWCT_WWISE_SWITCH_GROUP:
            return Audio::WwiseXmlTags::WwiseSwitchTag;
        case eWCT_WWISE_GAME_STATE_GROUP:
            return Audio::WwiseXmlTags::WwiseStateTag;
        }
        return "";
    }

    //-------------------------------------------------------------------------------------------//
    CAudioSystemEditor_wwise::CAudioSystemEditor_wwise()
    {
        InitWwiseResources();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemEditor_wwise::Reload()
    {
        // set all the controls as placeholder as we don't know if
        // any of them have been removed but still have connections to them
        for (const auto& idControlPair : m_controls)
        {
            TControlPtr control = idControlPair.second;
            if (control)
            {
                control->SetPlaceholder(true);
            }
        }

        // reload data
        m_loader.Load(this);

        m_connectionsByID.clear();
        UpdateConnectedStatus();
    }

    //-------------------------------------------------------------------------------------------//
    IAudioSystemControl* CAudioSystemEditor_wwise::CreateControl(const SControlDef& controlDefinition)
    {
        AZStd::string fullName = controlDefinition.m_name;
        IAudioSystemControl* parent = controlDefinition.m_parentControl;
        if (parent)
        {
            AZ::StringFunc::Path::Join(controlDefinition.m_parentControl->GetName().c_str(), fullName.c_str(), fullName);
        }

        if (!controlDefinition.m_path.empty())
        {
            AZ::StringFunc::Path::Join(controlDefinition.m_path.c_str(), fullName.c_str(), fullName);
        }

        CID id = GetID(fullName);

        IAudioSystemControl* control = GetControl(id);
        if (control)
        {
            if (control->IsPlaceholder())
            {
                control->SetPlaceholder(false);
                if (parent && parent->IsPlaceholder())
                {
                    parent->SetPlaceholder(false);
                }
            }
            return control;
        }
        else
        {
            TControlPtr newControl = AZStd::make_shared<IAudioSystemControl_wwise>(controlDefinition.m_name, id, controlDefinition.m_type);
            if (!parent)
            {
                parent = &m_rootControl;
            }

            parent->AddChild(newControl.get());
            newControl->SetParent(parent);
            newControl->SetLocalized(controlDefinition.m_isLocalized);
            m_controls[id] = newControl;
            return newControl.get();
        }
    }

    //-------------------------------------------------------------------------------------------//
    IAudioSystemControl* CAudioSystemEditor_wwise::GetControl(CID id) const
    {
        if (id != ACE_INVALID_CID)
        {
            auto it = m_controls.find(id);
            if (it != m_controls.end())
            {
                return it->second.get();
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    IAudioSystemControl* CAudioSystemEditor_wwise::GetControlByName(AZStd::string name, bool isLocalized, IAudioSystemControl* parent) const
    {
        if (parent)
        {
            AZ::StringFunc::Path::Join(parent->GetName().c_str(), name.c_str(), name);
        }

        if (isLocalized)
        {
            AZ::StringFunc::Path::Join(m_loader.GetLocalizationFolder().c_str(), name.c_str(), name);
        }
        return GetControl(GetID(name));
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CAudioSystemEditor_wwise::CreateConnectionToControl(EACEControlType atlControlType, IAudioSystemControl* middlewareControl)
    {
        if (middlewareControl)
        {
            middlewareControl->SetConnected(true);
            ++m_connectionsByID[middlewareControl->GetId()];

            if (middlewareControl->GetType() == eWCT_WWISE_RTPC)
            {
                switch (atlControlType)
                {
                    case EACEControlType::eACET_RTPC:
                    {
                        return AZStd::make_shared<CRtpcConnection>(middlewareControl->GetId());
                    }
                    case EACEControlType::eACET_SWITCH_STATE:
                    {
                        return AZStd::make_shared<CStateToRtpcConnection>(middlewareControl->GetId());
                    }
                }
            }

            return AZStd::make_shared<IAudioConnection>(middlewareControl->GetId());
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CAudioSystemEditor_wwise::CreateConnectionFromXMLNode(AZ::rapidxml::xml_node<char>* node, EACEControlType atlControlType)
    {
        if (node)
        {
            AZStd::string_view element(node->name());
            TImplControlType type = TagToType(element);
            if (type != AUDIO_IMPL_INVALID_TYPE)
            {
                AZStd::string name;
                AZStd::string_view localized;

                if (auto nameAttr = node->first_attribute(Audio::WwiseXmlTags::WwiseNameAttribute, 0, false);
                    nameAttr != nullptr)
                {
                    name = nameAttr->value();
                }

                if (auto localizedAttr = node->first_attribute(Audio::WwiseXmlTags::WwiseLocalizedAttribute, 0, false);
                    localizedAttr != nullptr)
                {
                    localized = localizedAttr->value();
                }

                bool isLocalized = AZ::StringFunc::Equal(localized, "true");

                // If the control wasn't found, create a placeholder.
                // We want to see that connection even if it's not in the middleware.
                // User could be viewing the editor without a middleware project.
                IAudioSystemControl* control = GetControlByName(name, isLocalized);
                if (!control)
                {
                    control = CreateControl(SControlDef(name, type));
                    if (control)
                    {
                        control->SetPlaceholder(true);
                        control->SetLocalized(isLocalized);
                    }
                }

                // If it's a switch we connect to one of the states within the switch
                if (type == eWCT_WWISE_SWITCH_GROUP || type == eWCT_WWISE_GAME_STATE_GROUP)
                {
                    if (auto childNode = node->first_node();
                        childNode != nullptr)
                    {
                        AZStd::string childName;
                        if (auto childNameAttr = childNode->first_attribute(Audio::WwiseXmlTags::WwiseNameAttribute, 0, false);
                            childNameAttr != nullptr)
                        {
                            childName = childNameAttr->value();
                        }

                        IAudioSystemControl* childControl = GetControlByName(childName, false, control);
                        if (!childControl)
                        {
                            childControl = CreateControl(SControlDef(
                                childName, type == eWCT_WWISE_SWITCH_GROUP ? eWCT_WWISE_SWITCH : eWCT_WWISE_GAME_STATE, false, control));
                        }
                        control = childControl;
                    }
                }

                if (control)
                {
                    control->SetConnected(true);
                    ++m_connectionsByID[control->GetId()];

                    if (type == eWCT_WWISE_RTPC)
                    {
                        switch (atlControlType)
                        {
                            case EACEControlType::eACET_RTPC:
                            {
                                TRtpcConnectionPtr connection = AZStd::make_shared<CRtpcConnection>(control->GetId());

                                float mult = 1.0f;
                                float shift = 0.0f;

                                if (auto multAttr = node->first_attribute(Audio::WwiseXmlTags::WwiseMutiplierAttribute, 0, false);
                                    multAttr != nullptr)
                                {
                                    mult = AZStd::stof(AZStd::string(multAttr->value()));
                                }

                                if (auto shiftAttr = node->first_attribute(Audio::WwiseXmlTags::WwiseShiftAttribute, 0, false);
                                    shiftAttr != nullptr)
                                {
                                    shift = AZStd::stof(AZStd::string(shiftAttr->value()));
                                }

                                connection->m_mult = mult;
                                connection->m_shift = shift;
                                return connection;
                            }
                            case EACEControlType::eACET_SWITCH_STATE:
                            {
                                TStateConnectionPtr connection = AZStd::make_shared<CStateToRtpcConnection>(control->GetId());

                                float value = 0.0f;
                                if (auto valueAttr = node->first_attribute(Audio::WwiseXmlTags::WwiseValueAttribute, 0, false);
                                    valueAttr != nullptr)
                                {
                                    value = AZStd::stof(AZStd::string(valueAttr->value()));
                                }

                                connection->m_value = value;
                                return connection;
                            }
                            case EACEControlType::eACET_ENVIRONMENT:
                            {
                                return AZStd::make_shared<IAudioConnection>(control->GetId());
                            }
                        }
                    }
                    else
                    {
                        return AZStd::make_shared<IAudioConnection>(control->GetId());
                    }
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    AZ::rapidxml::xml_node<char>* CAudioSystemEditor_wwise::CreateXMLNodeFromConnection(const TConnectionPtr connection, const EACEControlType atlControlType)
    {
        const IAudioSystemControl* control = GetControl(connection->GetID());
        if (control)
        {
            XmlAllocator& xmlAllocator(AudioControls::s_xmlAllocator);

            switch (control->GetType())
            {
                case AudioControls::eWCT_WWISE_SWITCH:
                    [[fallthrough]];
                case AudioControls::eWCT_WWISE_SWITCH_GROUP:
                    [[fallthrough]];
                case AudioControls::eWCT_WWISE_GAME_STATE:
                    [[fallthrough]];
                case AudioControls::eWCT_WWISE_GAME_STATE_GROUP:
                {
                    const IAudioSystemControl* parent = control->GetParent();
                    if (parent)
                    {
                        AZStd::string_view parentType = TypeToTag(parent->GetType());
                        auto switchNode = xmlAllocator.allocate_node(
                            AZ::rapidxml::node_element,
                            xmlAllocator.allocate_string(parentType.data())
                        );

                        auto switchNameAttr = xmlAllocator.allocate_attribute(
                            Audio::WwiseXmlTags::WwiseNameAttribute,
                            xmlAllocator.allocate_string(parent->GetName().c_str())
                        );

                        auto stateNode = xmlAllocator.allocate_node(
                            AZ::rapidxml::node_element,
                            Audio::WwiseXmlTags::WwiseValueTag
                        );

                        auto stateNameAttr = xmlAllocator.allocate_attribute(
                            Audio::WwiseXmlTags::WwiseNameAttribute,
                            xmlAllocator.allocate_string(control->GetName().c_str())
                        );

                        switchNode->append_attribute(switchNameAttr);
                        stateNode->append_attribute(stateNameAttr);
                        switchNode->append_node(stateNode);
                        return switchNode;
                    }
                    break;
                }

                case AudioControls::eWCT_WWISE_RTPC:
                {
                    auto connectionNode = xmlAllocator.allocate_node(
                        AZ::rapidxml::node_element,
                        xmlAllocator.allocate_string(TypeToTag(control->GetType()).data())
                    );

                    auto nameAttr = xmlAllocator.allocate_attribute(
                        Audio::WwiseXmlTags::WwiseNameAttribute,
                        xmlAllocator.allocate_string(control->GetName().c_str())
                    );

                    connectionNode->append_attribute(nameAttr);

                    if (atlControlType == eACET_RTPC)
                    {
                        AZStd::shared_ptr<const CRtpcConnection> rtpcConnection = AZStd::static_pointer_cast<const CRtpcConnection>(connection);
                        if (rtpcConnection->m_mult != 1.f)
                        {
                            auto multAttr = xmlAllocator.allocate_attribute(
                                Audio::WwiseXmlTags::WwiseMutiplierAttribute,
                                xmlAllocator.allocate_string(AZStd::to_string(rtpcConnection->m_mult).c_str())
                            );

                            connectionNode->append_attribute(multAttr);
                        }

                        if (rtpcConnection->m_shift != 0.f)
                        {
                            auto shiftAttr = xmlAllocator.allocate_attribute(
                                Audio::WwiseXmlTags::WwiseShiftAttribute,
                                xmlAllocator.allocate_string(AZStd::to_string(rtpcConnection->m_shift).c_str())
                            );

                            connectionNode->append_attribute(shiftAttr);
                        }
                    }
                    else if (atlControlType == eACET_SWITCH_STATE)
                    {
                        AZStd::shared_ptr<const CStateToRtpcConnection> stateConnection = AZStd::static_pointer_cast<const CStateToRtpcConnection>(connection);

                        auto valueAttr = xmlAllocator.allocate_attribute(
                            Audio::WwiseXmlTags::WwiseValueAttribute,
                            xmlAllocator.allocate_string(AZStd::to_string(stateConnection->m_value).c_str())
                        );

                        connectionNode->append_attribute(valueAttr);
                    }

                    return connectionNode;
                }

                case AudioControls::eWCT_WWISE_EVENT:
                    [[fallthrough]];
                case AudioControls::eWCT_WWISE_AUX_BUS:
                {
                    auto connectionNode = xmlAllocator.allocate_node(
                        AZ::rapidxml::node_element,
                        xmlAllocator.allocate_string(TypeToTag(control->GetType()).data())
                    );

                    auto nameAttr = xmlAllocator.allocate_attribute(
                        Audio::WwiseXmlTags::WwiseNameAttribute,
                        xmlAllocator.allocate_string(control->GetName().c_str())
                    );

                    connectionNode->append_attribute(nameAttr);
                    return connectionNode;
                }

                case AudioControls::eWCT_WWISE_SOUND_BANK:
                {
                    auto connectionNode = xmlAllocator.allocate_node(
                        AZ::rapidxml::node_element,
                        xmlAllocator.allocate_string(TypeToTag(control->GetType()).data())
                    );

                    auto nameAttr = xmlAllocator.allocate_attribute(
                        Audio::WwiseXmlTags::WwiseNameAttribute,
                        xmlAllocator.allocate_string(control->GetName().c_str())
                    );

                    connectionNode->append_attribute(nameAttr);

                    if (control->IsLocalized())
                    {
                        auto locAttr = xmlAllocator.allocate_attribute(
                            Audio::WwiseXmlTags::WwiseLocalizedAttribute,
                            xmlAllocator.allocate_string("true")
                        );

                        connectionNode->append_attribute(locAttr);
                    }

                    return connectionNode;
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    const AZStd::string_view CAudioSystemEditor_wwise::GetTypeIcon(TImplControlType type) const
    {
        switch (type)
        {
        case eWCT_WWISE_EVENT:
            return ":/Editor/WwiseIcons/event_nor.svg";
        case eWCT_WWISE_RTPC:
            return ":/Editor/WwiseIcons/gameparameter_nor.svg";
        case eWCT_WWISE_SWITCH:
            return ":/Editor/WwiseIcons/switch_nor.svg";
        case eWCT_WWISE_AUX_BUS:
            return ":/Editor/WwiseIcons/auxbus_nor.svg";
        case eWCT_WWISE_SOUND_BANK:
            return ":/Editor/WwiseIcons/soundbank_nor.svg";
        case eWCT_WWISE_GAME_STATE:
            return ":/Editor/WwiseIcons/state_nor.svg";
        case eWCT_WWISE_SWITCH_GROUP:
            return ":/Editor/WwiseIcons/switchgroup_nor.svg";
        case eWCT_WWISE_GAME_STATE_GROUP:
            return ":/Editor/WwiseIcons/stategroup_nor.svg";
        default:
            // should make a "default"/empty icon...
            return ":/Editor/WwiseIcons/switchgroup_nor.svg";
        }
    }

    const AZStd::string_view CAudioSystemEditor_wwise::GetTypeIconSelected(TImplControlType type) const
    {
        switch (type)
        {
        case eWCT_WWISE_EVENT:
            return ":/Editor/WwiseIcons/event_nor_hover.svg";
        case eWCT_WWISE_RTPC:
            return ":/Editor/WwiseIcons/gameparameter_nor_hover.svg";
        case eWCT_WWISE_SWITCH:
            return ":/Editor/WwiseIcons/switch_nor_hover.svg";
        case eWCT_WWISE_AUX_BUS:
            return ":/Editor/WwiseIcons/auxbus_nor_hover.svg";
        case eWCT_WWISE_SOUND_BANK:
            return ":/Editor/WwiseIcons/soundbank_nor_hover.svg";
        case eWCT_WWISE_GAME_STATE:
            return ":/Editor/WwiseIcons/state_nor_hover.svg";
        case eWCT_WWISE_SWITCH_GROUP:
            return ":/Editor/WwiseIcons/switchgroup_nor_hover.svg";
        case eWCT_WWISE_GAME_STATE_GROUP:
            return ":/Editor/WwiseIcons/stategroup_nor_hover.svg";
        default:
            // should make a "default"/empty icon...
            return ":/Editor/WwiseIcons/switchgroup_nor_hover.svg";
        }
    }

    //-------------------------------------------------------------------------------------------//
    EACEControlType CAudioSystemEditor_wwise::ImplTypeToATLType(TImplControlType type) const
    {
        switch (type)
        {
        case eWCT_WWISE_EVENT:
            return eACET_TRIGGER;
        case eWCT_WWISE_RTPC:
            return eACET_RTPC;
        case eWCT_WWISE_SWITCH:
        case eWCT_WWISE_GAME_STATE:
            return eACET_SWITCH_STATE;
        case eWCT_WWISE_AUX_BUS:
            return eACET_ENVIRONMENT;
        case eWCT_WWISE_SOUND_BANK:
            return eACET_PRELOAD;
        case eWCT_WWISE_GAME_STATE_GROUP:
        case eWCT_WWISE_SWITCH_GROUP:
            return eACET_SWITCH;
        }
        return eACET_NUM_TYPES;
    }

    //-------------------------------------------------------------------------------------------//
    TImplControlTypeMask CAudioSystemEditor_wwise::GetCompatibleTypes(EACEControlType atlControlType) const
    {
        switch (atlControlType)
        {
        case eACET_TRIGGER:
            return eWCT_WWISE_EVENT;
        case eACET_RTPC:
            return eWCT_WWISE_RTPC;
        case eACET_SWITCH:
            return (eWCT_WWISE_SWITCH | eWCT_WWISE_GAME_STATE);
        case eACET_SWITCH_STATE:
            return (eWCT_WWISE_SWITCH | eWCT_WWISE_GAME_STATE | eWCT_WWISE_RTPC);
        case eACET_ENVIRONMENT:
            return (eWCT_WWISE_AUX_BUS | eWCT_WWISE_RTPC);
        case eACET_PRELOAD:
            return eWCT_WWISE_SOUND_BANK;
        }
        return AUDIO_IMPL_INVALID_TYPE;
    }

    //-------------------------------------------------------------------------------------------//
    CID CAudioSystemEditor_wwise::GetID(const AZStd::string_view name) const
    {
        return Audio::AudioStringToID<CID>(name.data());
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::string CAudioSystemEditor_wwise::GetName() const
    {
        return "Wwise";
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemEditor_wwise::UpdateConnectedStatus()
    {
        for (const auto& idCountPair : m_connectionsByID)
        {
            if (idCountPair.second > 0)
            {
                IAudioSystemControl* control = GetControl(idCountPair.first);
                if (control)
                {
                    control->SetConnected(true);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemEditor_wwise::ConnectionRemoved(IAudioSystemControl* control)
    {
        int connectionCount = m_connectionsByID[control->GetId()] - 1;
        if (connectionCount <= 0)
        {
            connectionCount = 0;
            control->SetConnected(false);
        }
        m_connectionsByID[control->GetId()] = connectionCount;
    }

    //-------------------------------------------------------------------------------------------//
    AZ::IO::FixedMaxPath CAudioSystemEditor_wwise::GetDataPath() const
    {
        auto projectPath = AZ::IO::FixedMaxPath{ AZ::Utils::GetProjectPath() };
        return (projectPath / "sounds" / "wwise_project");
    }

} // namespace AudioControls
