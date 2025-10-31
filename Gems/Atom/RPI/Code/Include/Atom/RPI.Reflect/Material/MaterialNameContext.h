/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
    class Name;

    namespace RPI
    {
        //! This acts like a namespace description for various types of identifiers that appear in .materialtype files.
        //! When reusable property groups are nested inside other property groups, they usually need alternate naming
        //! to connect to the appropriate shader inputs. For example, a baseColor property group inside a "layer1" group
        //! needs to connect to "m_layer1_baseColor_texture" and the same property definition is repeated inside a "layer2"
        //! group where it connects to "m_layer2_baseColor_texture". This data structure provides the name context, like
        //! "m_layer1_" or "m_layer2_".
        class ATOM_RPI_REFLECT_API MaterialNameContext
        {
        public:
            AZ_TYPE_INFO(MaterialNameContext, "{AAC9BB28-F463-455D-8467-F877E50E1FA7}")
                
            static void Reflect(ReflectContext* context);

            MaterialNameContext() = default;

            //! Extends the name context to a deeper property group.
            void ExtendPropertyIdContext(AZStd::string_view nameContext, bool insertDelimiter=true);
            void ExtendSrgInputContext(AZStd::string_view nameContext);
            void ExtendShaderOptionContext(AZStd::string_view nameContext);
            
            //! Applies the name context to a given leaf name.
            bool ContextualizeProperty(Name& propertyName) const;
            bool ContextualizeSrgInput(Name& srgInputName) const;
            bool ContextualizeShaderOption(Name& shaderOptionName) const;
            bool ContextualizeProperty(AZStd::string& propertyName) const;
            bool ContextualizeSrgInput(AZStd::string& srgInputName) const;
            bool ContextualizeShaderOption(AZStd::string& shaderOptionName) const;

            Name GetContextualizedProperty(Name& propertyName) const;
            Name GetContextualizedSrgInput(Name& srgInputName) const;
            Name GetContextualizedShaderOption(Name& shaderOptionName) const;
            AZStd::string GetContextualizedProperty(const AZStd::string& propertyName) const;
            AZStd::string GetContextualizedSrgInput(const AZStd::string& srgInputName) const;
            AZStd::string GetContextualizedShaderOption(const AZStd::string& shaderOptionName) const;

            //! Returns true if there is some non-default name context.
            bool HasContextForProperties() const;
            bool HasContextForSrgInputs() const;
            bool HasContextForShaderOptions() const;

            //! Returns true if the name context is empty.
            bool IsDefault() const;

        private:
            AZStd::string m_propertyIdContext;
            AZStd::string m_srgInputNameContext;
            AZStd::string m_shaderOptionNameContext;
        };

    } // namespace RPI
} // namespace AZ
