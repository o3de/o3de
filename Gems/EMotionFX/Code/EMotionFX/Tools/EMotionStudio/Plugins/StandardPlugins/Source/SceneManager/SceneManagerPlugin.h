/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <AzCore/std/containers/vector.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include "../../../../EMStudioSDK/Source/SaveChangedFilesManager.h"
#include "ActorPropertiesWindow.h"
#include "ActorsWindow.h"

#include <EMotionFX/Source/EventHandler.h>
#endif

QT_FORWARD_DECLARE_CLASS(QPushButton)


namespace EMStudio
{
    class SceneManagerPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(SceneManagerPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000037
        };

        SceneManagerPlugin();
        ~SceneManagerPlugin();

        // overloaded
        const char* GetName() const override                { return "Actor Manager"; }
        uint32 GetClassID() const override                  { return CLASS_ID; }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() const override { return new SceneManagerPlugin(); }
        void UpdateInterface();
        void ReInit();

    public slots:
        void WindowReInit(bool visible);

    private:
        MCORE_DEFINECOMMANDCALLBACK(ImportActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(CreateActorInstanceCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(RemoveActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(RemoveActorInstanceCallback);
        MCORE_DEFINECOMMANDCALLBACK(SaveActorAssetInfoCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandActorSetCollisionMeshesCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustActorInstanceCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandScaleActorDataCallback);

        ImportActorCallback*                    m_importActorCallback;
        CreateActorInstanceCallback*            m_createActorInstanceCallback;
        CommandSelectCallback*                  m_selectCallback;
        CommandUnselectCallback*                m_unselectCallback;
        CommandClearSelectionCallback*          m_clearSelectionCallback;
        RemoveActorCallback*                    m_removeActorCallback;
        RemoveActorInstanceCallback*            m_removeActorInstanceCallback;
        SaveActorAssetInfoCallback*             m_saveActorAssetInfoCallback;
        CommandAdjustActorCallback*             m_adjustActorCallback;
        CommandActorSetCollisionMeshesCallback* m_actorSetCollisionMeshesCallback;
        CommandAdjustActorInstanceCallback*     m_adjustActorInstanceCallback;
        CommandScaleActorDataCallback*          m_scaleActorDataCallback;
        ActorsWindow*                           m_actorsWindow;
        ActorPropertiesWindow*                  m_actorPropsWindow;
    };
} // namespace EMStudio
