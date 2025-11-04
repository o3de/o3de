/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiEditorAnimationBus.h"
#include <LyShine/UiEditorDLLBus.h>
#include "UiAnimViewAnimNode.h"
#include "UiAnimViewTrack.h"
#include "UiAnimViewSequence.h"
#include "UiAnimViewUndo.h"
#include "UiAnimViewNodeFactories.h"
#include "AnimationContext.h"
#include "UiAnimViewSequenceManager.h"
#include "Util/EditorUtils.h"
#include "ViewManager.h"
#include "Clipboard.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <LyShine/Bus/UiAnimationBus.h>

bool CUiAnimViewAnimNode::s_isForcingAnimationBecausePropertyChanged = false;

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNodeBundle::AppendAnimNode(CUiAnimViewAnimNode* pNode)
{
    stl::push_back_unique(m_animNodes, pNode);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNodeBundle::AppendAnimNodeBundle(const CUiAnimViewAnimNodeBundle& bundle)
{
    for (auto iter = bundle.m_animNodes.begin(); iter != bundle.m_animNodes.end(); ++iter)
    {
        AppendAnimNode(*iter);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNodeBundle::ExpandAll(bool bAlsoExpandParentNodes)
{
    std::set<CUiAnimViewNode*> nodesToExpand;
    std::copy(m_animNodes.begin(), m_animNodes.end(), std::inserter(nodesToExpand, nodesToExpand.end()));

    if (bAlsoExpandParentNodes)
    {
        for (auto iter = nodesToExpand.begin(); iter != nodesToExpand.end(); ++iter)
        {
            CUiAnimViewNode* pNode = *iter;

            for (CUiAnimViewNode* pParent = pNode->GetParentNode(); pParent; pParent = pParent->GetParentNode())
            {
                nodesToExpand.insert(pParent);
            }
        }
    }

    for (auto iter = nodesToExpand.begin(); iter != nodesToExpand.end(); ++iter)
    {
        (*iter)->SetExpanded(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNodeBundle::CollapseAll()
{
    for (auto iter = m_animNodes.begin(); iter != m_animNodes.end(); ++iter)
    {
        (*iter)->SetExpanded(false);
    }
}

//////////////////////////////////////////////////////////////////////////
const bool CUiAnimViewAnimNodeBundle::DoesContain(const CUiAnimViewNode* pTargetNode)
{
    return stl::find(m_animNodes, pTargetNode);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNodeBundle::Clear()
{
    m_animNodes.clear();
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNode::CUiAnimViewAnimNode(IUiAnimSequence* pSequence, IUiAnimNode* pAnimNode, CUiAnimViewNode* pParentNode)
    : CUiAnimViewNode(pParentNode)
    , m_pAnimSequence(pSequence)
    , m_pAnimNode(pAnimNode)
    , m_pNodeUiAnimator(nullptr)
{
    if (pAnimNode)
    {
        // Search for child nodes
        const int nodeCount = pSequence->GetNodeCount();
        for (int i = 0; i < nodeCount; ++i)
        {
            IUiAnimNode* pNode = pSequence->GetNode(i);
            IUiAnimNode* pNodeParentNode = pNode->GetParent();

            // If our node is the parent, then the current node is a child of it
            if (pAnimNode == pNodeParentNode)
            {
                CUiAnimViewAnimNodeFactory animNodeFactory;
                CUiAnimViewAnimNode* pNewUiAVAnimNode = animNodeFactory.BuildAnimNode(pSequence, pNode, this);
                m_childNodes.push_back(std::unique_ptr<CUiAnimViewNode>(pNewUiAVAnimNode));
            }
        }

        // Search for tracks
        const int trackCount = pAnimNode->GetTrackCount();
        for (int i = 0; i < trackCount; ++i)
        {
            IUiAnimTrack* pTrack = pAnimNode->GetTrackByIndex(i);

            CUiAnimViewTrackFactory trackFactory;
            CUiAnimViewTrack* pNewUiAVTrack = trackFactory.BuildTrack(pTrack, this, this);
            m_childNodes.push_back(std::unique_ptr<CUiAnimViewNode>(pNewUiAVTrack));
        }

        // Set owner to update entity UI Animation system entity IDs and remove it again
        UiAnimNodeBus::EventResult(m_nodeEntityId, m_pAnimNode.get(), &UiAnimNodeBus::Events::GetAzEntityId);

        m_pAnimNode->SetNodeOwner(nullptr);
    }

    SortNodes();

    m_bExpanded = IsGroupNode();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::UiElementPropertyChanged()
{
    // we can detect which properties changed by comparing with a cached copy

    // then we need to tell the AnimNode what changed. This can add new tracks
    // and record new key values

    s_isForcingAnimationBecausePropertyChanged = true;

    bool valueChanged = false;

    if (m_nodeEntityId.IsValid() && !m_azEntityDataCache.empty())
    {
        AZ::Entity* pNodeEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(pNodeEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_nodeEntityId);

        // Check that the entity referenced by this AnimNode exists. There is a possiblity that
        // it has been deleted (in which case this anim node is drawn in red).
        if (pNodeEntity)
        {
            // The entity still exists, compare with cached data to see what changed
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "No serialization context found");

            AZ::IO::ByteContainerStream<const AZStd::string> charStream(&m_azEntityDataCache);
            AZ::Entity* oldEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(charStream);

            const AZ::Entity::ComponentArrayType& oldComponents = oldEntity->GetComponents();
            const AZ::Entity::ComponentArrayType& newComponents = pNodeEntity->GetComponents();

            // if the number of components has changed then a component has just been added of removed
            // we do not record such changes
            if (oldComponents.size() == newComponents.size())
            {
                for (AZ::u32 componentIndex = 0; componentIndex < oldComponents.size(); componentIndex++)
                {
                    AZ::Component* oldComponent = oldComponents[componentIndex];
                    AZ::Component* newComponent = newComponents[componentIndex];

                    [[maybe_unused]] AZ::Uuid oldComponentType = oldComponent->RTTI_GetType();
                    [[maybe_unused]] AZ::Uuid newComponentType = newComponent->RTTI_GetType();

                    AZ_Assert(oldComponentType == newComponentType, "Components have different types");

                    const AZ::Uuid& classId = AZ::SerializeTypeInfo<AZ::Component>::GetUuid(oldComponent);

                    const AZ::SerializeContext::ClassData* classData = context->FindClassData(classId);

                    // we would like to be able to know what changed
                    for (const AZ::SerializeContext::ClassElement& element : classData->m_elements)
                    {
                        if (element.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
                        {
                            if (BaseClassPropertyPotentiallyChanged(context, newComponent, oldComponent, element, element.m_offset))
                            {
                                valueChanged = true;
                            }
                        }
                        else
                        {
                            if (HasComponentParamValueAzChanged(newComponent, oldComponent, element, element.m_offset))
                            {
                                valueChanged = true;
                                AzEntityPropertyChanged(oldComponent, newComponent, element, element.m_offset);
                            }
                        }
                    }
                }
            }

            delete oldEntity;

            // save a new cache of the current values of all the entities properties
            // do this before calling OnKeysChanged because that can end up causing
            // UiElementPropertyChanged to get called again
            AZ::IO::ByteContainerStream<AZStd::string> saveStream(&m_azEntityDataCache);
            [[maybe_unused]] bool success = AZ::Utils::SaveObjectToStream(saveStream, AZ::ObjectStream::ST_XML, pNodeEntity);
            AZ_Assert(success, "Failed to serialize canvas entity to XML");

            if (valueChanged)
            {
                GetSequence()->OnKeysChanged();
            }
        }
    }

    s_isForcingAnimationBecausePropertyChanged = false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::AzEntityPropertyChanged(
    AZ::Component* oldComponent, AZ::Component* newComponent,
    const AZ::SerializeContext::ClassElement& element, size_t offset)
{
    if (element.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
    {
        // this is a base class of a member within the component e.g. the base class of an asset ref
        // we do not yet handle animating such values
        return;
    }

    const float time = GetSequence()->GetTime();

    UiAnimParamData param(newComponent->GetId(), element.m_name, element.m_typeId, offset);

    CUiAnimViewTrack* track = GetTrackForParameterAz(param);

    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);

    if (!track && pAnimationContext->IsRecording())
    {
        // create a new track
        track = CreateTrackAz(param);

        // not sure if we really want to do this but it seems useful, if the time is not zero
        // then we add a value at time zero which is the original value before this change
        // that caused a track to be created
        if (time != 0.0f)
        {
            SetComponentParamValueAz(0.0f, newComponent, oldComponent, element, offset);
        }
    }

    // add a new value
    if (track)
    {
        if (!pAnimationContext->IsRecording())
        {
            // Offset all keys by move amount.
            if (element.m_typeId == AZ::SerializeTypeInfo<float>::GetUuid())
            {
                float prevValue;
                track->GetValue(time, prevValue);
                //float offset = newElementFloat - prevValue;
                //track->OffsetKeyPosition(offset);
            }
        }
        else
        {
            UiAnimUndo::Record(new CUndoTrackObject(track, GetSequence()));
            const int flags = m_pAnimNode->GetFlags();
            // Set the selected flag to enable record when unselected camera is moved through viewport
            m_pAnimNode->SetFlags(flags | eUiAnimNodeFlags_EntitySelected);
            SetComponentParamValueAz(time, newComponent, newComponent, element, offset);
            m_pAnimNode->SetFlags(flags);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::AzCreateCompoundTrackIfNeeded(
    float time, AZ::Component* newComponent, AZ::Component* oldComponent,
    const AZ::SerializeContext::ClassElement& element, size_t offset)
{
    UiAnimParamData param(newComponent->GetId(), element.m_name, element.m_typeId, offset);
    CUiAnimViewTrack* track = GetTrackForParameterAz(param);

    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);

    if (!track && pAnimationContext->IsRecording())
    {
        // create a new track
        track = CreateTrackAz(param);

        // if time is not 0 then add the original values for all sub tracks
        if (time != 0.0f)
        {
            SetComponentParamValueAz(0.0f, newComponent, oldComponent, element, offset);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::SetComponentParamValueAz(float time,
    AZ::Component* dstComponent, AZ::Component* srcComponent,
    const AZ::SerializeContext::ClassElement& element, size_t offset)
{
    void* srcElementData = reinterpret_cast<char*>(srcComponent) + offset;

    UiAnimParamData param(dstComponent->GetId(), element.m_name, element.m_typeId, offset);

    if (element.m_typeId == AZ::SerializeTypeInfo<float>::GetUuid())
    {
        float srcElementFloat = *reinterpret_cast<float*>(srcElementData);
        m_pAnimNode->SetParamValueAz(time, param, srcElementFloat);
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<bool>::GetUuid())
    {
        bool srcElementValue = *reinterpret_cast<bool*>(srcElementData);
        m_pAnimNode->SetParamValueAz(time, param, srcElementValue);
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<int>::GetUuid())
    {
        // int srcElementValue = *reinterpret_cast<int*>(srcElementData);
        //        m_pAnimNode->SetParamValueAz(time, param, srcElementValue);
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<unsigned int>::GetUuid())
    {
        // unsigned int srcElementValue = *reinterpret_cast<unsigned int*>(srcElementData);
        //        m_pAnimNode->SetParamValueAz(time, param, srcElementValue);
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<AZ::Vector2>::GetUuid())
    {
        AZ::Vector2 srcElementValue = *reinterpret_cast<AZ::Vector2*>(srcElementData);
        m_pAnimNode->SetParamValueAz(time, param, srcElementValue);
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<AZ::Vector3>::GetUuid())
    {
        AZ::Vector3 srcElementValue = *reinterpret_cast<AZ::Vector3*>(srcElementData);
        m_pAnimNode->SetParamValueAz(time, param, srcElementValue);
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<AZ::Vector4>::GetUuid())
    {
        AZ::Vector4 srcElementValue = *reinterpret_cast<AZ::Vector4*>(srcElementData);
        m_pAnimNode->SetParamValueAz(time, param, srcElementValue);
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<AZ::Color>::GetUuid())
    {
        AZ::Color srcElementValue = *reinterpret_cast<AZ::Color*>(srcElementData);
        m_pAnimNode->SetParamValueAz(time, param, srcElementValue);
    }
    else
    {
        // it is not a float

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(context, "No serialization context found");

        // it is not a float
        const AZ::SerializeContext::ClassData* classData = context->FindClassData(element.m_typeId);
        if (classData && !classData->m_elements.empty())
        {
            AzCreateCompoundTrackIfNeeded(time, dstComponent, srcComponent, element, element.m_offset);

            for (const AZ::SerializeContext::ClassElement& subElement : classData->m_elements)
            {
                SetComponentParamValueAz(time,
                    dstComponent, srcComponent,
                    subElement, offset + subElement.m_offset);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::HasComponentParamValueAzChanged(
    AZ::Component* dstComponent, AZ::Component* srcComponent,
    const AZ::SerializeContext::ClassElement& element, size_t offset)
{
    if (element.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
    {
        // this is a base class of a member within the component e.g. the base class of an asset ref
        // we do not yet handle animating such values
        return false;
    }

    bool changed = false;
    const float floatEpsilon = 0.0001f;

    void* dstElementData = reinterpret_cast<char*>(dstComponent) + offset;
    void* srcElementData = reinterpret_cast<char*>(srcComponent) + offset;

    if (element.m_typeId == AZ::SerializeTypeInfo<float>::GetUuid())
    {
        float dstElementValue = *reinterpret_cast<float*>(dstElementData);
        float srcElementValue = *reinterpret_cast<float*>(srcElementData);

        changed = dstElementValue != srcElementValue;
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<bool>::GetUuid())
    {
        bool dstElementValue = *reinterpret_cast<bool*>(dstElementData);
        bool srcElementValue = *reinterpret_cast<bool*>(srcElementData);

        changed = dstElementValue != srcElementValue;
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<int>::GetUuid())
    {
        unsigned int dstElementValue = *reinterpret_cast<int*>(dstElementData);
        unsigned int srcElementValue = *reinterpret_cast<int*>(srcElementData);

        changed = dstElementValue != srcElementValue;
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<unsigned int>::GetUuid())
    {
        unsigned int dstElementValue = *reinterpret_cast<unsigned int*>(dstElementData);
        unsigned int srcElementValue = *reinterpret_cast<unsigned int*>(srcElementData);

        changed = dstElementValue != srcElementValue;
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<AZ::Vector2>::GetUuid())
    {
        AZ::Vector2 dstElementValue = *reinterpret_cast<AZ::Vector2*>(dstElementData);
        AZ::Vector2 srcElementValue = *reinterpret_cast<AZ::Vector2*>(srcElementData);

        changed = !dstElementValue.IsClose(srcElementValue, floatEpsilon);
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<AZ::Vector3>::GetUuid())
    {
        AZ::Vector3 dstElementValue = *reinterpret_cast<AZ::Vector3*>(dstElementData);
        AZ::Vector3 srcElementValue = *reinterpret_cast<AZ::Vector3*>(srcElementData);

        changed = !dstElementValue.IsClose(srcElementValue, floatEpsilon);
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<AZ::Vector4>::GetUuid())
    {
        AZ::Vector4 dstElementValue = *reinterpret_cast<AZ::Vector4*>(dstElementData);
        AZ::Vector4 srcElementValue = *reinterpret_cast<AZ::Vector4*>(srcElementData);

        changed = !dstElementValue.IsClose(srcElementValue, floatEpsilon);
    }
    else if (element.m_typeId == AZ::SerializeTypeInfo<AZ::Color>::GetUuid())
    {
        AZ::Color dstElementValue = *reinterpret_cast<AZ::Color*>(dstElementData);
        AZ::Color srcElementValue = *reinterpret_cast<AZ::Color*>(srcElementData);

        changed = !dstElementValue.IsClose(srcElementValue, floatEpsilon);
    }
    else
    {
        // it is not a float
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(context, "No serialization context found");

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(element.m_typeId);
        if (classData && !classData->m_elements.empty())
        {
            // this is an aggregate type, try finding any floats within
            // we would like to be able to know what changed

            for (const AZ::SerializeContext::ClassElement& subElement : classData->m_elements)
            {
                if (HasComponentParamValueAzChanged(dstComponent, srcComponent,
                        subElement, offset + subElement.m_offset))
                {
                    changed = true;
                    break;
                }
            }
        }
    }

    return changed;
}

bool CUiAnimViewAnimNode::BaseClassPropertyPotentiallyChanged(
    AZ::SerializeContext* context,
    AZ::Component* dstComponent, AZ::Component* srcComponent,
    const AZ::SerializeContext::ClassElement& element, [[maybe_unused]] size_t offset)
{
    size_t baseClassOffset = element.m_offset;
    const AZ::Uuid& baseClassId = element.m_typeId;
    const AZ::SerializeContext::ClassData* baseClassData = context->FindClassData(baseClassId);

    bool valueChanged = false;
    if (baseClassData)
    {
        for (const AZ::SerializeContext::ClassElement& baseElement : baseClassData->m_elements)
        {
            size_t baseOffset = baseClassOffset + baseElement.m_offset;
            if (baseElement.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
            {
                if (BaseClassPropertyPotentiallyChanged(context, dstComponent, srcComponent, baseElement, baseOffset))
                {
                    valueChanged = true;
                }
            }
            else
            {
                if (HasComponentParamValueAzChanged(dstComponent, srcComponent, baseElement, baseOffset))
                {
                    valueChanged = true;
                    AzEntityPropertyChanged(srcComponent, dstComponent, baseElement, baseOffset);
                }
            }
        }
    }

    return valueChanged;
}


//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::BindToEditorObjects()
{
    if (!IsActive())
    {
        return;
    }

    CUiAnimViewSequenceNotificationContext context(GetSequence());

    // if this node represents an AZ entity then register for updates
    if (m_nodeEntityId.IsValid())
    {
        if (!UiElementChangeNotificationBus::Handler::BusIsConnected())
        {
            // register for change events on the AZ entity
            UiElementChangeNotificationBus::Handler::BusConnect(m_nodeEntityId);

            AZ::Entity* pNodeEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(pNodeEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_nodeEntityId);
            if (pNodeEntity)
            {
                // save a cache of the current values of all the entities properties
                AZ::IO::ByteContainerStream<AZStd::string> charStream(&m_azEntityDataCache);
                [[maybe_unused]] bool success = AZ::Utils::SaveObjectToStream(charStream, AZ::ObjectStream::ST_XML, pNodeEntity);
                AZ_Assert(success, "Failed to serialize canvas entity to XML");
            }
        }
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = (CUiAnimViewAnimNode*)pChildNode;
            pChildAnimNode->BindToEditorObjects();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::UnBindFromEditorObjects()
{
    CUiAnimViewSequenceNotificationContext context(GetSequence());

    // UI_ANIMATION_REVISIT - what of this function is really needed?

    if (m_pAnimNode)
    {
        m_pAnimNode->SetNodeOwner(nullptr);
    }

    if (m_pNodeUiAnimator)
    {
        m_pNodeUiAnimator->UnBind(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = (CUiAnimViewAnimNode*)pChildNode;
            pChildAnimNode->UnBindFromEditorObjects();
        }
    }

    // if this node represents an AZ entity then unregister for updates
    if (m_nodeEntityId.IsValid())
    {
        // unregister for change events on the Az entity
        UiElementChangeNotificationBus::Handler::BusDisconnect(m_nodeEntityId);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::IsBoundToEditorObjects() const
{
    return m_pAnimNode ? (m_pAnimNode->GetNodeOwner() != NULL) : false;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNode* CUiAnimViewAnimNode::CreateSubNode(const QString& name, const EUiAnimNodeType animNodeType, AZ::Entity* pEntity, bool listen)
{
    const bool bIsGroupNode = IsGroupNode();
    AZ_Assert(bIsGroupNode, "bIsGroupNode is nullptr.");
    if (!bIsGroupNode)
    {
        return nullptr;
    }

    // Create new node
    IUiAnimNode* pNewAnimNode = m_pAnimSequence->CreateNode(animNodeType);
    if (!pNewAnimNode)
    {
        return nullptr;
    }

    pNewAnimNode->SetName(name.toUtf8().data());
    pNewAnimNode->CreateDefaultTracks();
    pNewAnimNode->SetParent(m_pAnimNode.get());

    CUiAnimViewAnimNodeFactory animNodeFactory;
    CUiAnimViewAnimNode* pNewNode = animNodeFactory.BuildAnimNode(m_pAnimSequence, pNewAnimNode, this);
    pNewNode->m_bExpanded = true;

    //AzEntity type nodes should have a valid pEntity
    if (animNodeType == eUiAnimNodeType_AzEntity)
    {
        AZ_Assert(pEntity, "AZ::Entity is nullptr.");
        if (pEntity)
        {
            pNewNode->SetNodeEntityAz(pEntity);
        }
    }
    pNewAnimNode->SetNodeOwner(pNewNode);

    if (listen)
    {
        pNewNode->BindToEditorObjects();
    }

    AddNode(pNewNode);
    UiAnimUndo::Record(new CUndoAnimNodeAdd(pNewNode));

    return pNewNode;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::RemoveSubNode(CUiAnimViewAnimNode* pSubNode)
{
    assert(UiAnimUndo::IsRecording());

    const bool bIsGroupNode = IsGroupNode();
    assert(bIsGroupNode);
    if (!bIsGroupNode)
    {
        return;
    }

    UiAnimUndo::Record(new CUndoAnimNodeRemove(pSubNode));
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrack* CUiAnimViewAnimNode::CreateTrack(const CUiAnimParamType& paramType)
{
    assert(UiAnimUndo::IsRecording());

    if (GetTrackForParameter(paramType) && !(GetParamFlags(paramType) & IUiAnimNode::eSupportedParamFlags_MultipleTracks))
    {
        return nullptr;
    }

    // Create UI Animation system and UiAnimView track
    IUiAnimTrack* pNewAnimTrack = m_pAnimNode->CreateTrack(paramType);
    if (!pNewAnimTrack)
    {
        return nullptr;
    }

    CUiAnimViewTrackFactory trackFactory;
    CUiAnimViewTrack* pNewTrack = trackFactory.BuildTrack(pNewAnimTrack, this, this);

    AddNode(pNewTrack);
    UiAnimUndo::Record(new CUndoTrackAdd(pNewTrack));

    return pNewTrack;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::RemoveTrack(CUiAnimViewTrack* pTrack)
{
    assert(UiAnimUndo::IsRecording());
    assert(!pTrack->IsSubTrack());

    if (!pTrack->IsSubTrack())
    {
        UiAnimUndo::Record(new CUndoTrackRemove(pTrack));
    }
}

CUiAnimViewTrack* CUiAnimViewAnimNode::CreateTrackAz(const UiAnimParamData& param)
{
    //    assert(UiAnimUndo::IsRecording());

    if (GetTrackForParameterAz(param))
    {
        return nullptr;
    }

    // Create UI Animation system and UiAnimView track

    // This will create sub-tracks if needed
    IUiAnimTrack* pNewAnimTrack = m_pAnimNode->CreateTrackForAzField(param);
    if (!pNewAnimTrack)
    {
        return nullptr;
    }

    CUiAnimViewTrackFactory trackFactory;
    CUiAnimViewTrack* pNewTrack = trackFactory.BuildTrack(pNewAnimTrack, this, this);

    AddNode(pNewTrack);
    UiAnimUndo::Record(new CUndoTrackAdd(pNewTrack));

    return pNewTrack;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::SnapTimeToPrevKey(float& time) const
{
    const float startTime = time;
    float closestTrackTime = std::numeric_limits<float>::min();
    bool bFoundPrevKey = false;

    for (size_t i = 0; i < m_childNodes.size(); ++i)
    {
        const CUiAnimViewNode* pNode = m_childNodes[i].get();

        float closestNodeTime = startTime;
        if (pNode->SnapTimeToPrevKey(closestNodeTime))
        {
            closestTrackTime = std::max(closestNodeTime, closestTrackTime);
            bFoundPrevKey = true;
        }
    }

    if (bFoundPrevKey)
    {
        time = closestTrackTime;
    }

    return bFoundPrevKey;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::SnapTimeToNextKey(float& time) const
{
    const float startTime = time;
    float closestTrackTime = std::numeric_limits<float>::max();
    bool bFoundNextKey = false;

    for (size_t i = 0; i < m_childNodes.size(); ++i)
    {
        const CUiAnimViewNode* pNode = m_childNodes[i].get();

        float closestNodeTime = startTime;
        if (pNode->SnapTimeToNextKey(closestNodeTime))
        {
            closestTrackTime = std::min(closestNodeTime, closestTrackTime);
            bFoundNextKey = true;
        }
    }

    if (bFoundNextKey)
    {
        time = closestTrackTime;
    }

    return bFoundNextKey;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyBundle CUiAnimViewAnimNode::GetSelectedKeys()
{
    CUiAnimViewKeyBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        bundle.AppendKeyBundle((*iter)->GetSelectedKeys());
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyBundle CUiAnimViewAnimNode::GetAllKeys()
{
    CUiAnimViewKeyBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        bundle.AppendKeyBundle((*iter)->GetAllKeys());
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyBundle CUiAnimViewAnimNode::GetKeysInTimeRange(const float t0, const float t1)
{
    CUiAnimViewKeyBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        bundle.AppendKeyBundle((*iter)->GetKeysInTimeRange(t0, t1));
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrackBundle CUiAnimViewAnimNode::GetAllTracks()
{
    return GetTracks(false, CUiAnimParamType());
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrackBundle CUiAnimViewAnimNode::GetSelectedTracks()
{
    return GetTracks(true, CUiAnimParamType());
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrackBundle CUiAnimViewAnimNode::GetTracksByParam(const CUiAnimParamType& paramType)
{
    return GetTracks(false, paramType);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrackBundle CUiAnimViewAnimNode::GetTracks(const bool bOnlySelected, const CUiAnimParamType& paramType)
{
    CUiAnimViewTrackBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pNode = (*iter).get();

        if (pNode->GetNodeType() == eUiAVNT_Track)
        {
            CUiAnimViewTrack* pTrack = static_cast<CUiAnimViewTrack*>(pNode);

            if (paramType != eUiAnimParamType_Invalid && pTrack->GetParameterType() != paramType)
            {
                continue;
            }

            if (!bOnlySelected || pTrack->IsSelected())
            {
                bundle.AppendTrack(pTrack);
            }

            const unsigned int subTrackCount = pTrack->GetChildCount();
            for (unsigned int subTrackIndex = 0; subTrackIndex < subTrackCount; ++subTrackIndex)
            {
                CUiAnimViewTrack* pSubTrack = static_cast<CUiAnimViewTrack*>(pTrack->GetChild(subTrackIndex));
                if (!bOnlySelected || pSubTrack->IsSelected())
                {
                    bundle.AppendTrack(pSubTrack);
                }
            }
        }
        else if (pNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pAnimNode = static_cast<CUiAnimViewAnimNode*>(pNode);
            bundle.AppendTrackBundle(pAnimNode->GetTracks(bOnlySelected, paramType));
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
EUiAnimNodeType CUiAnimViewAnimNode::GetType() const
{
    return m_pAnimNode ? m_pAnimNode->GetType() : eUiAnimNodeType_Invalid;
}

//////////////////////////////////////////////////////////////////////////
EUiAnimNodeFlags CUiAnimViewAnimNode::GetFlags() const
{
    return m_pAnimNode ? (EUiAnimNodeFlags)m_pAnimNode->GetFlags() : (EUiAnimNodeFlags)0;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::SetAsActiveDirector()
{
    if (GetType() == eUiAnimNodeType_Director)
    {
        m_pAnimSequence->SetActiveDirector(m_pAnimNode.get());

        GetSequence()->UnBindFromEditorObjects();
        GetSequence()->BindToEditorObjects();

        GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_SetAsActiveDirector);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::IsActiveDirector() const
{
    return m_pAnimNode.get() == m_pAnimSequence->GetActiveDirector();
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::IsParamValid(const CUiAnimParamType& param) const
{
    return m_pAnimNode ? m_pAnimNode->IsParamValid(param) : false;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrack* CUiAnimViewAnimNode::GetTrackForParameter(const CUiAnimParamType& paramType, uint32 index) const
{
    uint32 currentIndex = 0;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pNode = (*iter).get();

        if (pNode->GetNodeType() == eUiAVNT_Track)
        {
            CUiAnimViewTrack* pTrack = static_cast<CUiAnimViewTrack*>(pNode);

            if (pTrack->GetParameterType() == paramType)
            {
                if (currentIndex == index)
                {
                    return pTrack;
                }
                else
                {
                    ++currentIndex;
                }
            }

            if (pTrack->IsCompoundTrack())
            {
                unsigned int numChildTracks = pTrack->GetChildCount();
                for (unsigned int i = 0; i < numChildTracks; ++i)
                {
                    CUiAnimViewTrack* pChildTrack = static_cast<CUiAnimViewTrack*>(pTrack->GetChild(i));
                    if (pChildTrack->GetParameterType() == paramType)
                    {
                        if (currentIndex == index)
                        {
                            return pChildTrack;
                        }
                        else
                        {
                            ++currentIndex;
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrack* CUiAnimViewAnimNode::GetTrackForParameterAz(const UiAnimParamData& param) const
{
    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pNode = (*iter).get();

        if (pNode->GetNodeType() == eUiAVNT_Track)
        {
            CUiAnimViewTrack* pTrack = static_cast<CUiAnimViewTrack*>(pNode);

            if (pTrack->GetParameterType() == eUiAnimParamType_AzComponentField)
            {
                if (pTrack->GetParamData() == param)
                {
                    return pTrack;
                }
            }

            if (pTrack->IsCompoundTrack())
            {
                unsigned int numChildTracks = pTrack->GetChildCount();
                for (unsigned int i = 0; i < numChildTracks; ++i)
                {
                    CUiAnimViewTrack* pChildTrack = static_cast<CUiAnimViewTrack*>(pTrack->GetChild(i));
                    if (pChildTrack->GetParameterType() == eUiAnimParamType_AzComponentField)
                    {
                        if (pChildTrack->GetParameterType() == eUiAnimParamType_AzComponentField)
                        {
                            if (pChildTrack->GetParamData() == param)
                            {
                                return pChildTrack;
                            }
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::Render(const SUiAnimContext& ac)
{
    if (m_pNodeUiAnimator && IsActive())
    {
        m_pNodeUiAnimator->Render(this, ac);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = static_cast<CUiAnimViewAnimNode*>(pChildNode);
            pChildAnimNode->Render(ac);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::Animate(const SUiAnimContext& animContext)
{
    if (m_pNodeUiAnimator && IsActive())
    {
        m_pNodeUiAnimator->Animate(this, animContext);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = static_cast<CUiAnimViewAnimNode*>(pChildNode);
            pChildAnimNode->Animate(animContext);
        }
    }

    if (!s_isForcingAnimationBecausePropertyChanged)
    {
        GetIEditor()->Notify(eNotify_OnUpdateViewports);
    }

    // save a new cache of the current values of all the entities properties,
    // animating or manually moving the record head will change the component properties
    // so we need to update our cache so we can spot user edits to properties
    if (m_nodeEntityId.IsValid())
    {
        AZ::Entity* pNodeEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(pNodeEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_nodeEntityId);
        if (pNodeEntity)
        {
            AZ::IO::ByteContainerStream<AZStd::string> saveStream(&m_azEntityDataCache);
            [[maybe_unused]] bool success = AZ::Utils::SaveObjectToStream(saveStream, AZ::ObjectStream::ST_XML, pNodeEntity);
            AZ_Assert(success, "Failed to serialize canvas entity to XML");
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::SetName(const char* pName)
{
    // Check if the node's director already contains a node with this name
    CUiAnimViewAnimNode* pDirector = GetDirector();
    pDirector = pDirector ? pDirector : GetSequence();

    CUiAnimViewAnimNodeBundle nodes = pDirector->GetAnimNodesByName(pName);
    const uint numNodes = nodes.GetCount();
    for (uint i = 0; i < numNodes; ++i)
    {
        if (nodes.GetNode(i) != this)
        {
            return false;
        }
    }

    AZStd::string oldName = GetName();
    m_pAnimNode->SetName(pName);

    if (UiAnimUndo::IsRecording())
    {
        UiAnimUndo::Record(new CUndoAnimNodeRename(this, oldName));
    }

    GetSequence()->OnNodeRenamed(this, oldName.c_str());

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::CanBeRenamed() const
{
    return (GetFlags() & eUiAnimNodeFlags_CanChangeName) != 0;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::SetNodeEntityAz(AZ::Entity* pEntity)
{
    m_nodeEntityId = (pEntity) ? pEntity->GetId() : AZ::EntityId();

    if (m_pAnimNode)
    {
        m_pAnimNode->SetNodeOwner(this);
        UiAnimNodeBus::Event(m_pAnimNode.get(), &UiAnimNodeBus::Events::SetAzEntity, pEntity);
    }

    GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_NodeOwnerChanged);
}


//////////////////////////////////////////////////////////////////////////
AZ::Entity* CUiAnimViewAnimNode::GetNodeEntityAz([[maybe_unused]] const bool bSearch)
{
    if (m_pAnimNode)
    {
        if (m_nodeEntityId.IsValid())
        {
            AZ::Entity* pNodeEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(pNodeEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_nodeEntityId);
            return pNodeEntity;
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNodeBundle CUiAnimViewAnimNode::GetAllAnimNodes()
{
    CUiAnimViewAnimNodeBundle bundle;

    if (GetNodeType() == eUiAVNT_AnimNode)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = static_cast<CUiAnimViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAllAnimNodes());
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNodeBundle CUiAnimViewAnimNode::GetSelectedAnimNodes()
{
    CUiAnimViewAnimNodeBundle bundle;

    if ((GetNodeType() == eUiAVNT_AnimNode || GetNodeType() == eUiAVNT_Sequence) && IsSelected())
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = static_cast<CUiAnimViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetSelectedAnimNodes());
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNodeBundle CUiAnimViewAnimNode::GetAllOwnedNodes(const AZ::Entity* pOwner)
{
    CUiAnimViewAnimNodeBundle bundle;

    if (GetNodeType() == eUiAVNT_AnimNode && GetNodeEntityAz() == pOwner)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = static_cast<CUiAnimViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAllOwnedNodes(pOwner));
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNodeBundle CUiAnimViewAnimNode::GetAnimNodesByType(EUiAnimNodeType animNodeType)
{
    CUiAnimViewAnimNodeBundle bundle;

    if (GetNodeType() == eUiAVNT_AnimNode && GetType() == animNodeType)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = static_cast<CUiAnimViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAnimNodesByType(animNodeType));
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNodeBundle CUiAnimViewAnimNode::GetAnimNodesByName(const char* pName)
{
    CUiAnimViewAnimNodeBundle bundle;

    QString nodeName = QString::fromUtf8(GetName().c_str());
    if (GetNodeType() == eUiAVNT_AnimNode && QString::compare(pName, nodeName, Qt::CaseInsensitive) == 0)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = static_cast<CUiAnimViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAnimNodesByName(pName));
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CUiAnimViewAnimNode::GetParamName(const CUiAnimParamType& paramType) const
{
    return m_pAnimNode->GetParamName(paramType);
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CUiAnimViewAnimNode::GetParamNameForTrack(const CUiAnimParamType& paramType, const IUiAnimTrack* track) const
{
    return m_pAnimNode->GetParamNameForTrack(paramType, track);
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::IsGroupNode() const
{
    return GetType() == eUiAnimNodeType_Director || GetType() == eUiAnimNodeType_Group
           || GetType() == eUiAnimNodeType_AzEntity;
}

//////////////////////////////////////////////////////////////////////////
QString CUiAnimViewAnimNode::GetAvailableNodeNameStartingWith(const QString& name) const
{
    QString newName = name;
    unsigned int index = 2;

    while (const_cast<CUiAnimViewAnimNode*>(this)->GetAnimNodesByName(newName.toUtf8().data()).GetCount() > 0)
    {
        newName = QStringLiteral("%1%2").arg(name).arg(index);
        ++index;
    }

    return newName;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNodeBundle CUiAnimViewAnimNode::AddSelectedUiElements()
{
    assert(IsGroupNode());
    //assert(UiAnimUndo::IsRecording());

    CUiAnimViewAnimNodeBundle addedNodes;

    // Add selected nodes.
    // Need some way to communicate with the UiCanvasEditor here
    LyShine::EntityArray selectedElements;
    UiEditorDLLBus::BroadcastResult(selectedElements, &UiEditorDLLBus::Events::GetSelectedElements);

    for (AZ::Entity* entity : selectedElements)
    {
        // Check if object already assigned to some AnimNode.
        CUiAnimViewAnimNode* pExistingNode = CUiAnimViewSequenceManager::GetSequenceManager()->GetActiveAnimNode(entity);
        if (pExistingNode)
        {
            // If it has the same director than the current node, reject it
            // Actually for Az Entities this fails because of the component nodes
            //            if (pExistingNode->GetDirector() == GetDirector())
            {
                continue;
            }
        }

        // Get node type (either entity or camera)
        CUiAnimViewAnimNode* pAnimNode = nullptr;

        // Since entity names in canvases do not tend to be unique add the element ID
        LyShine::ElementId elementId = 0;
        UiElementBus::EventResult(elementId, entity->GetId(), &UiElementBus::Events::GetElementId);
        QString nodeName = entity->GetName().c_str();
        char suffix[10];
        azsnprintf(suffix, 10, " (%d)", elementId);
        nodeName += suffix;

        pAnimNode = CreateSubNode(nodeName, eUiAnimNodeType_AzEntity, entity, true);

        if (pAnimNode)
        {
            addedNodes.AppendAnimNode(pAnimNode);
        }
    }

    return addedNodes;
}

//////////////////////////////////////////////////////////////////////////
unsigned int CUiAnimViewAnimNode::GetParamCount() const
{
    return m_pAnimNode ? m_pAnimNode->GetParamCount() : 0;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimParamType CUiAnimViewAnimNode::GetParamType(unsigned int index) const
{
    unsigned int paramCount = GetParamCount();
    if (!m_pAnimNode || index >= paramCount)
    {
        return eUiAnimParamType_Invalid;
    }

    return m_pAnimNode->GetParamType(index);
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode::ESupportedParamFlags CUiAnimViewAnimNode::GetParamFlags(const CUiAnimParamType& paramType) const
{
    if (m_pAnimNode)
    {
        return m_pAnimNode->GetParamFlags(paramType);
    }

    return IUiAnimNode::ESupportedParamFlags(0);
}

//////////////////////////////////////////////////////////////////////////
EUiAnimValue CUiAnimViewAnimNode::GetParamValueType(const CUiAnimParamType& paramType) const
{
    if (m_pAnimNode)
    {
        return m_pAnimNode->GetParamValueType(paramType);
    }

    return eUiAnimValue_Unknown;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::UpdateDynamicParams()
{
    if (m_pAnimNode)
    {
        m_pAnimNode->UpdateDynamicParams();
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = static_cast<CUiAnimViewAnimNode*>(pChildNode);
            pChildAnimNode->UpdateDynamicParams();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks)
{
    XmlNodeRef childNode = xmlNode->createNode("Node");
    childNode->setAttr("name", GetName().c_str());
    childNode->setAttr("type", GetType());

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        pChildNode->CopyKeysToClipboard(childNode, bOnlySelectedKeys, bOnlyFromSelectedTracks);
    }

    if (childNode->getChildCount() > 0)
    {
        xmlNode->addChild(childNode);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::CopyNodesToClipboard(const bool bOnlySelected, QWidget* context)
{
    XmlNodeRef animNodesRoot = XmlHelpers::CreateXmlNode("CopyAnimNodesRoot");

    CopyNodesToClipboardRec(this, animNodesRoot, bOnlySelected);

    CClipboard clipboard(context);
    clipboard.Put(animNodesRoot, "Track view entity nodes");
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::CopyNodesToClipboardRec(CUiAnimViewAnimNode* pCurrentAnimNode, XmlNodeRef& xmlNode, const bool bOnlySelected)
{
    if (!pCurrentAnimNode->IsGroupNode() && (!bOnlySelected || pCurrentAnimNode->IsSelected()))
    {
        XmlNodeRef childXmlNode = xmlNode->newChild("Node");
        pCurrentAnimNode->m_pAnimNode->Serialize(childXmlNode, false, true);
    }

    for (auto iter = pCurrentAnimNode->m_childNodes.begin(); iter != pCurrentAnimNode->m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = static_cast<CUiAnimViewAnimNode*>(pChildNode);

            // If selected and group node, force copying of children
            const bool bSelectedAndGroupNode = pCurrentAnimNode->IsSelected() && pCurrentAnimNode->IsGroupNode();
            CopyNodesToClipboardRec(pChildAnimNode, xmlNode, !bSelectedAndGroupNode && bOnlySelected);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::PasteNodesFromClipboard(QWidget* context)
{
    assert(UiAnimUndo::IsRecording());

    CClipboard clipboard(context);
    if (clipboard.IsEmpty())
    {
        return false;
    }

    XmlNodeRef animNodesRoot = clipboard.Get();
    if (animNodesRoot == NULL || strcmp(animNodesRoot->getTag(), "CopyAnimNodesRoot") != 0)
    {
        return false;
    }

    const bool bLightAnimationSetActive = GetSequence()->GetFlags() & IUiAnimSequence::eSeqFlags_LightAnimationSet;

    const unsigned int numNodes = animNodesRoot->getChildCount();
    for (unsigned int i = 0; i < numNodes; ++i)
    {
        XmlNodeRef xmlNode = animNodesRoot->getChild(i);

        int type;
        if (!xmlNode->getAttr("Type", type))
        {
            continue;
        }

        if (bLightAnimationSetActive && (EUiAnimNodeType)type != eUiAnimNodeType_Light)
        {
            // Ignore non light nodes in light animation set
            continue;
        }

        PasteNodeFromClipboard(xmlNode);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::PasteNodeFromClipboard(XmlNodeRef xmlNode)
{
    QString name;

    if (!xmlNode->getAttr("Name", name))
    {
        return;
    }

    const bool bIsGroupNode = IsGroupNode();
    assert(bIsGroupNode);
    if (!bIsGroupNode)
    {
        return;
    }

    // Check if the node's director or sequence already contains a node with this name
    CUiAnimViewAnimNode* pDirector = GetDirector();
    pDirector = pDirector ? pDirector : GetSequence();
    if (pDirector->GetAnimNodesByName(name.toUtf8().data()).GetCount() > 0)
    {
        return;
    }

    // Create UI Animation system and UiAnimView node
    IUiAnimNode* pNewAnimNode = m_pAnimSequence->CreateNode(xmlNode);
    if (!pNewAnimNode)
    {
        return;
    }

    pNewAnimNode->SetParent(m_pAnimNode.get());

    CUiAnimViewAnimNodeFactory animNodeFactory;
    CUiAnimViewAnimNode* pNewNode = animNodeFactory.BuildAnimNode(m_pAnimSequence, pNewAnimNode, this);
    pNewNode->m_bExpanded = true;

    AddNode(pNewNode);
    UiAnimUndo::Record(new CUndoAnimNodeAdd(pNewNode));
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::IsValidReparentingTo(CUiAnimViewAnimNode* pNewParent)
{
    // UI_ANIMATION_REVISIT, do we want to support any reparenting?

    if (pNewParent == GetParentNode() || !pNewParent->IsGroupNode())
    {
        return false;
    }

    // Check if the new parent already contains a node with this name
    CUiAnimViewAnimNodeBundle foundNodes = pNewParent->GetAnimNodesByName(GetName().c_str());
    if (foundNodes.GetCount() > 1 || (foundNodes.GetCount() == 1 && foundNodes.GetNode(0) != this))
    {
        return false;
    }

    // Check if another node already owns this entity in the new parent's tree
    AZ::Entity* pOwner = GetNodeEntityAz();
    if (pOwner)
    {
        CUiAnimViewAnimNodeBundle ownedNodes = pNewParent->GetAllOwnedNodes(pOwner);
        if (ownedNodes.GetCount() > 0 && ownedNodes.GetNode(0) != this)
        {
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::SetNewParent(CUiAnimViewAnimNode* pNewParent)
{
    if (pNewParent == GetParentNode())
    {
        return;
    }

    assert(UiAnimUndo::IsRecording());
    assert(IsValidReparentingTo(pNewParent));

    UiAnimUndo::Record(new CUndoAnimNodeReparent(this, pNewParent));
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::SetDisabled(bool bDisabled)
{
    if (m_pAnimNode)
    {
        if (bDisabled)
        {
            m_pAnimNode->SetFlags(m_pAnimNode->GetFlags() | eUiAnimNodeFlags_Disabled);
            GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Disabled);
        }
        else
        {
            m_pAnimNode->SetFlags(m_pAnimNode->GetFlags() & ~eUiAnimNodeFlags_Disabled);
            GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Enabled);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::IsDisabled() const
{
    return m_pAnimNode ? m_pAnimNode->GetFlags() & eUiAnimNodeFlags_Disabled : false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::IsActive()
{
    CUiAnimViewSequence* pSequence = GetSequence();
    const bool bInActiveSequence = pSequence ? GetSequence()->IsBoundToEditorObjects() : false;

    CUiAnimViewAnimNode* pDirector = GetDirector();
    const bool bMemberOfActiveDirector = pDirector ? GetDirector()->IsActiveDirector() : true;

    return bInActiveSequence && bMemberOfActiveDirector;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::OnSelectionChanged(const bool bSelected)
{
    if (m_pAnimNode)
    {
        [[maybe_unused]] const EUiAnimNodeType animNodeType = GetType();
        assert(animNodeType == eUiAnimNodeType_Camera || animNodeType == eUiAnimNodeType_Entity || animNodeType == eUiAnimNodeType_GeomCache);

        const EUiAnimNodeFlags flags = (EUiAnimNodeFlags)m_pAnimNode->GetFlags();
        m_pAnimNode->SetFlags(bSelected ? (flags | eUiAnimNodeFlags_EntitySelected) : (flags & ~eUiAnimNodeFlags_EntitySelected));
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewAnimNode::CheckTrackAnimated(const CUiAnimParamType& paramType) const
{
    if (!m_pAnimNode)
    {
        return false;
    }

    CUiAnimViewTrack* pTrack = GetTrackForParameter(paramType);
    return pTrack && pTrack->GetKeyCount() > 0;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::OnNodeUiAnimated([[maybe_unused]] IUiAnimNode* pNode)
{
    // UI ANIMATION_REVISIT - is this function needed?
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::OnNodeVisibilityChanged([[maybe_unused]] IUiAnimNode* pNode, [[maybe_unused]] const bool bHidden)
{
    // UI ANIMATION_REVISIT - is this function needed?
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::OnNodeReset([[maybe_unused]] IUiAnimNode* pNode)
{
    // UI ANIMATION_REVISIT - is this function needed?
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewAnimNode::OnDone()
{
    SetNodeEntityAz(nullptr);
}
