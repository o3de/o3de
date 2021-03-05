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

#include "precompiled.h"
#include "GraphToCPlusPlus.h"

#include <ScriptCanvas/Grammar/AbstractCodeModel.h>

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
                    TranslateFunctions();
                    TranslateStartNode();
                }
                TranslateClassClose();
            }
            TranslateNamespaceClose();
        }

        AZ::Outcome<void, AZStd::string> GraphToCPlusPlus::Translate(const Grammar::AbstractCodeModel& model, AZStd::string& dotH, AZStd::string& dotCPP)
        {
            GraphToCPlusPlus translation(model);
            auto outcome = translation.m_outcome;

            if (outcome.IsSuccess())
            {
                dotH = AZStd::move(translation.m_dotH.MoveOutput());
                dotCPP = AZStd::move(translation.m_dotCPP.MoveOutput());
            }

            return outcome;
        }

        void GraphToCPlusPlus::TranslateClassClose()
        {
            m_dotH.Outdent();
            m_dotH.WriteIndent();
            m_dotH.Write("};");
            m_dotH.WriteSpace();
            SingleLineComment(m_dotH);
            m_dotH.WriteSpace();
            m_dotH.WriteLine("class %s", GetGraphName());
        }

        void GraphToCPlusPlus::TranslateClassOpen()
        {
            m_dotH.WriteIndent();
            m_dotH.WriteLine("class %s", GetGraphName());
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

        void GraphToCPlusPlus::TranslateFunctions()
        {
            const Grammar::Functions& functions = m_model.GetFunctions();

            for (const auto& functionsEntry : functions)
            {
                const Grammar::Function& function = functionsEntry.second;
                // TranslateFunction(function);
            }
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
            if (const Node* startNode = m_model.GetStartNode())
            {
                { // .h
                    m_dotH.WriteIndent();
                    m_dotH.WriteLine("public: static void OnGraphStart(const RuntimeContext& context);");
                }
                
                { // .cpp
                    m_dotCPP.WriteIndent();
                    m_dotCPP.WriteLine("void %s::OnGraphStart(const RuntimeContext& context)", GetGraphName());
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

        void GraphToCPlusPlus::WriterHeader()
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
            m_dotCPP.WriteLine("#include \"%s.h\"", GetGraphName());
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

    } // namespace Translation
} // namespace ScriptCanvas