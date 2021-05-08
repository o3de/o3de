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

#include <AtomLyIntegration/AtomFont/AtomFont_precompiled.h>
#include "AtomFontSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/Components/ConsoleBus.h>
#include <ISystem.h>

#include <AtomLyIntegration/AtomFont/AtomNullFont.h>
#include <AtomLyIntegration/AtomFont/AtomFont.h>

namespace AZ
{
    namespace Render
    {
        void AtomFontSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<AtomFontSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<AtomFontSystemComponent>("Font", "Manages lifetime of the font subsystem")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void AtomFontSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AtomFontService"));
        }

        void AtomFontSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AtomFontService"));
        }

        void AtomFontSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        void AtomFontSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
        }

        void AtomFontSystemComponent::Activate()
        {
            CrySystemEventBus::Handler::BusConnect();
        }

        void AtomFontSystemComponent::Deactivate()
        {
            CrySystemEventBus::Handler::BusDisconnect();
        }

        void LoadFont(ICryFont& cryFont, const AZStd::string& fontName)
        {
            IFFont* font = cryFont.NewFont(fontName.c_str());
            AZ_Assert(font, "Could not instantiate font: %s", fontName.c_str());

            const AZStd::string fontPath = "Fonts/" + fontName + ".font";
            if (!font->Load(fontPath.c_str()))
            {
                AZ_Error("AtomFont", false, "Could not load font: %s", fontPath.c_str());
            }
        }

        void AtomFontSystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
        {
#if !defined(AZ_MONOLITHIC_BUILD)
            // When module is linked dynamically, we must set our gEnv pointer.
            // When module is linked statically, we'll share the application's gEnv pointer.
            gEnv = system.GetGlobalEnvironment();
#endif

            if (gEnv->IsDedicated())
            {
        #if defined(USE_NULLFONT)
                gEnv->pCryFont = new AtomNullFont();
        #else
                // The NULL font implementation must be present for all platforms
                // supporting running as a pure dedicated server.
                system->GetILog()->LogError("Missing NULL font implementation for dedicated server");
                env.pCryFont = NULL;
        #endif
            }
            else
            {
        #if defined(USE_NULLFONT) && defined(USE_NULLFONT_ALWAYS)
                gEnv->pCryFont = new AtomNullFont();
        #else
                gEnv->pCryFont = new AtomFont(&system);
        #endif
            }

            if (gEnv->pCryFont)
            {
                LoadFont(*gEnv->pCryFont, "default");
                LoadFont(*gEnv->pCryFont, "default-ui");
            }
        }

        void AtomFontSystemComponent::OnCrySystemShutdown([[maybe_unused]] ISystem& system)
        {
#if !defined(AZ_MONOLITHIC_BUILD)
            gEnv = nullptr;
#endif
        }

    } // namespace Render
} // namespace AZ
