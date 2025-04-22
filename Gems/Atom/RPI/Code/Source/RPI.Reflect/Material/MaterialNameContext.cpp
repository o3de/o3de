/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialNameContext::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialNameContext>()
                    ->Version(1) 
                    ->Field("propertyIdContext", &MaterialNameContext::m_propertyIdContext)
                    ->Field("srgInputNameContext", &MaterialNameContext::m_srgInputNameContext)
                    ->Field("shaderOptionNameContext", &MaterialNameContext::m_shaderOptionNameContext)
                    ;
            }
        }
        
        bool MaterialNameContext::IsDefault() const
        {
            return m_propertyIdContext.empty() && m_srgInputNameContext.empty() && m_shaderOptionNameContext.empty();
        }
        
        void MaterialNameContext::ExtendPropertyIdContext(AZStd::string_view nameContext, bool insertDelimiter)
        {
            m_propertyIdContext += nameContext;
            if (insertDelimiter && !nameContext.empty() && !nameContext.ends_with("."))
            {
                m_propertyIdContext += ".";
            }
        }

        void MaterialNameContext::ExtendSrgInputContext(AZStd::string_view nameContext)
        {
            m_srgInputNameContext += nameContext;
        }

        void MaterialNameContext::ExtendShaderOptionContext(AZStd::string_view nameContext)
        {
            m_shaderOptionNameContext += nameContext;
        }

        bool MaterialNameContext::ContextualizeProperty(Name& propertyName) const
        {
            if (m_propertyIdContext.empty())
            {
                return false;
            }

            propertyName = m_propertyIdContext + propertyName.GetCStr();
            return true;
        }

        bool MaterialNameContext::ContextualizeSrgInput(Name& srgInputName) const
        {
            if (m_srgInputNameContext.empty())
            {
                return false;
            }

            srgInputName = m_srgInputNameContext + srgInputName.GetCStr();
            return true;
        }

        bool MaterialNameContext::ContextualizeShaderOption(Name& shaderOptionName) const
        {
            if (m_shaderOptionNameContext.empty())
            {
                return false;
            }

            shaderOptionName = m_shaderOptionNameContext + shaderOptionName.GetCStr();
            return true;
        }
        
        bool MaterialNameContext::ContextualizeProperty(AZStd::string& propertyName) const
        {
            if (m_propertyIdContext.empty())
            {
                return false;
            }

            propertyName = m_propertyIdContext + propertyName;
            return true;
        }

        bool MaterialNameContext::ContextualizeSrgInput(AZStd::string& srgInputName) const
        {
            if (m_srgInputNameContext.empty())
            {
                return false;
            }

            srgInputName = m_srgInputNameContext + srgInputName;
            return true;
        }

        bool MaterialNameContext::ContextualizeShaderOption(AZStd::string& shaderOptionName) const
        {
            if (m_shaderOptionNameContext.empty())
            {
                return false;
            }

            shaderOptionName = m_shaderOptionNameContext + shaderOptionName;
            return true;
        }

        Name MaterialNameContext::GetContextualizedProperty(Name& propertyName) const
        {
            Name contextualizedName = propertyName;
            ContextualizeProperty(contextualizedName);
            return contextualizedName;
        }

        Name MaterialNameContext::GetContextualizedSrgInput(Name& srgInputName) const
        {
            Name contextualizedName = srgInputName;
            ContextualizeSrgInput(contextualizedName);
            return contextualizedName;
        }

        Name MaterialNameContext::GetContextualizedShaderOption(Name& shaderOptionName) const
        {
            Name contextualizedName = shaderOptionName;
            ContextualizeShaderOption(contextualizedName);
            return contextualizedName;
        }

        AZStd::string MaterialNameContext::GetContextualizedProperty(const AZStd::string& propertyName) const
        {
            AZStd::string contextualizedName = propertyName;
            ContextualizeProperty(contextualizedName);
            return contextualizedName;
        }

        AZStd::string MaterialNameContext::GetContextualizedSrgInput(const AZStd::string& srgInputName) const
        {
            AZStd::string contextualizedName = srgInputName;
            ContextualizeSrgInput(contextualizedName);
            return contextualizedName;
        }

        AZStd::string MaterialNameContext::GetContextualizedShaderOption(const AZStd::string& shaderOptionName) const
        {
            AZStd::string contextualizedName = shaderOptionName;
            ContextualizeShaderOption(contextualizedName);
            return contextualizedName;
        }

        bool MaterialNameContext::HasContextForProperties() const
        {
            return !m_propertyIdContext.empty();
        }

        bool MaterialNameContext::HasContextForSrgInputs() const
        {
            return !m_srgInputNameContext.empty();
        }

        bool MaterialNameContext::HasContextForShaderOptions() const
        {
            return !m_shaderOptionNameContext.empty();
        }
    } // namespace RPI
} // namespace AZ
