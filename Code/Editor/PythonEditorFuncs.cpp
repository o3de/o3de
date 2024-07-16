/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "PythonEditorFuncs.h"

// Qt
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>

#include <AzCore/Utils/Utils.h>

// AzToolsFramework
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

// Editor
#include "CryEdit.h"
#include "GameEngine.h"
#include "ViewManager.h"
#include "GenericSelectItemDialog.h"
#include "Commands/CommandManager.h"

AZ_CVAR_EXTERNED(bool, ed_previewGameInFullscreen_once);

namespace
{
    //////////////////////////////////////////////////////////////////////////
    const char* PyGetCVarAsString(const char* pName)
    {
        ICVar* pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(pName);
        if (!pCVar)
        {
            AZ_Warning("editor", false, "PyGetCVar: Attempt to access non-existent CVar '%s'", pName ? pName : "(null)");
            return "(missing)";
        }
        return pCVar->GetString();
    }

    //////////////////////////////////////////////////////////////////////////
    // forward declarations
    void PySetCVarFromInt(const char* pName, int pValue);
    void PySetCVarFromFloat(const char* pName, float pValue);

    //////////////////////////////////////////////////////////////////////////
    void PySetCVarFromString(const char* pName, const char* pValue)
    {
        ICVar* pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(pName);
        if (!pCVar)
        {
            AZ_Warning("editor", false, "Attempt to set non-existent string CVar '%s'", pName ? pName : "(null)");
        }
        else if (pCVar->GetType() == CVAR_INT)
        {
            PySetCVarFromInt(pName, static_cast<int>(std::stol(pValue)));
        }
        else if (pCVar->GetType() == CVAR_FLOAT)
        {
            PySetCVarFromFloat(pName, static_cast<float>(std::stod(pValue)));
        }
        else if (pCVar->GetType() != CVAR_STRING)
        {
            AZ_Warning("editor", false, "Type mismatch while assigning CVar '%s' as a string.", pName ? pName : "(null)");
        }
        else
        {
            pCVar->Set(pValue);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void PySetCVarFromInt(const char* pName, int pValue)
    {
        ICVar* pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(pName);
        if (!pCVar)
        {
            AZ_Warning("editor", false, "Attempt to set non-existent integer CVar '%s'", pName ? pName : "(null)");
        }
        else if (pCVar->GetType() == CVAR_FLOAT)
        {
            PySetCVarFromFloat(pName, float(pValue));
        }
        else if (pCVar->GetType() == CVAR_STRING)
        {
            auto stringValue = AZStd::to_string(pValue);
            PySetCVarFromString(pName, stringValue.c_str());
        }
        else if (pCVar->GetType() != CVAR_INT)
        {
            AZ_Warning("editor", false, "Type mismatch while assigning CVar '%s' as an integer.", pName ? pName : "(null)");
        }
        else
        {
            pCVar->Set(pValue);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void PySetCVarFromFloat(const char* pName, float pValue)
    {
        ICVar* pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(pName);
        if (!pCVar)
        {
            AZ_Warning("editor", false, "Attempt to set non-existent float CVar '%s'", pName ? pName : "(null)");
        }
        else if (pCVar->GetType() == CVAR_INT)
        {
            PySetCVarFromInt(pName, static_cast<int>(pValue));
        }
        else if (pCVar->GetType() == CVAR_STRING)
        {
            auto stringValue = AZStd::to_string(pValue);
            PySetCVarFromString(pName, stringValue.c_str());
        }
        else if (pCVar->GetType() != CVAR_FLOAT)
        {
            AZ_Warning("editor", false, "Type mismatch while assigning CVar '%s' as a float.", pName ? pName : "(null)");
        }
        else
        {
            pCVar->Set(pValue);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void PySetCVarFromAny(const char* pName, const AZStd::any& value)
    {
        ICVar* pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(pName);
        if (!pCVar)
        {
            AZ_Warning("editor", false, "Attempt to set non-existent float CVar '%s'", pName ? pName : "(null)");
        }
        else if (pCVar->GetType() == CVAR_INT)
        {
            PySetCVarFromInt(pName, static_cast<int>(AZStd::any_cast<AZ::s64>(value)));
        }
        else if (pCVar->GetType() == CVAR_FLOAT)
        {
            PySetCVarFromFloat(pName, static_cast<float>(AZStd::any_cast<double>(value)));
        }
        else if (pCVar->GetType() == CVAR_STRING)
        {
            PySetCVarFromString(pName, AZStd::any_cast<AZStd::string_view>(value).data());
        }
        else
        {
            AZ_Warning("editor", false, "Type mismatch while assigning CVar '%s' as a float.", pName ? pName : "(null)");
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void PyEnterGameMode()
    {
        if (GetIEditor()->GetGameEngine())
        {
            GetIEditor()->GetGameEngine()->RequestSetGameMode(true);
        }
    }

    void PyEnterGameModeFullscreen()
    {
        ed_previewGameInFullscreen_once = true;
        PyEnterGameMode();
    }

    void PyExitGameMode()
    {
        if (GetIEditor()->GetGameEngine())
        {
            GetIEditor()->GetGameEngine()->RequestSetGameMode(false);
        }
    }

    bool PyIsInGameMode()
    {
        return GetIEditor()->IsInGameMode();
    }

    //////////////////////////////////////////////////////////////////////////
    void PyEnterSimulationMode()
    {
        if (!GetIEditor()->IsInSimulationMode())
        {
            CCryEditApp::instance()->OnSwitchPhysics();
        }
    }

    void PyExitSimulationMode()
    {
        if (GetIEditor()->IsInSimulationMode())
        {
            CCryEditApp::instance()->OnSwitchPhysics();
        }
    }

    bool PyIsInSimulationMode()
    {
        return GetIEditor()->IsInSimulationMode();
    }

    //////////////////////////////////////////////////////////////////////////
    void PyRunConsole(const char* text)
    {
        GetIEditor()->GetSystem()->GetIConsole()->ExecuteString(text);
    }

    //////////////////////////////////////////////////////////////////////////
    bool GetPythonScriptPath(const char* pFile, QString& path)
    {
        bool bRelativePath = true;
        char drive[_MAX_DRIVE];
        char fdir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char fext[_MAX_EXT];
        drive[0] = '\0';

        _splitpath_s(pFile, drive, fdir, fname, fext);
        if (strlen(drive) != 0)
        {
            bRelativePath = false;
        }

        if (bRelativePath)
        {
            // Try to open from user folder
            QString userSandboxFolder = Path::GetResolvedUserSandboxFolder();
            Path::ConvertBackSlashToSlash(userSandboxFolder);
            path = userSandboxFolder + pFile;

            // If not found try editor folder
            if (!CFileUtil::FileExists(path))
            {
                AZ::IO::FixedMaxPathString engineRoot = AZ::Utils::GetEnginePath();
                QDir engineDir = !engineRoot.empty() ? QDir(QString(engineRoot.c_str())) : QDir::current();

                QString scriptFolder = engineDir.absoluteFilePath("Assets/Editor/Scripts/");
                Path::ConvertBackSlashToSlash(scriptFolder);
                path = scriptFolder + pFile;

                if (!CFileUtil::FileExists(path))
                {
                    QString error = QString("Could not find '%1'\n in '%2'\n or '%3'\n").arg(pFile).arg(userSandboxFolder).arg(scriptFolder);
                    AZ_Warning("python", false, error.toUtf8().data());
                    return false;
                }
            }
        }
        else
        {
            path = pFile;
            if (!CFileUtil::FileExists(path))
            {
                QString error = QString("Could not find '") + pFile + "'\n";
                AZ_Warning("python", false, error.toUtf8().data());
                return false;
            }
        }

        Path::ConvertBackSlashToSlash(path);
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    void GetPythonArgumentsVector(const char* pArguments, QStringList& inputArguments)
    {
        if (pArguments == nullptr)
        {
            return;
        }

        inputArguments = QString(pArguments).split(" ", Qt::SkipEmptyParts);
    }

    //////////////////////////////////////////////////////////////////////////
    void PyRunFileWithParameters(const char* pFile, const char* pArguments)
    {
        QString path;
        QStringList inputArguments;
        GetPythonArgumentsVector(pArguments, inputArguments);
        if (GetPythonScriptPath(pFile, path))
        {
            AZStd::vector<AZStd::string_view> argList;
            argList.reserve(inputArguments.size() + 1);
            QByteArray p = path.toUtf8();

            QVector<QByteArray> argData;
            argData.reserve(inputArguments.count());
            for (auto iter = inputArguments.begin(); iter != inputArguments.end(); ++iter)
            {
                argData.push_back(iter->toLatin1());
                argList.push_back(argData.last().data());
            }
            using namespace AzToolsFramework;
            EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs, p.data(), argList);
        }
    }

    void PyRunFile(const char* pFile)
    {
        PyRunFileWithParameters(pFile, nullptr);
    }

    //////////////////////////////////////////////////////////////////////////
    void PyExecuteCommand(const char* cmdline)
    {
        GetIEditor()->GetCommandManager()->Execute(cmdline);
    }

    //////////////////////////////////////////////////////////////////////////
    void PyLog(const char* pMessage)
    {
        if (strcmp(pMessage, "") != 0)
        {
            CryLogAlways("%s", pMessage);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    bool PyMessageBox(const char* pMessage)
    {
        return QMessageBox::information(QApplication::activeWindow(), QString(), pMessage, QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok;
    }

    bool PyMessageBoxYesNo(const char* pMessage)
    {
        return QMessageBox::question(QApplication::activeWindow(), QString(), pMessage) == QMessageBox::Yes;
    }

    bool PyMessageBoxOK(const char* pMessage)
    {
        return QMessageBox::information(QApplication::activeWindow(), QString(), pMessage) == QMessageBox::Ok;
    }

    AZStd::string PyEditBox(AZStd::string_view pTitle)
    {
        QString stringValue = QInputDialog::getText(AzToolsFramework::GetActiveWindow(), pTitle.data(), QString());
        if (!stringValue.isEmpty())
        {
            return stringValue.toUtf8().constData();
        }
        return "";
    }

    AZStd::any PyEditBoxAndCheckProperty(const char* pTitle)
    {
        QString stringValue = QInputDialog::getText(AzToolsFramework::GetActiveWindow(), pTitle, QString());
        if (!stringValue.isEmpty())
        {
            // detect data type
            QString tempString = stringValue;
            int countComa = 0;
            int countOpenRoundBraket = 0;
            int countCloseRoundBraket = 0;
            int countDots = 0;

            int posDots = 0;
            int posComa = 0;
            int posOpenRoundBraket = 0;
            int posCloseRoundBraket = 0;

            for (int i = 0; i < 3; i++)
            {
                if (tempString.indexOf(".", posDots) > -1)
                {
                    posDots = tempString.indexOf(".", posDots) + 1;
                    countDots++;
                }
                if (tempString.indexOf(",", posComa) > -1)
                {
                    posComa = tempString.indexOf(",", posComa) + 1;
                    countComa++;
                }
                if (tempString.indexOf("(", posOpenRoundBraket) > -1)
                {
                    posOpenRoundBraket = tempString.indexOf("(", posOpenRoundBraket) + 1;
                    countOpenRoundBraket++;
                }
                if (tempString.indexOf(")", posCloseRoundBraket) > -1)
                {
                    posCloseRoundBraket = tempString.indexOf(")", posCloseRoundBraket) + 1;
                    countCloseRoundBraket++;
                }
            }

            if (countDots == 3 && countComa == 2 && countOpenRoundBraket == 1 && countCloseRoundBraket == 1)
            {
                // for example: (1.95, 2.75, 3.36)
                QString valueRed = stringValue;
                int iStart = valueRed.indexOf("(");
                valueRed.remove(0, iStart + 1);
                int iEnd = valueRed.indexOf(",");
                valueRed.remove(iEnd, valueRed.length());
                float fValueRed = valueRed.toFloat();

                QString valueGreen = stringValue;
                iStart = valueGreen.indexOf(",");
                valueGreen.remove(0, iStart + 1);
                iEnd = valueGreen.indexOf(",");
                valueGreen.remove(iEnd, valueGreen.length());
                float fValueGreen = valueGreen.toFloat();

                QString valueBlue = stringValue;
                valueBlue.remove(0, valueBlue.indexOf(",") + 1);
                valueBlue.remove(0, valueBlue.indexOf(",") + 1);
                valueBlue.remove(valueBlue.indexOf(")"), valueBlue.length());
                float fValueBlue = valueBlue.toFloat();

                return AZStd::make_any<AZ::Vector3>(fValueRed, fValueGreen, fValueBlue);
            }
            else if (countDots == 0 && countComa == 2 && countOpenRoundBraket == 1 && countCloseRoundBraket == 1)
            {
                // for example: (128, 32, 240)
                const AZ::u8 lowColorValue { 0 };
                const AZ::u8 highColorValue { 255 };
                QString valueRed = stringValue;
                int iStart = valueRed.indexOf("(");
                valueRed.remove(0, iStart + 1);
                int iEnd = valueRed.indexOf(",");
                valueRed.remove(iEnd, valueRed.length());
                AZ::u8 iValueRed = AZStd::clamp(aznumeric_cast<AZ::u8>(valueRed.toInt()), lowColorValue, highColorValue);

                QString valueGreen = stringValue;
                iStart = valueGreen.indexOf(",");
                valueGreen.remove(0, iStart + 1);
                iEnd = valueGreen.indexOf(",");
                valueGreen.remove(iEnd, valueGreen.length());
                AZ::u8 iValueGreen = AZStd::clamp(aznumeric_cast<AZ::u8>(valueGreen.toInt()), lowColorValue, highColorValue);

                QString valueBlue = stringValue;
                valueBlue.remove(0, valueBlue.indexOf(",") + 1);
                valueBlue.remove(0, valueBlue.indexOf(",") + 1);
                valueBlue.remove(valueBlue.indexOf(")"), valueBlue.length());
                AZ::u8 iValueBlue = AZStd::clamp(aznumeric_cast<AZ::u8>(valueBlue.toInt()), lowColorValue, highColorValue);

                return AZStd::make_any<AZ::Color>(iValueRed, iValueGreen, iValueBlue, highColorValue);
            }
            else if (countDots == 1 && countComa == 0 && countOpenRoundBraket == 0 && countCloseRoundBraket == 0)
            {
                // for example: 2.56
                return AZStd::make_any<double>(stringValue.toDouble());
            }
            else if (countDots == 0 && countComa == 0 && countOpenRoundBraket == 0 && countCloseRoundBraket == 0)
            {
                if (stringValue == "False" || stringValue == "True")
                {
                    // for example: True
                    return AZStd::make_any<bool>(stringValue == "True");
                }
                else
                {
                    const bool anyNotDigits = AZStd::any_of(stringValue.begin(), stringValue.end(), [](const QChar& ch) { return !ch.isDigit(); });

                    // looks like a string value?
                    if (stringValue.isEmpty() || anyNotDigits)
                    {
                        // for example: Hello
                        return AZStd::make_any<AZStd::string>(stringValue.toUtf8().data());
                    }
                    else // then it looks like an integer
                    {
                        // for example: 456
                        return AZStd::make_any<AZ::s64>(stringValue.toInt());
                    }
                }
            }
        }
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QObject::tr("Invalid Data"), QObject::tr("Invalid data type."));
        return {};
    }

    AZStd::string PyOpenFileBox()
    {
        QString path = QFileDialog::getOpenFileName();
        if (!path.isEmpty())
        {
            Path::ConvertBackSlashToSlash(path);
        }
        return path.toUtf8().constData();
    }

    AZStd::string PyComboBox(AZStd::string title, AZStd::vector<AZStd::string> values, int selectedIdx = 0)
    {
        AZStd::string result;

        if (title.empty())
        {
            throw std::runtime_error("Incorrect title argument passed in. ");
        }

        if (values.size() == 0)
        {
            throw std::runtime_error("Empty value list passed in. ");
        }

        QStringList list;

        for (const AZStd::string& str : values)
        {
            list.push_back(str.c_str());
        }

        CGenericSelectItemDialog pyDlg;
        pyDlg.setWindowTitle(title.c_str());
        pyDlg.SetMode(CGenericSelectItemDialog::eMODE_LIST);
        pyDlg.SetItems(list);
        pyDlg.PreSelectItem(list[selectedIdx]);

        if (pyDlg.exec() == QDialog::Accepted)
        {
            result = pyDlg.GetSelectedItem().toUtf8().constData();
        }

        return result;
    }

    void PyCrash()
    {
        AZ_Crash();
    }

    static void PyDrawLabel(int x, int y, float size, float r, float g, float b, float a, const char* pLabel)
    {
        if (!pLabel)
        {
            throw std::logic_error("No label given.");
            return;
        }

        if (!r || !g || !b || !a)
        {
            throw std::logic_error("Invalid color parameters given.");
            return;
        }

        if (!x || !y || !size)
        {
            throw std::logic_error("Invalid position or size parameters given.");
        }
        else
        {
            // ToDo: Remove function or update to work with Atom? LYN-3672
            // float color[] = {r, g, b, a};
            // ???->Draw2dLabel(x, y, size, color, false, pLabel);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Constrain
    //////////////////////////////////////////////////////////////////////////
    const char* PyGetAxisConstraint()
    {
        AxisConstrains actualConstrain = GetIEditor()->GetAxisConstrains();
        switch (actualConstrain)
        {
        case AXIS_X:
            return "X";
        case AXIS_Y:
            return "Y";
        case AXIS_Z:
            return "Z";
        case AXIS_XY:
            return "XY";
        case AXIS_XZ:
            return "XZ";
        case AXIS_YZ:
            return "YZ";
        case AXIS_XYZ:
            return "XYZ";
        case AXIS_TERRAIN:
            return (GetIEditor()->IsTerrainAxisIgnoreObjects()) ? "TERRAIN" : "TERRAINSNAP";
        default:
            throw std::logic_error("Invalid axes.");
        }
    }

    void PySetAxisConstraint(AZStd::string_view pConstrain)
    {
        if (pConstrain == "X")
        {
            GetIEditor()->SetAxisConstraints(AXIS_X);
        }
        else if (pConstrain == "Y")
        {
            GetIEditor()->SetAxisConstraints(AXIS_Y);
        }
        else if (pConstrain == "Z")
        {
            GetIEditor()->SetAxisConstraints(AXIS_Z);
        }
        else if (pConstrain == "XY")
        {
            GetIEditor()->SetAxisConstraints(AXIS_XY);
        }
        else if (pConstrain == "YZ")
        {
            GetIEditor()->SetAxisConstraints(AXIS_YZ);
        }
        else if (pConstrain == "XZ")
        {
            GetIEditor()->SetAxisConstraints(AXIS_XZ);
        }
        else if (pConstrain == "XYZ")
        {
            GetIEditor()->SetAxisConstraints(AXIS_XYZ);
        }
        else if (pConstrain == "TERRAIN")
        {
            GetIEditor()->SetAxisConstraints(AXIS_TERRAIN);
            GetIEditor()->SetTerrainAxisIgnoreObjects(true);
        }
        else if (pConstrain == "TERRAINSNAP")
        {
            GetIEditor()->SetAxisConstraints(AXIS_TERRAIN);
            GetIEditor()->SetTerrainAxisIgnoreObjects(false);
        }
        else
        {
            throw std::logic_error("Invalid axes.");
        }
    }

    //////////////////////////////////////////////////////////////////////////
    AZ::IO::Path PyGetPakFromFile(const char* filename)
    {
        auto pIPak = GetIEditor()->GetSystem()->GetIPak();
        AZ::IO::HandleType fileHandle = pIPak->FOpen(filename, "rb");
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            throw std::logic_error("Invalid file name.");
        }
        AZ::IO::Path pArchPath = pIPak->GetFileArchivePath(fileHandle);
        pIPak->FClose(fileHandle);

        return pArchPath;
    }

    //////////////////////////////////////////////////////////////////////////
    void PyUndo()
    {
        GetIEditor()->Undo();
    }

    //////////////////////////////////////////////////////////////////////////
    void PyRedo()
    {
        GetIEditor()->Redo();
    }
}

//////////////////////////////////////////////////////////////////////////
// Temporal, to be removed by LY-101149
AZ::EntityId PyFindEditorEntity(const char* name)
{
    AZ::EntityId foundEntityId;

    auto searchFunc = [name, &foundEntityId](AZ::Entity* e)
    {
        if (!foundEntityId.IsValid() && e->GetName() == name)
        {
            bool isEditorEntity = false;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(isEditorEntity, &AzToolsFramework::EditorEntityContextRequests::IsEditorEntity, e->GetId());
            if (isEditorEntity)
            {
                foundEntityId = e->GetId();
            }
        }
    };

    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::EnumerateEntities, searchFunc);
    return foundEntityId;
}

AZ::EntityId PyFindGameEntity(const char* name)
{
    AZ::EntityId foundEntityId;

    auto searchFunc = [name, &foundEntityId](AZ::Entity* e)
    {
        if (!foundEntityId.IsValid() && e->GetName() == name)
        {
            bool isEditorEntity = true;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(isEditorEntity, &AzToolsFramework::EditorEntityContextRequests::IsEditorEntity, e->GetId());
            if (!isEditorEntity)
            {
                foundEntityId = e->GetId();
            }
        }
    };

    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::EnumerateEntities, searchFunc);
    return foundEntityId;
}


struct PyDumpBindings
{
    static AZ_INLINE bool IsBehaviorFlaggedForEditor(const AZ::AttributeArray& attributes)
    {
        // defaults to Launcher
        AZ::Script::Attributes::ScopeFlags scopeType = AZ::Script::Attributes::ScopeFlags::Launcher;
        AZ::Attribute* scopeAttribute = AZ::FindAttribute(AZ::Script::Attributes::Scope, attributes);
        if (scopeAttribute)
        {
            AZ::AttributeReader scopeAttributeReader(nullptr, scopeAttribute);
            scopeAttributeReader.Read<AZ::Script::Attributes::ScopeFlags>(scopeType);
        }
        return (scopeType == AZ::Script::Attributes::ScopeFlags::Automation || scopeType == AZ::Script::Attributes::ScopeFlags::Common);
    }

    static AZ_INLINE AZStd::string GetModuleName(const AZ::AttributeArray& attributes)
    {
        AZStd::string moduleName;
        AZ::Attribute* moduleAttribute = AZ::FindAttribute(AZ::Script::Attributes::Module, attributes);
        if (moduleAttribute)
        {
            AZ::AttributeReader scopeAttributeReader(nullptr, moduleAttribute);
            scopeAttributeReader.Read<AZStd::string>(moduleName);
        }
        if (!moduleName.empty())
        {
            moduleName = "azlmbr." + moduleName;
        }
        else
        {
            moduleName = "azlmbr";
        }

        return moduleName;
    }


    static AZStd::string ParameterToString(const AZ::BehaviorMethod* method, size_t index)
    {
        const AZStd::string* argNameStr = method->GetArgumentName(index);
        const char* argName = (argNameStr && !argNameStr->empty()) ? argNameStr->c_str() : nullptr;
        if (argName)
        {
            return AZStd::string::format("%s %s", method->GetArgument(index)->m_name, argName);
        }
        else
        {
            return method->GetArgument(index)->m_name;
        }
    }

    static AZStd::string MethodArgumentsToString(const AZ::BehaviorMethod* method)
    {
        AZStd::string ret;
        AZStd::string argumentStr;
        for (size_t i = 0; i < method->GetNumArguments(); ++i)
        {
            argumentStr = ParameterToString(method, i);
            ret += argumentStr;
            if (i < method->GetNumArguments() - 1)
            {
                ret += ", ";
            }
        }
        return ret;
    }

    static AZStd::string MethodToString(const AZStd::string& methodName, const AZ::BehaviorMethod* method)
    {
        AZStd::string methodNameStrip = methodName.data() + methodName.rfind(':') + 1; // remove ClassName:: part as it is redundant
        return AZStd::string::format("%s %s(%s)%s", method->GetResult()->m_name, methodNameStrip.c_str(), MethodArgumentsToString(method).c_str(), method->m_isConst ? " const" : "");
    }

    static AZStd::string GetExposedPythonClasses()
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

        AZStd::string output = "";
        output += "// Classes\n\n";

        for (auto elem : behaviorContext->m_classes)
        {
            AZ::BehaviorClass* cls = elem.second;

            bool exposedToPython = IsBehaviorFlaggedForEditor(cls->m_attributes);
            if (!exposedToPython)
                continue;

            output += AZStd::string::format("// Module: %s\n", GetModuleName(cls->m_attributes).c_str());
            output += AZStd::string::format("class %s\n", cls->m_name.c_str());
            output += "{\n";

            if (cls->m_methods.size() > 0)
            {
                output += "    // Methods\n";
                for (auto method_elem : cls->m_methods)
                {
                    output += AZStd::string::format("    %s;\n", MethodToString(method_elem.first, method_elem.second).c_str());
                }
            }
            if (cls->m_properties.size() > 0)
            {
                output += "    // Properties\n";
                for (auto property_elem : cls->m_properties)
                {
                    AZ::BehaviorProperty* bproperty = property_elem.second;
                    output += AZStd::string::format("    %s %s;\n", bproperty->m_getter->GetResult()->m_name, bproperty->m_name.c_str());
                }
            }

            output += "}\n";
        }
        output += "\n\n// Ebuses\n\n";
        for (auto elem : behaviorContext->m_ebuses)
        {
            AZ::BehaviorEBus* ebus = elem.second;

            bool exposedToPython = IsBehaviorFlaggedForEditor(ebus->m_attributes);
            if (!exposedToPython)
                continue;

            output += AZStd::string::format("// Module: %s\n", GetModuleName(ebus->m_attributes).c_str());
            output += AZStd::string::format("ebus %s\n", ebus->m_name.c_str());
            output += "{\n";
            for (auto event_elem : ebus->m_events)
            {
                auto method = event_elem.second.m_event ? event_elem.second.m_event : event_elem.second.m_broadcast;
                if (method)
                {
                    const char* comment = event_elem.second.m_event ? "/* event */" : "/* broadcast */";
                    output += AZStd::string::format("    %s %s\n", comment, MethodToString(event_elem.first, method).c_str());
                }
                else
                {
                    output += AZStd::string::format("    %s %s\n", "/* unknown */", event_elem.first.c_str());
                }
            }

            if (ebus->m_createHandler)
            {
                AZ::BehaviorEBusHandler* handler = nullptr;
                ebus->m_createHandler->InvokeResult(handler);
                if (handler)
                {
                    const auto& notifications = handler->GetEvents();
                    for (const auto& notification : notifications)
                    {
                        AZStd::string argsStr;
                        const size_t paramCount = notification.m_parameters.size();
                        for (size_t i = 0; i < notification.m_parameters.size(); ++i)
                        {
                            AZStd::string argName = notification.m_parameters[i].m_name;
                            argsStr += argName;
                            if (i != paramCount - 1)
                            {
                                argsStr += ", ";
                            }
                        }

                        AZStd::string funcName = notification.m_name;
                        output += AZStd::string::format("    /* notification */ %s(%s);\n", funcName.c_str(), argsStr.c_str());
                    }
                    ebus->m_destroyHandler->Invoke(handler);
                }
            }

            output += "}\n";

        }
        AzFramework::StringFunc::Replace(output, "AZStd::basic_string<char, AZStd::char_traits<char>, allocator>", "AZStd::string");
        return output;
    }
};

//////////////////////////////////////////////////////////////////////////

namespace AzToolsFramework
{
    void PythonEditorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorLayerPythonRequestBus>("PythonEditorBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "python_editor_funcs")
                ->Event("GetCVar", &EditorLayerPythonRequestBus::Events::GetCVar)
                ->Event("SetCVar", &EditorLayerPythonRequestBus::Events::SetCVar)
                ->Event("SetCVarFromString", &EditorLayerPythonRequestBus::Events::SetCVarFromString)
                ->Event("SetCVarFromInteger", &EditorLayerPythonRequestBus::Events::SetCVarFromInteger)
                ->Event("SetCVarFromFloat", &EditorLayerPythonRequestBus::Events::SetCVarFromFloat)
                ->Event("RunConsole", &EditorLayerPythonRequestBus::Events::PyRunConsole)
                ->Event("EnterGameMode", &EditorLayerPythonRequestBus::Events::EnterGameMode)
                ->Event("IsInGameMode", &EditorLayerPythonRequestBus::Events::IsInGameMode)
                ->Event("ExitGameMode", &EditorLayerPythonRequestBus::Events::ExitGameMode)
                ->Event("EnterSimulationMode", &EditorLayerPythonRequestBus::Events::EnterSimulationMode)
                ->Event("IsInSimulationMode", &EditorLayerPythonRequestBus::Events::IsInSimulationMode)
                ->Event("ExitSimulationMode", &EditorLayerPythonRequestBus::Events::ExitSimulationMode)
                ->Event("RunFile", &EditorLayerPythonRequestBus::Events::RunFile)
                ->Event("RunFileParameters", &EditorLayerPythonRequestBus::Events::RunFileParameters)
                ->Event("ExecuteCommand", &EditorLayerPythonRequestBus::Events::ExecuteCommand)
                ->Event("MessageBoxOkCancel", &EditorLayerPythonRequestBus::Events::MessageBoxOkCancel)
                ->Event("MessageBoxYesNo", &EditorLayerPythonRequestBus::Events::MessageBoxYesNo)
                ->Event("MessageBoxOk", &EditorLayerPythonRequestBus::Events::MessageBoxOk)
                ->Event("EditBox", &EditorLayerPythonRequestBus::Events::EditBox)
                ->Event("EditBoxCheckDataType", &EditorLayerPythonRequestBus::Events::EditBoxCheckDataType)
                ->Event("OpenFileBox", &EditorLayerPythonRequestBus::Events::OpenFileBox)
                ->Event("GetAxisConstraint", &EditorLayerPythonRequestBus::Events::GetAxisConstraint)
                ->Event("SetAxisConstraint", &EditorLayerPythonRequestBus::Events::SetAxisConstraint)
                ->Event("GetPakFromFile", &EditorLayerPythonRequestBus::Events::GetPakFromFile)
                ->Event("Log", &EditorLayerPythonRequestBus::Events::Log)
                ->Event("Undo", &EditorLayerPythonRequestBus::Events::Undo)
                ->Event("Redo", &EditorLayerPythonRequestBus::Events::Redo)
                ->Event("DrawLabel", &EditorLayerPythonRequestBus::Events::DrawLabel)
                ->Event("ComboBox", &EditorLayerPythonRequestBus::Events::ComboBox)
                ;
        }
    }

    void PythonEditorComponent::Activate()
    {
        EditorLayerPythonRequestBus::Handler::BusConnect(GetEntityId());
    }

    void PythonEditorComponent::Deactivate()
    {
        EditorLayerPythonRequestBus::Handler::BusDisconnect();
    }

    const char* PythonEditorComponent::GetCVar(const char* pName)
    {
        return PyGetCVarAsString(pName);
    }

    void PythonEditorComponent::SetCVar(const char* pName, const AZStd::any& value)
    {
        return PySetCVarFromAny(pName, value);
    }

    void PythonEditorComponent::SetCVarFromString(const char* pName, const char* pValue)
    {
        return PySetCVarFromString(pName, pValue);
    }

    void PythonEditorComponent::SetCVarFromInteger(const char* pName, int pValue)
    {
        return PySetCVarFromInt(pName, pValue);
    }

    void PythonEditorComponent::SetCVarFromFloat(const char* pName, float pValue)
    {
        return PySetCVarFromFloat(pName, pValue);
    }

    void PythonEditorComponent::PyRunConsole(const char* text)
    {
        return ::PyRunConsole(text);
    }

    void PythonEditorComponent::EnterGameMode()
    {
        return PyEnterGameMode();
    }

    bool PythonEditorComponent::IsInGameMode()
    {
        return PyIsInGameMode();
    }

    void PythonEditorComponent::ExitGameMode()
    {
        return PyExitGameMode();
    }

    void PythonEditorComponent::EnterSimulationMode()
    {
        return PyEnterSimulationMode();
    }

    bool PythonEditorComponent::IsInSimulationMode()
    {
        return PyIsInSimulationMode();
    }

    void PythonEditorComponent::ExitSimulationMode()
    {
        return PyExitSimulationMode();
    }

    void PythonEditorComponent::RunFile(const char *pFile)
    {
        return PyRunFile(pFile);
    }

    void PythonEditorComponent::RunFileParameters(const char* pFile, const char* pArguments)
    {
        return PyRunFileWithParameters(pFile, pArguments);
    }

    void PythonEditorComponent::ExecuteCommand(const char* cmdline)
    {
        return PyExecuteCommand(cmdline);
    }

    bool PythonEditorComponent::MessageBoxOkCancel(const char* pMessage)
    {
        return PyMessageBox(pMessage);
    }

    bool PythonEditorComponent::MessageBoxYesNo(const char* pMessage)
    {
        return PyMessageBoxYesNo(pMessage);
    }

    bool PythonEditorComponent::MessageBoxOk(const char* pMessage)
    {
        return PyMessageBoxOK(pMessage);
    }

    AZStd::string PythonEditorComponent::EditBox(AZStd::string_view pTitle)
    {
        return PyEditBox(pTitle);
    }

    AZStd::any PythonEditorComponent::EditBoxCheckDataType(const char* pTitle)
    {
        return PyEditBoxAndCheckProperty(pTitle);
    }

    AZStd::string PythonEditorComponent::OpenFileBox()
    {
        return PyOpenFileBox();
    }

    const char* PythonEditorComponent::GetAxisConstraint()
    {
        return PyGetAxisConstraint();
    }

    void PythonEditorComponent::SetAxisConstraint(AZStd::string_view pConstrain)
    {
        return PySetAxisConstraint(pConstrain);
    }

    AZ::IO::Path PythonEditorComponent::GetPakFromFile(const char* filename)
    {
        return PyGetPakFromFile(filename);
    }

    void PythonEditorComponent::Log(const char* pMessage)
    {
        return PyLog(pMessage);
    }

    void PythonEditorComponent::Undo()
    {
        return PyUndo();
    }

    void PythonEditorComponent::Redo()
    {
        return PyRedo();
    }

    void PythonEditorComponent::DrawLabel(int x, int y, float size, float r, float g, float b, float a, const char* pLabel)
    {
        return PyDrawLabel(x, y, size, r, g, b, a, pLabel);
    }

    AZStd::string PythonEditorComponent::ComboBox(AZStd::string title, AZStd::vector<AZStd::string> values, int selectedIdx)
    {
        return PyComboBox(title, values, selectedIdx);
    }
}

namespace AzToolsFramework
{
    void PythonEditorFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.general' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/General")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.general");
            };
            addLegacyGeneral(behaviorContext->Method("get_cvar", PyGetCVarAsString, nullptr, "Gets a CVar value as a string."));
            addLegacyGeneral(behaviorContext->Method("set_cvar", PySetCVarFromAny, nullptr, "Sets a CVar value from any simple value."));
            addLegacyGeneral(behaviorContext->Method("set_cvar_string", PySetCVarFromString, nullptr, "Sets a CVar value from a string."));
            addLegacyGeneral(behaviorContext->Method("set_cvar_integer", PySetCVarFromInt, nullptr, "Sets a CVar value from an integer."));
            addLegacyGeneral(behaviorContext->Method("set_cvar_float", PySetCVarFromFloat, nullptr, "Sets a CVar value from a float."));
            addLegacyGeneral(behaviorContext->Method("run_console", PyRunConsole, nullptr, "Runs a console command."));

            addLegacyGeneral(behaviorContext->Method("enter_game_mode", PyEnterGameMode, nullptr, "Enters the editor game mode."));
            addLegacyGeneral(behaviorContext->Method("enter_game_mode_fullscreen", PyEnterGameModeFullscreen, nullptr, "Enters the editor game mode in fullscreen."));
            addLegacyGeneral(behaviorContext->Method("is_in_game_mode", PyIsInGameMode, nullptr, "Queries if it's in the game mode or not."));
            addLegacyGeneral(behaviorContext->Method("exit_game_mode", PyExitGameMode, nullptr, "Exits the editor game mode."));

            addLegacyGeneral(behaviorContext->Method("enter_simulation_mode", PyEnterSimulationMode, nullptr, "Enters the editor AI/Physics simulation mode."));
            addLegacyGeneral(behaviorContext->Method("is_in_simulation_mode", PyIsInSimulationMode, nullptr, "Queries if the editor is currently in the AI/Physics simulation mode or not."));
            addLegacyGeneral(behaviorContext->Method("exit_simulation_mode", PyExitSimulationMode, nullptr, "Exits the editor AI/Physics simulation mode."));
            addLegacyGeneral(behaviorContext->Method("run_file", PyRunFile, nullptr, "Runs a script file. A relative path from the editor user folder or an absolute path should be given as an argument."));
            addLegacyGeneral(behaviorContext->Method("run_file_parameters", PyRunFileWithParameters, nullptr, "Runs a script file with parameters. A relative path from the editor user folder or an absolute path should be given as an argument. The arguments should be separated by whitespace."));
            addLegacyGeneral(behaviorContext->Method("execute_command", PyExecuteCommand, nullptr, "Executes a given string as an editor command."));

            addLegacyGeneral(behaviorContext->Method("message_box", PyMessageBox, nullptr, "Shows a confirmation message box with ok|cancel and shows a custom message."));
            addLegacyGeneral(behaviorContext->Method("message_box_yes_no", PyMessageBoxYesNo, nullptr, "Shows a confirmation message box with yes|no and shows a custom message."));
            addLegacyGeneral(behaviorContext->Method("message_box_ok", PyMessageBoxOK, nullptr, "Shows a confirmation message box with only ok and shows a custom message."));
            addLegacyGeneral(behaviorContext->Method("edit_box", PyEditBox, nullptr, "Shows an edit box and returns the value as string."));

            addLegacyGeneral(behaviorContext->Method("edit_box_check_data_type", PyEditBoxAndCheckProperty, nullptr, "Shows an edit box and checks the custom value to use the return value with other functions correctly."));

            addLegacyGeneral(behaviorContext->Method("open_file_box", PyOpenFileBox, nullptr, "Shows an open file box and returns the selected file path and name."));

            addLegacyGeneral(behaviorContext->Method("get_axis_constraint", PyGetAxisConstraint, nullptr, "Gets axis."));
            addLegacyGeneral(behaviorContext->Method("set_axis_constraint", PySetAxisConstraint, nullptr, "Sets axis."));

            addLegacyGeneral(behaviorContext->Method("get_pak_from_file", [](const char* filename) -> AZStd::string { return PyGetPakFromFile(filename).Native(); }, nullptr, "Finds a pak file name for a given file."));

            addLegacyGeneral(behaviorContext->Method("log", PyLog, nullptr, "Prints the message to the editor console window."));

            addLegacyGeneral(behaviorContext->Method("undo", PyUndo, nullptr, "Undoes the last operation."));
            addLegacyGeneral(behaviorContext->Method("redo", PyRedo, nullptr, "Redoes the last undone operation."));

            addLegacyGeneral(behaviorContext->Method("draw_label", PyDrawLabel, nullptr, "Shows a 2d label on the screen at the given position and given color."));
            addLegacyGeneral(behaviorContext->Method("combo_box", PyComboBox, nullptr, "Shows a combo box listing each value passed in, returns string value selected by the user."));

            addLegacyGeneral(behaviorContext->Method("crash", PyCrash, nullptr, "Crashes the application, useful for testing crash reporting and other automation tools."));

            /////////////////////////////////////////////////////////////////////////
            // Temporal, to be removed by LY-101149
            addLegacyGeneral(behaviorContext->Method("find_editor_entity", PyFindEditorEntity, nullptr, "Retrieves a editor entity id by name"));
            addLegacyGeneral(behaviorContext->Method("find_game_entity", PyFindGameEntity, nullptr, "Retrieves a game entity id by name"));
            //////////////////////////////////////////////////////////////////////////
            addLegacyGeneral(behaviorContext->Method("dump_exposed_classes", PyDumpBindings::GetExposedPythonClasses, nullptr, "Retrieves exposed classes"));
        }
    }
}
