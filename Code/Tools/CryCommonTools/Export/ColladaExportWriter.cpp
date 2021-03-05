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

#include "StdAfx.h"

#include "ColladaExportWriter.h"
#include "ColladaWriter.h"
#include "IExportSource.h"
#include "PathHelpers.h"
#include "ResourceCompilerHelper.h"
#include "SettingsManagerHelpers.h"
#include "IExportContext.h"
#include "ProgressRange.h"
#include "XMLWriter.h"
#include "XMLPakFileSink.h"
#include "ISettings.h"
#include "SingleAnimationExportSourceAdapter.h"
#include "GeometryExportSourceAdapter.h"
#include "ModelData.h"
#include "MaterialData.h"
#include "GeometryFileData.h"
#include "FileUtil.h"
#include "CBAHelpers.h"
#include "ModuleHelpers.h"
#include "PropertyHelpers.h"
#include "StringHelpers.h"
#include <ctime>
#include <list>

namespace
{
    class ResourceCompilerLogListener
        : public IResourceCompilerListener
    {
    public:
        ResourceCompilerLogListener(IExportContext* context)
            : m_context(context)
        {
        }

        virtual void OnRCMessage(IResourceCompilerListener::MessageSeverity severity, const char* text)
        {
            ILogger::ESeverity outSeverity;
            switch (severity)
            {
            case IResourceCompilerListener::MessageSeverity_Debug:
            case IResourceCompilerListener::MessageSeverity_Info:   // normal RC text should just be debug
                outSeverity = ILogger::eSeverity_Debug;
                break;
            case IResourceCompilerListener::MessageSeverity_Warning:
                outSeverity = ILogger::eSeverity_Warning;
                break;
            case IResourceCompilerListener::MessageSeverity_Error:
                outSeverity = ILogger::eSeverity_Error;
                break;
            default:
                outSeverity = ILogger::eSeverity_Error;
                break;
            }

            m_context->Log(outSeverity, "%s", text);
        }

    private:
        IExportContext* m_context;
    };
}

void ColladaExportWriter::Export(IExportSource* source, IExportContext* context)
{
    // Create an object to report on our progress to the export context.
    ProgressRange progressRange(context, &IExportContext::SetProgress);
    CResourceCompilerHelper compiler; // we need a real instance of this specific implementation.

    // Log build information.
    context->Log(ILogger::eSeverity_Info, "Exporter build created on " __DATE__);

#ifdef STLPORT
    context->Log(ILogger::eSeverity_Info, "Using STLport C++ Standard Library implementation");
#else //STLPORT
    context->Log(ILogger::eSeverity_Info, "Using Microsoft (tm) C++ Standard Library implementation");
#endif //STLPORT

#if defined(_DEBUG)
    context->Log(ILogger::eSeverity_Info, "******DEBUG BUILD******");
#else //_DEBUG
    context->Log(ILogger::eSeverity_Info, "Release build.");
#endif //_DEBUG

    context->Log(ILogger::eSeverity_Debug, "Bit count == %d.", (sizeof(void*) * 8));

    std::string exePath = StringHelpers::ConvertString<string>(ModuleHelpers::GetCurrentModulePath(ModuleHelpers::CurrentModuleSpecifier_Executable));
    context->Log(ILogger::eSeverity_Debug, "Application path: %s", exePath.c_str());

    std::string exporterPath = StringHelpers::ConvertString<string>(ModuleHelpers::GetCurrentModulePath(ModuleHelpers::CurrentModuleSpecifier_Library));
    context->Log(ILogger::eSeverity_Debug, "Exporter path: %s", exporterPath.c_str());

    bool const bExportCompressed = (GetSetting<int>(context->GetSettings(), "ExportCompressedCOLLADA", 1)) != 0;
    context->Log(ILogger::eSeverity_Debug, "ExportCompressedCOLLADA key: %d", (bExportCompressed ? 1 : 0));

    std::string const exportExtension = bExportCompressed ? ".dae.zip" : ".dae";

    // Log the start time.
    {
        char buf[1024];
        std::time_t t = std::time(0);
        std::strftime(buf, sizeof(buf) / sizeof(buf[0]), "%H:%M:%S on %a, %d/%m/%Y", std::localtime(&t));
        context->Log(ILogger::eSeverity_Info, "Export begun at %s", buf);
    }

    // Select the name of the directory to export to.
    std::string const originalExportDirectory = source->GetExportDirectory();
    if (originalExportDirectory.empty())
    {
        throw IExportContext::NeedSaveError("Scene must be saved before exporting.");
    }

    GeometryFileData geometryFileData;
    std::vector<std::string> colladaGeometryFileNameList;
    std::vector<std::string> assetGeometryFileNameList;
    typedef std::vector<std::pair<std::pair<int, int>, std::string> > AnimationFileNameList;
    AnimationFileNameList animationFileNameList;
    AnimationFileNameList animationCompileFileNameList;
    {
        CurrentTaskScope currentTask(context, "dae");

        // Choose the files to which to export all the animations.
        std::list<SingleAnimationExportSourceAdapter> animationExportSources;
        std::list<GeometryExportSourceAdapter> geometryExportSources;
        typedef std::vector<std::pair<std::string, IExportSource*> > ExportList;
        ExportList exportList;
        std::vector<int> geometryFileIndices;
        {
            ProgressRange readProgressRange(progressRange, 0.2f);

            source->ReadGeometryFiles(context, &geometryFileData);

            for (int geometryFileIndex = 0; geometryFileIndex < geometryFileData.GetGeometryFileCount(); ++geometryFileIndex)
            {
                const std::string geometryFileName = geometryFileData.GetGeometryFileName(geometryFileIndex);

                IGeometryFileData::SProperties properties = geometryFileData.GetProperties(geometryFileIndex);
                if (properties.filetypeInt == CRY_FILE_TYPE_CAF)
                {
                    // LDS: This is a temporary fix for some old hacky code that would activate a deprecated compression path during export
                    //      It needs a proper fix by tearing out the old compression code and moving the system to the new i_caf system by default.
                    //      See for http://docs.cryengine.com/display/SDKDOC3/Transition+from+CBA+to+AnimSettings details.
                    properties.filetypeInt = CRY_FILE_TYPE_INTERMEDIATE_CAF;
                    geometryFileData.SetProperties(geometryFileIndex, properties);
                }

                bool const hasGeometry = (properties.filetypeInt != CRY_FILE_TYPE_CAF &&
                                          properties.filetypeInt != CRY_FILE_TYPE_INTERMEDIATE_CAF);

                if (hasGeometry && !geometryFileName.empty())
                {
                    geometryFileIndices.push_back(geometryFileIndex);
                }
            }

            if (!geometryFileIndices.empty())
            {
                std::string name = PathHelpers::RemoveExtension(PathHelpers::GetFilename(source->GetDCCFileName()));
                std::replace(name.begin(), name.end(), ' ', '_');
                std::string const colladaPath = PathHelpers::Join(originalExportDirectory, name + exportExtension);
                colladaGeometryFileNameList.push_back(colladaPath);

                geometryExportSources.push_back(GeometryExportSourceAdapter(source, &geometryFileData, geometryFileIndices));
                exportList.push_back(std::make_pair(colladaPath, &geometryExportSources.back()));
            }

            for (int geometryFileIndex = 0; geometryFileIndex < geometryFileData.GetGeometryFileCount(); ++geometryFileIndex)
            {
                std::string const geometryFileName = geometryFileData.GetGeometryFileName(geometryFileIndex);
                int const fileTypeInt = geometryFileData.GetProperties(geometryFileIndex).filetypeInt;
                std::string customExportPath = geometryFileData.GetProperties(geometryFileIndex).customExportPath;
                bool const hasGeometry = (fileTypeInt != CRY_FILE_TYPE_CAF &&
                                          fileTypeInt != CRY_FILE_TYPE_INTERMEDIATE_CAF);

                if (hasGeometry && !geometryFileName.empty())
                {
                    std::string extension = "missingextension";
                    if (fileTypeInt == CRY_FILE_TYPE_CGF)
                    {
                        extension = "cgf";
                    }
                    else if ((fileTypeInt == CRY_FILE_TYPE_CGA) || (fileTypeInt == (CRY_FILE_TYPE_CGA | CRY_FILE_TYPE_ANM)))
                    {
                        extension = "cga";
                    }
                    else if (fileTypeInt == CRY_FILE_TYPE_ANM)
                    {
                        extension = "anm";
                    }
                    else if (fileTypeInt == CRY_FILE_TYPE_CHR ||
                             (fileTypeInt == (CRY_FILE_TYPE_CHR | CRY_FILE_TYPE_CAF)) ||
                             (fileTypeInt == (CRY_FILE_TYPE_CHR | CRY_FILE_TYPE_INTERMEDIATE_CAF)))
                    {
                        extension = "chr";
                    }
                    else if (fileTypeInt == CRY_FILE_TYPE_SKIN)
                    {
                        extension = "skin";
                    }

                    std::string safeGeometryFileName = geometryFileName;
                    std::replace(safeGeometryFileName.begin(), safeGeometryFileName.end(), ' ', '_');
                    std::string finalFileName;
                    if (customExportPath.size() > 0)
                    {
                        if (PathHelpers::IsRelative(customExportPath))
                        {
                            std::string const assetRelativePath = PathHelpers::Join(originalExportDirectory, customExportPath);
                            finalFileName = PathHelpers::Join(assetRelativePath, safeGeometryFileName + "." + extension);
                        }
                        else
                        {
                            context->Log(ILogger::eSeverity_Warning, "An absolute path was specified for export of node %s (%s) - This is unlikely to be correct", geometryFileName.c_str(), customExportPath.c_str());
                            finalFileName = PathHelpers::Join(customExportPath, safeGeometryFileName + "." + extension);
                        }
                    }
                    else
                    {
                        // no relative path, just export it in the original directory.
                        finalFileName = PathHelpers::Join(originalExportDirectory, safeGeometryFileName + "." + extension);
                    }
                    if (finalFileName.size() > 0)
                    {
                        assetGeometryFileNameList.push_back(finalFileName);
                        if (!FileUtil::EnsureDirectoryExists(PathHelpers::GetDirectory(finalFileName).c_str()))
                        {
                            context->Log(ILogger::eSeverity_Error, "Unable to create directory for %s", finalFileName.c_str());
                            return;
                        }
                    }
                }

                if ((fileTypeInt & (CRY_FILE_TYPE_CAF | CRY_FILE_TYPE_INTERMEDIATE_CAF)) != 0)
                {
                    for (int animationIndex = 0; animationIndex < source->GetAnimationCount(); ++animationIndex)
                    {
                        std::string const animationName = source->GetAnimationName(&geometryFileData, geometryFileIndex, animationIndex);

                        // Animations beginning with an underscore should be ignored.
                        bool const ignoreAnimation = animationName.empty() || (animationName[0] == '_');

                        if (!ignoreAnimation)
                        {
                            std::string safeAnimationName = animationName;
                            std::replace(safeAnimationName.begin(), safeAnimationName.end(), ' ', '_');

                            std::string exportPath = PathHelpers::Join(originalExportDirectory, safeAnimationName + exportExtension);
                            animationFileNameList.push_back(std::make_pair(std::make_pair(animationIndex, geometryFileIndex), exportPath));
                            if (fileTypeInt & CRY_FILE_TYPE_CAF)
                            {
                                animationCompileFileNameList.push_back(std::make_pair(std::make_pair(animationIndex, geometryFileIndex), exportPath));
                            }
                            animationExportSources.push_back(SingleAnimationExportSourceAdapter(source, &geometryFileData, geometryFileIndex, animationIndex));
                            exportList.push_back(std::make_pair(exportPath, &animationExportSources.back()));
                        }
                    }
                }
            }
        }

        // Export the COLLADA file to the chosen file.
        {
            ProgressRange exportProgressRange(progressRange, 0.6f);

            size_t const daeCount = exportList.size();
            float const daeProgressRangeSlice = 1.0f / (daeCount > 0 ? daeCount : 1);
            for (ExportList::iterator itFile = exportList.begin(); itFile != exportList.end(); ++itFile)
            {
                const std::string& colladaFileName = (*itFile).first;
                IExportSource* fileExportSource = (*itFile).second;

                ProgressRange animationExportProgressRange(exportProgressRange, daeProgressRangeSlice);

                try
                {
                    context->Log(ILogger::eSeverity_Info, "Exporting to file '%s'", colladaFileName.c_str());

                    // Try to create the directory for the file.
                    if (!FileUtil::EnsureDirectoryExists(PathHelpers::GetDirectory(colladaFileName).c_str()))
                    {
                        context->Log(ILogger::eSeverity_Error, "Unable to create directory for %s", colladaFileName.c_str());
                        return;
                    }

                    bool ok;

                    if (bExportCompressed)
                    {
                        IPakSystem* pakSystem = (context ? context->GetPakSystem() : 0);
                        if (!pakSystem)
                        {
                            throw IExportContext::PakSystemError("No pak system provided.");
                        }

                        std::string const archivePath = colladaFileName;
                        std::string archiveRelativePath = colladaFileName.substr(0, colladaFileName.length() - exportExtension.length()) + ".dae";
                        archiveRelativePath = PathHelpers::GetFilename(archiveRelativePath);

                        XMLPakFileSink sink(pakSystem, archivePath, archiveRelativePath);
                        ok = ColladaWriter::Write(fileExportSource, context, &sink, animationExportProgressRange);
                    }
                    else
                    {
                        XMLFileSink fileSink(colladaFileName);
                        ok = ColladaWriter::Write(fileExportSource, context, &fileSink, animationExportProgressRange);
                    }

                    if (!ok)
                    {
                        // FIXME: erase the resulting file somehow
                        context->Log(ILogger::eSeverity_Error, "Failed to export '%s'", colladaFileName.c_str());
                        return;
                    }
                }
                catch (IXMLSink::OpenFailedError e)
                {
                    context->Log(ILogger::eSeverity_Error, "Unable to open output file: %s", e.what());
                    return;
                }
                catch (...)
                {
                    context->Log(ILogger::eSeverity_Error, "Unexpected crash in COLLADA exporter");
                    return;
                }
            }
        }
    }

    // Get the RC path. If a custom one isn't specified then fall back to the registry method as per the default.
    wchar_t resourceCompilerPath[512];
    {
        const std::string resourceCompilerPathString = source->GetResourceCompilerPath();
        if (!resourceCompilerPathString.empty())
        {
            SettingsManagerHelpers::ConvertUtf8ToUtf16(resourceCompilerPathString.c_str(), SettingsManagerHelpers::CWCharBuffer(resourceCompilerPath, sizeof(resourceCompilerPath)));
        }
    }

    // Run the resource compiler on the COLLADA file to generate uncompressed CAFs.
    {
        ProgressRange compilerProgressRange(progressRange, 0.075f);

        CurrentTaskScope currentTask(context, "rc");

        size_t const daeCount = animationFileNameList.size();
        float const animationProgressRangeSlice = 1.0f / (daeCount > 0 ? daeCount : 1);
        for (AnimationFileNameList::iterator itFile = animationFileNameList.begin(); itFile != animationFileNameList.end(); ++itFile)
        {
            std::string colladaFileName = (*itFile).second;
            int geometryFileIndex = itFile->first.second;

            std::string expectedCAFPath;
            {
                bool isIntermediateCAF = (geometryFileData.GetProperties(geometryFileIndex).filetypeInt & CRY_FILE_TYPE_INTERMEDIATE_CAF) != 0;
                string nameWithoutExtension = colladaFileName.substr(0, colladaFileName.length() - exportExtension.length());
                expectedCAFPath = nameWithoutExtension + (isIntermediateCAF ? ".i_caf" : ".caf");
            }

            if (FileUtil::FileExists(expectedCAFPath.c_str()))
            {
                if (!DeleteFileA(expectedCAFPath.c_str()))
                {
                    context->Log(ILogger::eSeverity_Error, "Failed to remove existing animation file: %s", expectedCAFPath.c_str());
                    continue;
                }
            }

            string arguments = "/refresh";

            ProgressRange animationCompileProgressRange(compilerProgressRange, animationProgressRangeSlice);
            ResourceCompilerLogListener listener(context);

            context->Log(ILogger::eSeverity_Info, "Calling RC to generate uncompressed CAF file: %s", colladaFileName.c_str());
            CResourceCompilerHelper::ERcCallResult result = compiler.CallResourceCompiler( // actual instance of compiler used
                    colladaFileName.c_str(),
                    arguments.c_str(),
                    &listener,
                    true, false, false, 0, resourceCompilerPath);

            if (result != CResourceCompilerHelper::eRcCallResult_success)
            {
                context->Log(ILogger::eSeverity_Error, "%s", compiler.GetCallResultDescription(result));
                continue;
            }

            context->Log(ILogger::eSeverity_Debug, "RC finished: %s", colladaFileName.c_str());
            if (!FileUtil::FileExists(expectedCAFPath.c_str()))
            {
                context->Log(ILogger::eSeverity_Error, "Following Animation file is expected to be created by RC: %s", expectedCAFPath.c_str());
                context->Log(ILogger::eSeverity_Error, "Do you have an old RC version?");
            }

#if !defined(_DEBUG)
            // Delete the Collada file.
            DeleteFileA(colladaFileName.c_str());
#endif
        }
    }

    // Run the resource compiler on the COLLADA file to generate the geometry assets.
    {
        ProgressRange compilerProgressRange(progressRange, 0.075f);

        CurrentTaskScope currentTask(context, "rc");

        size_t const daeCount = colladaGeometryFileNameList.size();
        float const assetProgressRangeSlice = 1.0f / (daeCount > 0 ? daeCount : 1);
        for (size_t i = 0; i < daeCount; ++i)
        {
            const std::string& colladaFileName = colladaGeometryFileNameList[i];

            ProgressRange assetCompileProgressRange(compilerProgressRange, assetProgressRangeSlice);
            ResourceCompilerLogListener listener(context);
            context->Log(ILogger::eSeverity_Info, "Calling RC to generate raw asset file: %s", colladaFileName.c_str());
            CResourceCompilerHelper::ERcCallResult result = compiler.CallResourceCompiler(
                    colladaFileName.c_str(),
                    "/refresh",
                    &listener,
                    true, false, false, 0, resourceCompilerPath);

#if !defined(_DEBUG)
            // Delete the Collada file.
            DeleteFileA(colladaFileName.c_str());
#endif

            if (result == CResourceCompilerHelper::eRcCallResult_success)
            {
                context->Log(ILogger::eSeverity_Debug, "RC finished: %s", colladaFileName.c_str());
            }
            else
            {
                context->Log(ILogger::eSeverity_Error, "%s", compiler.GetCallResultDescription(result));
                return;
            }
        }
    }

    {
        // Create an RC helper - do it outside the loop, since it queries the registry on construction.
        ResourceCompilerLogListener listener(context);

        // Check the registry to see whether we should compress the animations or not.
        int processAnimations = GetSetting<int>(context->GetSettings(), "CompressCAFs", 1);

        if (!processAnimations)
        {
            context->Log(ILogger::eSeverity_Warning, "CompressCAFs registry key set to 0 - not compressing CAFs");
        }
        else
        {
            // Run the resource compiler again on the generated CAF files to compress/process them.
            context->Log(ILogger::eSeverity_Debug, "CompressCAFs not set or set to 1 - compressing CAFs");

            CurrentTaskScope currentTask(context, "compress");
            ProgressRange compressRange(progressRange, 0.025f);

            size_t const cafCount = animationCompileFileNameList.size();
            float const animationProgressRangeSlice = 1.0f / (cafCount > 0 ? cafCount : 1);
            for (AnimationFileNameList::iterator itFile = animationCompileFileNameList.begin(); itFile != animationCompileFileNameList.end(); ++itFile)
            {
                std::string colladaFileName = (*itFile).second;
                ProgressRange animationProgressRange(compressRange, animationProgressRangeSlice);

                // Assume the RC generated the CAF file using the take name and adding .CAF.
                std::string cafPath = colladaFileName.substr(0, colladaFileName.length() - exportExtension.length()) + ".caf";
                std::string cbaPath = StringHelpers::ConvertString<string>(CBAHelpers::FindCBAFileForFile(cafPath.c_str(), context->GetPakSystem()));

                if (cbaPath.empty())
                {
                    context->Log(ILogger::eSeverity_Error, "Unable to find CBA file for file \"%s\" (looked for a root game directory that contains a relative path of \"Animations/Animations.cba\"", cafPath.c_str());
                }
                else
                {
                    char buffer[2048];
                    sprintf(buffer, "/file=\"%s\" /refresh /SkipDba", cafPath.c_str());
                    context->Log(ILogger::eSeverity_Info, "Calling RC to compress CAF file: (CBA file = %s) %s", cbaPath.c_str(), buffer);
                    CResourceCompilerHelper::ERcCallResult result = compiler.CallResourceCompiler(cbaPath.c_str(), buffer, &listener, true, resourceCompilerPathType, false, false, 0, resourceCompilerPath);
                    if (result == CResourceCompilerHelper::eRcCallResult_success)
                    {
                        context->Log(ILogger::eSeverity_Debug, "RC finished: %s %s", cbaPath.c_str(), buffer);
                    }
                    else
                    {
                        context->Log(ILogger::eSeverity_Error, "%s", compiler.GetCallResultDescription(result));
                        return;
                    }
                }
            }
        }

        // Check the registry to see whether we should optimize the geometry files or not.
        int optimizeGeometry = GetSetting<int>(context->GetSettings(), "OptimizeAssets", 1);

        // Run the resource compiler again on the generated geometry files to compress/process them.
        // TODO: This should not be necessary, the RC should be modified so that assets are automatically
        // compressed when exported from COLLADA.
        if (!optimizeGeometry)
        {
            context->Log(ILogger::eSeverity_Warning, "OptimizeAssets registry key set to 0 - not compressing CAFs");
        }
        else
        {
            context->Log(ILogger::eSeverity_Debug, "OptimizeAssets not set or set to 1 - optimizing geometry");

            CurrentTaskScope currentTask(context, "compress");
            ProgressRange compressRange(progressRange, 0.025f);

            size_t const assetCount = assetGeometryFileNameList.size();
            float const assetProgressRangeSlice = 1.0f / (assetCount > 0 ? assetCount : 1);
            for (size_t i = 0; i < assetCount; ++i)
            {
                const std::string& assetFileName = assetGeometryFileNameList[i];
                ProgressRange animationProgressRange(compressRange, assetProgressRangeSlice);

                // note: we skip some asset types because we know that they are "optimized" already
                if (StringHelpers::EndsWithIgnoreCase(assetFileName, ".anm") || StringHelpers::EndsWithIgnoreCase(assetFileName, ".chr") || StringHelpers::EndsWithIgnoreCase(assetFileName, ".skin"))
                {
                    context->Log(ILogger::eSeverity_Info, "Calling RC to optimize asset \"%s\"", assetFileName.c_str());
                    CResourceCompilerHelper::ERcCallResult result = compiler.CallResourceCompiler(assetFileName.c_str(), "/refresh", &listener, true, resourceCompilerPathType, false, false, 0, resourceCompilerPath);
                    if (result == CResourceCompilerHelper::eRcCallResult_success)
                    {
                        context->Log(ILogger::eSeverity_Debug, "RC finished: %s", assetFileName.c_str());
                    }
                    else
                    {
                        context->Log(ILogger::eSeverity_Error, "%s", compiler.GetCallResultDescription(result));
                        return;
                    }
                }
            }
        }
    }

    // Log the end time.
    {
        char buf[1024];
        std::time_t t = std::time(0);
        std::strftime(buf, sizeof(buf) / sizeof(buf[0]), "%H:%M:%S on %a, %d/%m/%Y", std::localtime(&t));
        context->Log(ILogger::eSeverity_Info, "Export finished at %s", buf);
    }
}
