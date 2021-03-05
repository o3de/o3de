/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include "Util/smartptr.h"

#include "UiAnimViewSequence.h"
#include <LyShine/Animation/IUiAnimation.h>
#include "UiEditorAnimationBus.h"
#include "UiAnimUndoManager.h"

#include <IEditor.h>

struct IUiAnimViewSequenceManagerListener
{
    virtual void OnSequenceAdded([[maybe_unused]] CUiAnimViewSequence* pSequence) {}
    virtual void OnSequenceRemoved([[maybe_unused]] CUiAnimViewSequence* pSequence) {}
};

class CUiAnimViewSequenceManager
    : public IEditorNotifyListener
    , public UiEditorAnimationBus::Handler
{
    friend class CAbstractUndoSequenceTransaction;

public:
    CUiAnimViewSequenceManager();
    ~CUiAnimViewSequenceManager();

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    unsigned int GetCount() const { return m_sequences.size(); }

    void CreateSequence(QString name);
    void DeleteSequence(CUiAnimViewSequence* pSequence);

    CUiAnimViewSequence* GetSequenceByName(QString name) const;
    CUiAnimViewSequence* GetSequenceByIndex(unsigned int index) const;
    CUiAnimViewSequence* GetSequenceByAnimSequence(IUiAnimSequence* pAnimSequence) const;

    CUiAnimViewAnimNodeBundle GetAllRelatedAnimNodes(const AZ::Entity* pEntityObject) const;
    CUiAnimViewAnimNode* GetActiveAnimNode(const AZ::Entity* pEntityObject) const;

    void AddListener(IUiAnimViewSequenceManagerListener* pListener) { stl::push_back_unique(m_listeners, pListener); }
    void RemoveListener(IUiAnimViewSequenceManagerListener* pListener) { stl::find_and_erase(m_listeners, pListener); }

    // UI_ANIMATION_REVISIT, made this a singleton for now
    static CUiAnimViewSequenceManager* GetSequenceManager();
    static void Create();
    static void Destroy();

    // UiEditorAnimationInterface
    CUiAnimationContext* GetAnimationContext() override;
    IUiAnimationSystem* GetAnimationSystem() override;
    CUiAnimViewSequence* GetCurrentSequence() override;
    void ActiveCanvasChanged() override;
    // ~UiEditorAnimationInterface

private:

    void DeleteAllSequences();
    void CreateSequencesFromAnimationSystem();

    void SortSequences();

    void OnSequenceAdded(CUiAnimViewSequence* pSequence);
    void OnSequenceRemoved(CUiAnimViewSequence* pSequence);

    std::vector<IUiAnimViewSequenceManagerListener*> m_listeners;
    std::vector<std::unique_ptr<CUiAnimViewSequence> > m_sequences;

    uint32 m_nextSequenceId;

    // Used to handle object attach/detach
    std::unordered_map<CUiAnimViewNode*, Matrix34> m_prevTransforms;

    static CUiAnimViewSequenceManager* s_instance;
    IUiAnimationSystem* m_animationSystem;
    CUiAnimationContext* m_animationContext;

    UiAnimUndoManager m_undoManager;
};

