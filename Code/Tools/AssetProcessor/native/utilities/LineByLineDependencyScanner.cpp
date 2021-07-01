/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LineByLineDependencyScanner.h"
#include "assetprocessor.h"
#include "PotentialDependencies.h"

namespace AssetProcessor
{
    class RegexComplexityAssertAbsorber
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        RegexComplexityAssertAbsorber()
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        virtual ~RegexComplexityAssertAbsorber()
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        bool OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message) override
        {
            // Ignore the regex complexity assert, there's no reason for the asset processor to crash when running a complex regex.
            const char* complexityError = AZStd::Internal::RegexError(AZStd::regex_constants::error_complexity);
            if (strncmp(message, complexityError, strlen(complexityError)) == 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    };

    LineByLineDependencyScanner::SearchResult GlobalSearch(const AZStd::string& scanString, int maxScanIteration, const AZStd::regex& regex, AZStd::function<void(const AZStd::smatch&)> callback)
    {
        AZStd::smatch result;
        AZStd::string::const_iterator searchStart = scanString.begin();

        bool useScanTimeout = maxScanIteration > 0;

        // Some binary files can cause the regex system to emit an assert that they are too complex to scan.
        // There's no harm in this case failing here, so ignore that assert if it occurs.
        RegexComplexityAssertAbsorber assertAbsorber;

        while (AZStd::regex_search(searchStart, scanString.end(), result, regex) && (!useScanTimeout || maxScanIteration > 0))
        {
            callback(result);
            searchStart = result[0].second;
            --maxScanIteration;
        }
        if (useScanTimeout && maxScanIteration == 0)
        {
            return LineByLineDependencyScanner::SearchResult::ScanLimitHit;
        }
        return LineByLineDependencyScanner::SearchResult::Completed;
    }

    LineByLineDependencyScanner::SearchResult LineByLineDependencyScanner::ScanStringForMissingDependencies(
        const AZStd::string& scanString,
        int maxScanIteration,
        const AZStd::regex& subIdRegex,
        const AZStd::regex& uuidRegex,
        const AZStd::regex& pathRegex,
        PotentialDependencies& potentialDependencies)
    {
        SearchResult assetIdSearchResult = GlobalSearch(scanString, maxScanIteration, subIdRegex, [this, &potentialDependencies](const AZStd::smatch& assetIdMatchResult)
        {
            AZ::Uuid uuid(assetIdMatchResult[1].str().c_str());
            AZ::u32 subId = AZStd::stoi(assetIdMatchResult[3].str());
            AZ::Data::AssetId assetId(uuid, subId);
            AZStd::string assetIdAsInFile(AZStd::string::format("%s%s%s",
                assetIdMatchResult[1].str().c_str(),
                assetIdMatchResult[2].str().c_str(),
                assetIdMatchResult[3].str().c_str()));
            // If one asset ID appears multiple times, only report it once to avoid too much repetitive output.
            PotentialDependencyMetaData dependencyMetaData(assetIdAsInFile, shared_from_this());
            potentialDependencies.m_assetIds[assetId] = dependencyMetaData;
        });

        SearchResult uuidSearchResult = GlobalSearch(scanString, maxScanIteration, uuidRegex, [this, &potentialDependencies, &subIdRegex](const AZStd::smatch& uuidMatchResult)
        {
            AZStd::string uuidStr = uuidMatchResult[1].str();
            AZ::Uuid uuid(uuidStr.c_str());
            // If one UUID appears multiple times, only report it once to avoid too much repetitive output.
            AZStd::smatch assetIdResult;
            const AZStd::string original{ uuidMatchResult.m_original };
            if (AZStd::regex_search(original.begin(), original.end(), assetIdResult, subIdRegex)) // check to see if this UUID is part of it a full asset id (which would be caught above)
            {
                if (assetIdResult.position(0) > uuidMatchResult.position(0))
                {
                    PotentialDependencyMetaData dependencyMetaData(uuidStr, shared_from_this());
                    potentialDependencies.m_uuids[uuid] = dependencyMetaData;
                }
            }
            else
            {
                PotentialDependencyMetaData dependencyMetaData(uuidStr, shared_from_this());
                potentialDependencies.m_uuids[uuid] = dependencyMetaData;
            }
        });

        // We'll first break up the input string into blocks that *could* contain a path.  This is a faster and simpler regex test
        // For each block, we'll do a quick string check to see if it contains a path separator or a file extension (.)
        // Only if we find one will we do the more expensive path regex check
        SearchResult pathSearchResult = GlobalSearch(scanString, maxScanIteration, AZStd::regex(R"~(([^:*?<>|" ]+))~"), [this, &maxScanIteration, &potentialDependencies, &pathRegex](const AZStd::smatch& matchResult)
        {
            AZStd::string stringSection = matchResult[1].str();
            if(stringSection.find('\\') != AZStd::string::npos || stringSection.find('/') != AZStd::string::npos || stringSection.find('.') != AZStd::string::npos)
            {
                return GlobalSearch(stringSection, maxScanIteration, pathRegex, [this, &potentialDependencies](const AZStd::smatch& pathMatchResult)
                {
                    AZStd::string potentialPath = pathMatchResult[1].str();
                    PotentialDependencyMetaData dependencyMetaData(potentialPath, shared_from_this());
                    potentialDependencies.m_paths.insert(dependencyMetaData);
                });
            }
            return SearchResult::Completed;
        });

        // If any scan did not complete, return that result.
        // There should only be one warning per file.
        if (assetIdSearchResult != SearchResult::Completed)
        {
            return assetIdSearchResult;
        }
        if (uuidSearchResult != SearchResult::Completed)
        {
            return uuidSearchResult;
        }
        if (pathSearchResult != SearchResult::Completed)
        {
            return pathSearchResult;
        }
        return SearchResult::Completed;
    }

    // A UUID is groups of hexadecimal digits, that may or may not be separated every 8, 4, 4, 4, 12 characters by a dash.
    AZStd::string GetUUIDRegex()
    {
        const char validUUIDVals[] = R"([\da-fA-F])";
        AZStd::string uuidSearchString = AZStd::string::format("\\b(%s{8}-?%s{4}-?%s{4}-?%s{4}-?%s{12})",
            validUUIDVals,
            validUUIDVals,
            validUUIDVals,
            validUUIDVals,
            validUUIDVals);
        return uuidSearchString;
    }

    bool LineByLineDependencyScanner::ScanFileForPotentialDependencies(
        AZ::IO::GenericStream& fileStream,
        PotentialDependencies& potentialDependencies,
        int maxScanIteration)
    {
        // An empty file will have no missing dependencies.
        AZ::IO::SizeType length = fileStream.GetLength();
        if (length == 0)
        {
            return true;
        }

        AZStd::vector<char> charBuffer;
        charBuffer.resize_no_construct(length + 1);
        fileStream.Read(length, charBuffer.data());
        charBuffer.back() = 0;

        // Search the file line by line. This won't catch cases where a missing
        // dependency uses data from multiple lines, but the regexes in use here also wouldn't catch that.
        AZStd::vector<AZStd::string> fileLines;
        AzFramework::StringFunc::Tokenize(charBuffer.data(), fileLines, "\r\n");

        AZStd::string uuidRegexStr(GetUUIDRegex());
        AZStd::regex uuidRegex(AZStd::string::format("%s(\\b)", uuidRegexStr.c_str()));
        // The sub ID may be immediately after the UUID, or there may be a character separating, like }.
        // There is a colon or dash character that separates the sub ID from the asset ID.
        // The sub ID may or may not be wrapped in braces of some kind, like [5] or {4}.
        // This will match things like:
        //      {A4844298-8495-4E2A-B587-C6E8ED9552AB}:5
        //      aaaaaaaa84954E2AB587C6E8ED9552AB-[5]
        AZStd::regex subIdRegex(AZStd::string::format(R"(%s(.?[-:][\{\(\[]?)(\d+))", uuidRegexStr.c_str()));

        // Don't use a greedy search, a given line may have multiple start/end quotes, find the smallest
        // thing that looks like a path. This search won't find things that look like paths without file extensions.
        AZStd::regex pathRegex(R"(([\w\\/-]*?\.[\w\d\.-]*))");
        int currentLineIndex = 1; // Most file editing software starts at line 1, not 0.
        for (const AZStd::string& line : fileLines)
        {
            SearchResult searchResult = ScanStringForMissingDependencies(
                line,
                maxScanIteration,
                subIdRegex,
                uuidRegex,
                pathRegex,
                potentialDependencies);

            switch (searchResult)
            {
            case SearchResult::ScanLimitHit:
                // This doesn't print the actual line in question out because it's likely a line complex enough to hit this limit isn't going to be print friendly.
                AZ_Printf(AssetProcessor::ConsoleChannel,
                    "\tFile will only be partially scanned, line %d matched more than the scan limit allows. To perform a more complete and lengthy scan, use the '--dependencyScanMaxIteration' setting.\n",
                    currentLineIndex);
                break;
            default:
                break;
            }

            ++currentLineIndex;
        }
        return true;
    }

    bool LineByLineDependencyScanner::DoesScannerMatchFileData(AZ::IO::GenericStream& /*fileStream*/)
    {
        // This scanner can handle any file.
        return true;
    }

    bool LineByLineDependencyScanner::DoesScannerMatchFileExtension(const AZStd::string& /*fullPath*/)
    {
        // This scanner can handle any file.
        return true;
    }

    AZStd::string LineByLineDependencyScanner::GetVersion() const
    {
        return "1.0.0";
    }

    AZStd::string LineByLineDependencyScanner::GetName() const
    {
        return "Line by line scanner";
    }

    AZ::Crc32 LineByLineDependencyScanner::GetScannerCRC() const
    {
        return AZ::Crc32(GetName().c_str());
    }
}
