/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <ScriptEvents/Internal/BehaviorContextBinding/ScriptEventBinding.h>
#include "ScriptEventsAsset.h"

namespace ScriptEvents
{
    namespace Internal
    {
        //! This is the internal object that represents a ScriptEvent.
        //! It provides the binding between the BehaviorContext and the messaging functionality.
        //! It is ref counted so that it remains alive as long as anything is referencing it, this can happen
        //! when multiple scripts or script canvas graphs are sending or receiving events defined in a given script event.
        class ScriptEventRegistration
            : public AZ::Data::AssetBus::Handler
            , public AZ::SystemTickBus::Handler
        {
        public:
            AZ_RTTI(ScriptEventRegistration, "{B8801400-65CD-49D5-B797-58E56D705A0A}");
            AZ_CLASS_ALLOCATOR(ScriptEventRegistration, AZ::SystemAllocator);

            AZ::AttributeArray* m_currentAttributes;

            ScriptEventRegistration() = default;
            virtual ~ScriptEventRegistration();

            ScriptEventRegistration(AZ::Data::AssetId scriptEventAssetId)
            {
                Init(scriptEventAssetId);
            }

            void Init(AZ::Data::AssetId scriptEventAssetId);

            bool GetMethod(AZStd::string_view eventName, AZ::BehaviorMethod*& outMethod);

            AZ::BehaviorEBus* GetBehaviorBus(AZ::u32 version = (std::numeric_limits<AZ::u32>::max)())
            {
                if (version == (std::numeric_limits<AZ::u32>::max)())
                {
                    return m_behaviorEBus[m_maxVersion];
                }

                return m_behaviorEBus[version];
            }

            void CompleteRegistration(AZ::Data::Asset<AZ::Data::AssetData> asset);

            AZStd::string GetBusName() const
            {
                return m_busName;
            }

            bool IsReady() const { return m_isReady; }

        protected:

            void OnSystemTick() override;

        private:

            AZ::u32 m_maxVersion = 0;
            AZ::Data::AssetId m_assetId;

            AZStd::string m_busName;
            AZStd::unordered_map<AZ::u32, AZ::BehaviorEBus*> m_behaviorEBus; // version, ebus

            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> m_asset;

            AZStd::unordered_map<AZ::Data::AssetId, AZStd::unique_ptr<ScriptEventBinding>> m_scriptEventBindings;

            bool m_isReady = false;

            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            //


            // Reference count for intrusive_ptr
            template<class T>
            friend struct AZStd::IntrusivePtrCountPolicy;
            mutable unsigned int m_refCount = 0;
            AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
            AZ_FORCE_INLINE void release()
            {
                AZ_Assert(m_refCount > 0, "Reference count logic error, trying to release reference when there are none left.");
                if (--m_refCount == 0)
                {
                    AZ::Data::AssetBus::Handler::BusDisconnect();
                    delete this;
                }
            }

            AZStd::string m_previousName;
            int m_previousVersion;
        };

        class OnScopeEnd
        {
        public:
            using ScopeEndFunctor = std::function<void()>;

        private:
            ScopeEndFunctor m_functor;

        public:
            OnScopeEnd(ScopeEndFunctor&& functor)
                : m_functor(AZStd::move(functor))
            {}

            OnScopeEnd(const ScopeEndFunctor& functor)
                : m_functor(functor)
            {}

            ~OnScopeEnd()
            {
                m_functor();
            }
        };
    }
}
