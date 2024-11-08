/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "UiAnimViewNode.h"
#include "UiAnimViewTrack.h"
#include <LyShine/Bus/UiElementBus.h>
#include <AzCore/Serialization/SerializeContext.h>

class CUiAnimViewAnimNode;
class QWidget;

namespace AZ
{
    class Entity;
}

// Represents a bundle of anim nodes
class CUiAnimViewAnimNodeBundle
{
public:
    unsigned int GetCount() const { return static_cast<unsigned int>(m_animNodes.size()); }
    CUiAnimViewAnimNode* GetNode(const unsigned int index) { return m_animNodes[index]; }
    const CUiAnimViewAnimNode* GetNode(const unsigned int index) const { return m_animNodes[index]; }

    void Clear();
    const bool DoesContain(const CUiAnimViewNode* pTargetNode);

    void AppendAnimNode(CUiAnimViewAnimNode* pNode);
    void AppendAnimNodeBundle(const CUiAnimViewAnimNodeBundle& bundle);

    void ExpandAll(bool bAlsoExpandParentNodes = true);
    void CollapseAll();

private:
    std::vector<CUiAnimViewAnimNode*> m_animNodes;
};

// Callback called by animation node when its animated.
class IUiAnimNodeUiAnimator
{
public:
    virtual ~IUiAnimNodeUiAnimator() {}

    virtual void Animate(CUiAnimViewAnimNode* pNode, const SUiAnimContext& ac) = 0;
    virtual void Render([[maybe_unused]] CUiAnimViewAnimNode* pNode, [[maybe_unused]] const SUiAnimContext& ac) {}

    // Called when binding/unbinding the owning node
    virtual void Bind([[maybe_unused]] CUiAnimViewAnimNode* pNode) {}
    virtual void UnBind([[maybe_unused]] CUiAnimViewAnimNode* pNode) {}
};

////////////////////////////////////////////////////////////////////////////
//
// This class represents a IUiAnimNode in UiAnimView and contains
// the editor side code for changing it
//
// It does *not* have ownership of the IUiAnimNode, therefore deleting it
// will not destroy the UI Animation system track
//
////////////////////////////////////////////////////////////////////////////
class CUiAnimViewAnimNode
    : public CUiAnimViewNode
    , public IUiAnimNodeOwner
    , public UiElementChangeNotificationBus::Handler
{
    friend class CAbstractUndoAnimNodeTransaction;
    friend class CAbstractUndoTrackTransaction;
    friend class CUndoAnimNodeReparent;

public:
    CUiAnimViewAnimNode(IUiAnimSequence* pSequence, IUiAnimNode* pAnimNode, CUiAnimViewNode* pParentNode);

    // UiElementChangeNotification
    void UiElementPropertyChanged() override;
    // ~UiElementChangeNotification

    // Rendering
    virtual void Render(const SUiAnimContext& ac);

    // Playback
    virtual void Animate(const SUiAnimContext& animContext);

    // Binding/Unbinding
    virtual void BindToEditorObjects();
    virtual void UnBindFromEditorObjects();
    virtual bool IsBoundToEditorObjects() const;

    // CUiAnimViewAnimNode
    virtual EUiAnimViewNodeType GetNodeType() const override { return eUiAVNT_AnimNode; }

    // Create & remove sub anim nodes
    virtual CUiAnimViewAnimNode* CreateSubNode(const QString& name, const EUiAnimNodeType animNodeType, AZ::Entity* pEntity = nullptr, bool listen = false);
    virtual void RemoveSubNode(CUiAnimViewAnimNode* pSubNode);

    // Create & remove sub tracks
    virtual CUiAnimViewTrack* CreateTrack(const CUiAnimParamType& paramType);
    virtual void RemoveTrack(CUiAnimViewTrack* pTrack);

    virtual CUiAnimViewTrack* CreateTrackAz(const UiAnimParamData& param);

    // Add selected UI elements from current canvas to group node
    virtual CUiAnimViewAnimNodeBundle AddSelectedUiElements();

    // Director related
    virtual void SetAsActiveDirector();
    virtual bool IsActiveDirector() const;

    // Checks if anim node is part of active sequence and of an active director
    virtual bool IsActive();

    // Name setter/getter
    AZStd::string GetName() const override { return m_pAnimNode->GetName(); }
    virtual bool SetName(const char* pName) override;
    virtual bool CanBeRenamed() const override;

    // Node owner setter/getter
    virtual void SetNodeEntityAz(AZ::Entity* pEntity);
    virtual AZ::Entity* GetNodeEntityAz(const bool bSearch = true);

    // Snap time value to prev/next key in sequence
    virtual bool SnapTimeToPrevKey(float& time) const override;
    virtual bool SnapTimeToNextKey(float& time) const override;

    // Node getters
    CUiAnimViewAnimNodeBundle GetAllAnimNodes();
    CUiAnimViewAnimNodeBundle GetSelectedAnimNodes();
    CUiAnimViewAnimNodeBundle GetAllOwnedNodes(const AZ::Entity* pOwner);
    CUiAnimViewAnimNodeBundle GetAnimNodesByType(EUiAnimNodeType animNodeType);
    CUiAnimViewAnimNodeBundle GetAnimNodesByName(const char* pName);

    // Track getters
    virtual CUiAnimViewTrackBundle GetAllTracks();
    virtual CUiAnimViewTrackBundle GetSelectedTracks();
    virtual CUiAnimViewTrackBundle GetTracksByParam(const CUiAnimParamType& paramType);

    // Key getters
    virtual CUiAnimViewKeyBundle GetAllKeys() override;
    virtual CUiAnimViewKeyBundle GetSelectedKeys() override;
    virtual CUiAnimViewKeyBundle GetKeysInTimeRange(const float t0, const float t1) override;

    // Type getters
    EUiAnimNodeType GetType() const;

    // Flags
    EUiAnimNodeFlags GetFlags() const;

    // Disabled state
    virtual void SetDisabled(bool bDisabled) override;
    virtual bool IsDisabled() const override;

    // Return track assigned to the specified parameter.
    CUiAnimViewTrack* GetTrackForParameter(const CUiAnimParamType& paramType, uint32 index = 0) const;
    CUiAnimViewTrack* GetTrackForParameterAz(const UiAnimParamData& param) const;

    // Param
    unsigned int GetParamCount() const;
    CUiAnimParamType GetParamType(unsigned int index) const;
    AZStd::string GetParamName(const CUiAnimParamType& paramType) const;
    AZStd::string GetParamNameForTrack(const CUiAnimParamType& paramType, const IUiAnimTrack* track) const;
    bool IsParamValid(const CUiAnimParamType& param) const;
    IUiAnimNode::ESupportedParamFlags GetParamFlags(const CUiAnimParamType& paramType) const;
    EUiAnimValue GetParamValueType(const CUiAnimParamType& paramType) const;
    void UpdateDynamicParams();

    // Parameter getters/setters
    template <class Type>
    bool SetParamValue(const float time, const CUiAnimParamType& param, const Type& value)
    {
        assert(m_pAnimNode);
        return m_pAnimNode->SetParamValue(time, param, value);
    }

    template <class Type>
    bool GetParamValue(const float time, const CUiAnimParamType& param, Type& value)
    {
        assert(m_pAnimNode);
        return m_pAnimNode->GetParamValue(time, param, value);
    }

    // Check if it's a group node
    virtual bool IsGroupNode() const override;

    // Generate a new node name
    virtual QString GetAvailableNodeNameStartingWith(const QString& name) const;

    // Copy/Paste nodes
    virtual void CopyNodesToClipboard(const bool bOnlySelected, QWidget* context);
    virtual bool PasteNodesFromClipboard(QWidget* context);

    // Set new parent
    virtual void SetNewParent(CUiAnimViewAnimNode* pNewParent);

    // Check if this node may be moved to new parent
    virtual bool IsValidReparentingTo(CUiAnimViewAnimNode* pNewParent);

protected:
    // IUiAnimNodeOwner
    virtual void OnNodeUiAnimated(IUiAnimNode* pNode) override;
    // ~IUiAnimNodeOwner

    IUiAnimNode* GetAnimNode() { return m_pAnimNode.get(); }

private:
    // Copy selected keys to XML representation for clipboard
    virtual void CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) override;

    void CopyNodesToClipboardRec(CUiAnimViewAnimNode* pCurrentAnimNode, XmlNodeRef& xmlNode, const bool bOnlySelected);

    bool HasObsoleteTrackRec(CUiAnimViewNode* pCurrentNode) const;
    CUiAnimViewTrackBundle GetTracks(const bool bOnlySelected, const CUiAnimParamType& paramType);

    void PasteNodeFromClipboard(XmlNodeRef xmlNode);

    bool CheckTrackAnimated(const CUiAnimParamType& paramType) const;

    // Az Entity specific
    void AzEntityPropertyChanged(AZ::Component* oldComponent, AZ::Component* newComponent,
        const AZ::SerializeContext::ClassElement& element, size_t offset);
    void AzCreateCompoundTrackIfNeeded(float time, AZ::Component* newComponent, AZ::Component* oldComponent,
        const AZ::SerializeContext::ClassElement& element, size_t offset);

    void SetComponentParamValueAz(float time,
        AZ::Component* dstComponent, AZ::Component* srcComponent,
        const AZ::SerializeContext::ClassElement& element, size_t offset);

    bool HasComponentParamValueAzChanged(
        AZ::Component* dstComponent, AZ::Component* srcComponent,
        const AZ::SerializeContext::ClassElement& element, size_t offset);

    bool BaseClassPropertyPotentiallyChanged(
        AZ::SerializeContext* context,
        AZ::Component* dstComponent, AZ::Component* srcComponent,
        const AZ::SerializeContext::ClassElement& element, size_t offset);

    // IUiAnimNodeOwner
    virtual void OnNodeVisibilityChanged(IUiAnimNode* pNode, const bool bHidden) override;
    virtual void OnNodeReset(IUiAnimNode* pNode) override;
    // ~IUiAnimNodeOwner

    // IEntityObjectListener
    virtual void OnNameChanged([[maybe_unused]] const char* pName)  {}
    virtual void OnSelectionChanged(const bool bSelected);
    virtual void OnDone();
    // ~IEntityObjectListener

    IUiAnimSequence* m_pAnimSequence;
    AZStd::intrusive_ptr<IUiAnimNode> m_pAnimNode;
    std::unique_ptr<IUiAnimNodeUiAnimator> m_pNodeUiAnimator;

    AZ::EntityId m_nodeEntityId;
    AZStd::string m_azEntityDataCache;

    static bool s_isForcingAnimationBecausePropertyChanged;
};
