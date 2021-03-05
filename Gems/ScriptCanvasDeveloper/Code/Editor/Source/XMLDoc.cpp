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

#include "XMLDoc.h"
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace ScriptCanvasDeveloperEditor
{
    namespace Internal
    {
        xml_node<> *FindDocumentTypeNode(const xml_document<> & doc, const AZStd::string & nodeName)
        {
            // this will only work if the xml document was parsed with the flag "parse_doctype_node"

            for(xml_node<> *docTypeNode=doc.first_node(); docTypeNode!=nullptr; docTypeNode=docTypeNode->next_sibling())
            {
                if (docTypeNode->type() == node_type::node_doctype)
                {
                    if ( nodeName == docTypeNode->value() )
                    {
                        return docTypeNode;
                    }
                }
            }

            return nullptr;
        }

        bool GetAttribute(xml_node<> *tsNode, AZStd::string_view attribName, float & value)
        {
            if( tsNode != nullptr)
            {
                for (xml_attribute<> *attrib = tsNode->first_attribute(); attrib != nullptr; attrib = attrib->next_attribute())
                {
                    if ( attribName == attrib->name() )
                    {
                        value = AzFramework::StringFunc::ToFloat( attrib->value() );
                        return true;
                    }
                }
            }

           return false;
        }

        bool GetAttribute(xml_node<> *tsNode, AZStd::string_view attribName, AZStd::string & value)
        {
            if( tsNode != nullptr)
            {
                for (xml_attribute<> *attrib = tsNode->first_attribute(); attrib != nullptr; attrib = attrib->next_attribute())
                {
                    if (attribName == attrib->name())
                    {
                        value = attrib->value();
                        return true;
                    }
                }
            }

            return false;
        }

        xml_node<> *FindContextNode(const xml_document<> & doc, const AZStd::string & contextName)
        {
            xml_node<> *tsNode = doc.first_node("TS");
            
            if (tsNode != nullptr)
            {
                for(xml_node<> *contextNode=tsNode->first_node("context"); contextNode!=nullptr; contextNode=contextNode->next_sibling("context") )
                {
                    xml_node<> *contextNameNode = contextNode->first_node("name");

                    if (contextNameNode != nullptr)
                    {
                        if( contextName == contextNameNode->value() )
                        {
                            return contextNode;
                        }
                    }
                }
            }

            return nullptr;
        }
    }

    XMLDocPtr XMLDoc::Alloc(const AZStd::string& contextName)
    {
        XMLDocPtr xmlDoc( AZStd::make_shared<XMLDoc>() );

        xmlDoc->CreateTSDoc(contextName);

        return xmlDoc;
    }

    XMLDocPtr XMLDoc::LoadFromDisk(const AZStd::string& fileName)
    {
        XMLDocPtr xmlDoc(AZStd::make_shared<XMLDoc>());

        if ( !xmlDoc->LoadTSDoc(fileName) )
        {
            xmlDoc = nullptr;
        }

        return xmlDoc;
    }
    
    XMLDoc::XMLDoc()
        : m_tsNode(nullptr)
        , m_context(nullptr)
        , m_readBuffer(0)
    {
    }

    void XMLDoc::CreateTSDoc(const AZStd::string& contextName)
    {
        xml_node<>* decl = m_doc.allocate_node(node_declaration);
        decl->append_attribute(m_doc.allocate_attribute("version", "1.0"));
        decl->append_attribute(m_doc.allocate_attribute("encoding", "utf-8"));
        m_doc.append_node(decl);

        xml_node<>* commentNode = m_doc.allocate_node(node_comment, "", AllocString(AZStd::string::format("Generated for %s", contextName.c_str())));
        m_doc.append_node(commentNode);

        xml_node<>* docType = m_doc.allocate_node(node_doctype, "", "TS");
        m_doc.append_node(docType);

        m_tsNode = m_doc.allocate_node(node_element, "TS");
        m_doc.append_node(m_tsNode);
        m_tsNode->append_attribute(m_doc.allocate_attribute("version", "2.1"));
        m_tsNode->append_attribute(m_doc.allocate_attribute("language", "en_US"));
    }

    bool XMLDoc::LoadTSDoc(const AZStd::string& fileName)
    {
        bool success = false;

        char tsFilePath[AZ::IO::MaxPathLength] = { "" };

        if (AZ::IO::FileIOBase::GetInstance()->ResolvePath(fileName.c_str(), tsFilePath, AZ::IO::MaxPathLength))
        {
            AZ::IO::FileIOStream xmlFile;

            if( AZ::IO::FileIOBase::GetInstance()->Exists(tsFilePath) )
            {
                if ( xmlFile.Open(tsFilePath, AZ::IO::OpenMode::ModeRead|AZ::IO::OpenMode::ModeText) )
                {
                    AZ::IO::SizeType bytesToRead = xmlFile.GetLength();

                    if( bytesToRead > 0 )
                    {
                        m_readBuffer.resize(bytesToRead, '\0');
                        if( xmlFile.Read(bytesToRead, m_readBuffer.data() ) == bytesToRead )
                        {
                            m_doc.parse<parse_doctype_node|parse_declaration_node|parse_comment_nodes>( m_readBuffer.data() );

                            success = IsValidTSDoc();

                            if (success)
                            {
                                AZ_TracePrintf("ScriptCanvas", "Loaded \"%s\"", tsFilePath);
                            }
                        }
                        else
                        {
                            AZ_Error("ScriptCanvas", false, "XMLDoc::LoadTSDoc-Error reading Qt .ts file! filename=\"%s\".", tsFilePath);
                        }
                    }
                    else
                    {
                        AZ_Error("ScriptCanvas", false, "XMLDoc::LoadTSDoc-Zero byte Qt .ts file! filename=\"%s\".", tsFilePath);
                    }
                }
                else
                {
                    AZ_Error("ScriptCanvas", false, "XMLDoc::LoadTSDoc-Can't open file Qt .ts file! filename=\"%s\".", tsFilePath);
                }
            }
        }
        else
        {
            AZ_Error("ScriptCanvas", false, "XMLDoc::LoadTSDoc-Invalid filename specified! filename=\"%s\".", tsFilePath);
        }
        
        return success;
    }

    bool XMLDoc::WriteToDisk(const AZStd::string & fileName)
    {
        bool success = false;

        char tsFilePath[AZ::IO::MaxPathLength] = { "" };
        if (AZ::IO::FileIOBase::GetInstance()->ResolvePath(fileName.c_str(), tsFilePath, AZ::IO::MaxPathLength))
        {
            AZStd::string writeFolder(tsFilePath);
            AzFramework::StringFunc::Path::StripFullName(writeFolder);

            if (!AZ::IO::FileIOBase::GetInstance()->IsDirectory(writeFolder.c_str()))
            {
                AZ::IO::FileIOBase::GetInstance()->CreatePath(writeFolder.c_str());
            }
       
            AZ::IO::FileIOStream xmlFile;

            if (xmlFile.Open(tsFilePath, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText))
            {
                AZStd::string xmlData(ToString());
                AZ::IO::SizeType bytesWritten = xmlFile.Write(xmlData.size(), xmlData.data());

                if (bytesWritten != xmlData.size())
                {
                    AZ_Error("ScriptCanvas", false, "Write error writing out %s, bytes actually written: %llu expected bytes to write: %llu!", tsFilePath, bytesWritten, xmlData.size());
                }
                else
                {
                    AZ_TracePrintf("ScriptCanvas", "Successfully wrote out ScriptCanvas localization file \"%s\".", tsFilePath);
                    success = true;
                }

                xmlFile.Close();
            }
            else
            {
                AZ_Error("ScriptCanvas", false, "Could not open file \"%s\"!", tsFilePath);
            }
        }
        else
        {
            AZ_Error("ScriptCanvas", false, "Invalid filename specified XMLDoc::WriteToDisk! filename=\"%s\".", tsFilePath);
        }

        return success;
    }

    
    bool XMLDoc::StartContext(const AZStd::string& contextName)
    {
        bool isNew = false;

        if( !contextName.empty() )
        {
            // check to see if we have a context of this name already, if so find that node and use it, otherwise
            // create a new one.

            xml_node<> *existingContextNode = Internal::FindContextNode(m_doc, contextName);

            if (existingContextNode == nullptr)
            {
                m_context = m_doc.allocate_node(node_element, "context");
                m_tsNode->append_node(m_context);

                xml_node<>* contextNameNode = m_doc.allocate_node(node_element, "name", AllocString(contextName));
                m_context->append_node(contextNameNode);

                isNew = true;
            }
            else
            {
                m_context = existingContextNode;
            }
        }
        else
        {
            m_context = nullptr;
        }

        return isNew;
    }
            
    void XMLDoc::AddToContext(const AZStd::string& id, const AZStd::string& translation/*=""*/, const AZStd::string& comment /*=""*/, const AZStd::string& source/*=""*/)
    {
        if (m_context == nullptr)
        {
            return;
        }

        xml_node<>* messageNode = m_doc.allocate_node(node_element, "message");

        messageNode->append_attribute(m_doc.allocate_attribute("id", AllocString(id)));

        xml_node<>* sourceNode = m_doc.allocate_node(node_element, "source", AllocString(source.empty() ? id.c_str() : source.c_str()));
        messageNode->append_node(sourceNode);

        xml_node<>* translationNode = m_doc.allocate_node(node_element, "translation", AllocString(translation));
        messageNode->append_node(translationNode);

        xml_node<>* commentNode = m_doc.allocate_node(node_element, "comment", AllocString(comment));
        messageNode->append_node(commentNode);
            
        m_context->append_node(messageNode);
    }

    bool XMLDoc::MethodFamilyExists(const AZStd::string& baseId) const
    {
        if (m_tsNode != nullptr)
        {
            AZStd::string nameID(baseId + "_NAME");

            for (xml_node<> *contextNode=m_tsNode->first_node("context"); contextNode!=nullptr; contextNode=contextNode->next_sibling("context"))
            {
                for (xml_node<> *messageNode = contextNode->first_node("message"); messageNode != nullptr; messageNode = messageNode->next_sibling("message"))
                {
                    AZStd::string id;

                    if ( Internal::GetAttribute(messageNode, "id", id) )
                    {
                        if (id == nameID)
                        {
                            return true;
                        }
                    }
                }
            }
        }
        
        return false;
    }

    AZStd::string XMLDoc::ToString() const
    {
        AZStd::string buffer;

        print( AZStd::back_inserter(buffer), m_doc, 0);

        return buffer;
    }

    const char *XMLDoc::AllocString(const AZStd::string& str)
    {
        return m_doc.allocate_string( str.c_str(), str.size() + 1 );
    }

    bool XMLDoc::IsValidTSDoc()
    {
        bool isTSDoc = false;
        //
        // Basic format of a .ts file, we are checking to make sure the document type is ts, and the version is 2.1 or greater
        // and there is a non-null language attribute. and at least 1 context section
        //
        //<?xml version="1.0" encoding="utf-8"?>
        //<!DOCTYPE TS>
        //<TS version="2.1" language="en_US">
        //    <context>
        //      ...
        //    </context>
        //</TS>
        //

        // this node should be the doc type
       
        xml_node<> *docTypeNode = Internal::FindDocumentTypeNode(m_doc, "TS");

        if (docTypeNode !=nullptr)
        {
            xml_node<> *tsNode = m_doc.first_node("TS");

            if( tsNode != nullptr)
            {
                float attribVersion = 0.0f;
                if( Internal::GetAttribute(tsNode, "version", attribVersion) && (attribVersion >= 2.1f) )
                {
                    AZStd::string attribLanguage;
                    if( Internal::GetAttribute(tsNode, "language", attribLanguage) && !attribLanguage.empty() )
                    {
                        xml_node<> *contextNode = tsNode->first_node("context");
                        if(contextNode != nullptr)
                        {
                            m_tsNode = tsNode;
                            
                            AZ_TracePrintf("ScriptCanvas", "TS: Version=%2.1f, Language=\"%s\"", attribVersion, attribLanguage.c_str());
                            isTSDoc = true;
                        }
                        else
                        {
                            AZ_Warning("ScriptCanvas", false, "TS document contains no \"context\" nodes!");
                        }
                    }
                    else
                    {
                        AZ_Warning("ScriptCanvas", false, "TS document has a bad or missing \"language\" attribute!");
                    }
                }
                else
                {
                    AZ_Warning("ScriptCanvas", false, "TS document has a bad or missing \"version\" attribute!");
                }
            }
            else
            {
                AZ_Error("ScriptCanvas", false, "TS document contains no \"TS\" node!");
            }
        }
        else
        {
            AZ_Error("ScriptCanvas", false, "XML doc is not a valid TS document!");
        }

        return isTSDoc;
    }
}
