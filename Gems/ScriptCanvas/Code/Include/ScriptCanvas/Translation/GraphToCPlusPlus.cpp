/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GraphToCPlusPlus.h"

#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Grammar/Primitives.h>

namespace ScriptCanvas
{
    namespace Translation
    {
        Configuration CreateCPlusPluseConfig()
        {
            Configuration configuration;
            configuration.m_blockCommentClose = "*/";
            configuration.m_blockCommentOpen = "/*";
            configuration.m_namespaceClose = "}";
            configuration.m_namespaceOpen = "{";
            configuration.m_namespaceOpenPrefix = "namespace";
            configuration.m_scopeClose = "}";
            configuration.m_scopeOpen = "{";
            configuration.m_singleLineComment = "//";
            return configuration;
        }

        GraphToCPlusPlus::GraphToCPlusPlus(const Grammar::AbstractCodeModel& model)
            : GraphToX(CreateCPlusPluseConfig(), model) 
        {
            WriteHeaderDotH();
            WriteHeaderDotCPP();
            
            TranslateDependenciesDotH();
            TranslateDependenciesDotCPP();
                        
            TranslateNamespaceOpen();
            {
                TranslateClassOpen();
                {
                    TranslateVariables();
                    TranslateHandlers();
                    TranslateConstruction();
                    TranslateDestruction();
                    TranslateStartNode();
                }
                TranslateClassClose();
            }
            TranslateNamespaceClose();
        }

        AZ::Outcome<void, AZStd::pair<AZStd::string, AZStd::string>> GraphToCPlusPlus::Translate(const Grammar::AbstractCodeModel& model, AZStd::string& dotH, AZStd::string& dotCPP)
        {
            GraphToCPlusPlus translation(model);

            if (translation.IsSuccessfull())
            {
                dotH = AZStd::move(translation.m_dotH.MoveOutput());
                dotCPP = AZStd::move(translation.m_dotCPP.MoveOutput());
                return AZ::Success();
            }
            else
            {
                return AZ::Failure(AZStd::make_pair(AZStd::string(".h errors"), AZStd::string(".cpp errors")));
            }
        }

        void GraphToCPlusPlus::TranslateClassClose()
        {
            m_dotH.Outdent();
            m_dotH.WriteIndent();
            m_dotH.Write("};");
            m_dotH.WriteSpace();
            SingleLineComment(m_dotH);
            m_dotH.WriteSpace();
            m_dotH.WriteLine("class %s", GetGraphName().data());
        }

        void GraphToCPlusPlus::TranslateClassOpen()
        {
            m_dotH.WriteIndent();
            m_dotH.WriteLine("class %s", GetGraphName().data());
            m_dotH.WriteIndent();
            m_dotH.WriteLine("{");
            m_dotH.Indent();
        }

        void GraphToCPlusPlus::TranslateConstruction()
        {

        }

        void GraphToCPlusPlus::TranslateDependencies()
        {
            TranslateDependenciesDotH();
            TranslateDependenciesDotCPP();
        }

        void GraphToCPlusPlus::TranslateDependenciesDotH()
        {

        }

        void GraphToCPlusPlus::TranslateDependenciesDotCPP()
        {

        }

        void GraphToCPlusPlus::TranslateDestruction()
        {

        }
                
        void GraphToCPlusPlus::TranslateHandlers()
        {

        }

        void GraphToCPlusPlus::TranslateNamespaceOpen()
        {
            OpenNamespace(m_dotH, "ScriptCanvas");
            OpenNamespace(m_dotH, GetAutoNativeNamespace());
            OpenNamespace(m_dotCPP, "ScriptCanvas");
            OpenNamespace(m_dotCPP, GetAutoNativeNamespace());
        }

        void GraphToCPlusPlus::TranslateNamespaceClose()
        {
            CloseNamespace(m_dotH, "ScriptCanvas");
            CloseNamespace(m_dotH, GetAutoNativeNamespace());
            CloseNamespace(m_dotCPP, "ScriptCanvas");
            CloseNamespace(m_dotCPP, GetAutoNativeNamespace());
        }

        void GraphToCPlusPlus::TranslateStartNode()
        {
            // write a start function
            const Node* startNode = nullptr;
            
            if (startNode)
            {
                { // .h
                    m_dotH.WriteIndent();
                    m_dotH.WriteLine("public: static void %s(const RuntimeContext& context);", Grammar::k_OnGraphStartFunctionName);
                }
                
                { // .cpp
                    m_dotCPP.WriteIndent();
                    m_dotCPP.WriteLine("void %s::%s(const RuntimeContext& context)", GetGraphName().data(), Grammar::k_OnGraphStartFunctionName);
                    OpenScope(m_dotCPP);
                    {
                        m_dotCPP.WriteIndent();
                        m_dotCPP.WriteLine("AZ_TracePrintf(\"ScriptCanvas\", \"This call wasn't generated from parsing a print node!\");");
                        m_dotCPP.WriteLine("LogNotificationBus::Event(context.GetGraphId(), &LogNotifications::LogMessage, \"This call wasn't generated from parsing a print node!\");");
                        // get the related function call and call it
                        // with possible appropriate variables
                    }
                    CloseScope(m_dotCPP);
                }
            }
        }

        void GraphToCPlusPlus::TranslateVariables()
        {

        }

        void GraphToCPlusPlus::WriteHeader()
        {
            WriteHeaderDotH();
            WriteHeaderDotCPP();
        }

        void GraphToCPlusPlus::WriteHeaderDotCPP()
        {
            WriteCopyright(m_dotCPP);
            m_dotCPP.WriteNewLine();
            WriteDoNotModify(m_dotCPP);
            m_dotCPP.WriteNewLine();
            m_dotCPP.WriteLine("#include \"precompiled.h\"");
            m_dotCPP.WriteLine("#include \"%s.h\"", GetGraphName().data());
            m_dotCPP.WriteNewLine();
        }

        void GraphToCPlusPlus::WriteHeaderDotH()
        {
            WriteCopyright(m_dotH);
            m_dotH.WriteNewLine();
            m_dotH.WriteLine("#pragma once");
            m_dotH.WriteNewLine();
            WriteDoNotModify(m_dotH);
            m_dotH.WriteNewLine();
            m_dotH.WriteLine("#include <Execution/NativeHostDeclarations.h>");
            m_dotH.WriteNewLine();
        }

    } 
} 
