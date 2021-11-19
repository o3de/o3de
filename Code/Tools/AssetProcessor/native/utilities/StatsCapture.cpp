 /*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/utilities/StatsCapture.h>
#include <native/assetprocessor.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <inttypes.h>

namespace AssetProcessor
{
    namespace StatsCapture
    {
        // This class captures stats by storing them in a map of type
        // [name of stat] -> Stat struct
        // It can then analyze these stats and produce more stats from the original
        // Captures, before dumping.
        class StatsCaptureImpl final
        {
        public:
            AZ_CLASS_ALLOCATOR(StatsCaptureImpl, AZ::SystemAllocator, 0);

            void BeginCaptureStat(AZStd::string_view statName);
            void EndCaptureStat(AZStd::string_view statName);
            void Dump();
        private:
            using timepoint = AZStd::chrono::high_resolution_clock::time_point;
            using duration = AZStd::chrono::milliseconds;
            struct StatsEntry
            {
                duration m_cumulativeTime = {};    // The total amount of time spent on this.
                timepoint m_operationStartTime = {}; // Async tracking - the last time stamp an operation started.
                int64_t m_operationCount = 0; // In case there's more than one sample.  Used to calc average.
            };
            
            AZStd::unordered_map<AZStd::string, StatsEntry> m_stats;
            bool m_dumpMachineReadableStats = false;
            bool m_dumpHumanReadableStats = true;

            // Make a friendly time string of the format nnHnnMhhS.xxxms
            AZStd::string FormatDuration(const duration& duration)
            {
                int64_t milliseconds = duration.count();
                constexpr int64_t millisecondsInASecond = 1000;
                constexpr int64_t millisecondsInAMinute = millisecondsInASecond * 60;
                constexpr int64_t millisecondsInAnHour = millisecondsInAMinute * 60;
                
                int64_t hours = milliseconds / millisecondsInAnHour;
                milliseconds -= hours * millisecondsInAnHour;
                
                int64_t minutes = milliseconds / millisecondsInAMinute;
                milliseconds -= minutes * millisecondsInAMinute;
                
                int64_t seconds = milliseconds / millisecondsInASecond;
                milliseconds -= seconds * millisecondsInASecond;
                
                // omit the sections which dont make sense for readability
                if (hours)
                {
                    return AZStd::string::format("%02" PRId64 "h%02" PRId64 "m%02" PRId64 "s%03" PRId64 "ms" , hours, minutes, seconds, milliseconds);
                }
                else if (minutes)
                {
                    return AZStd::string::format("   %02" PRId64 "m%02" PRId64 "s%03" PRId64 "ms", minutes, seconds, milliseconds);
                }
                else if (seconds)
                {
                    return AZStd::string::format("      %02" PRId64 "s%03" PRId64 "ms", seconds, milliseconds);
                }
                
                return AZStd::string::format("         %03" PRId64 "ms", milliseconds);
            }

            // Prints out a single stat. 
            void PrintStat([[maybe_unused]] const char* name, duration milliseconds, int64_t count)
            {
                // note that name may be unused as it only appears in Trace macros, which are
                // stripped out in release builds.
                if (count <= 1)
                {
                    count = 1;
                }

                duration average(static_cast<int64_t>(static_cast<double>(milliseconds.count()) / static_cast<double>(count)));

                if (m_dumpHumanReadableStats)
                {
                    if (count > 1)
                    {
                        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "    Time: %s, Count: %4" PRId64 ", Average: %s, EventName: %s\n",
                            FormatDuration(milliseconds).c_str(),
                            count,
                            FormatDuration(average).c_str(),
                            name);
                    }
                    else
                    {
                        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "    Time: %s, EventName: %s\n",
                            FormatDuration(milliseconds).c_str(),
                            name);
                    }
                }
                if (m_dumpMachineReadableStats)
                {
                    // machine Readable mode prints raw milliseconds and uses a CSV-like format
                    // note that the stat itself may contain commas, so we dont acutally separate with comma
                    // instead we separate with :
                    // and each "interesting line" is 'MachineReadableStat:milliseconds:count:average:name'
                    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "MachineReadableStat:%" PRId64 ":%" PRId64 ":%" PRId64 ":%s\n",
                            milliseconds.count(),
                            count,
                            count > 1 ? average.count() : milliseconds.count(),
                            name);
                }
            }
            
            // calls PrintStat on each element in the vector.
            void PrintStatsArray(AZStd::vector<AZStd::string>& keys, int maxToPrint, const char* header)
            {
                if ((m_dumpHumanReadableStats)&&(header))
                {
                    AZ_TracePrintf(AssetProcessor::ConsoleChannel,"Top %i %s\n", maxToPrint, header);
                }
                   
                auto sortByTimeDescending = [&](const AZStd::string& s1, const AZStd::string& s2)
                {
                    return this->m_stats[s1].m_cumulativeTime > this->m_stats[s2].m_cumulativeTime;
                };
        
                AZStd::sort(keys.begin(), keys.end(), sortByTimeDescending);
                
                for (int idx = 0; idx < maxToPrint; ++idx)
                {
                    if (idx < keys.size())
                    {
                        PrintStat(keys[idx].c_str(), m_stats[keys[idx]].m_cumulativeTime, m_stats[keys[idx]].m_operationCount);
                    }
                }
            }
        };


        void StatsCaptureImpl::BeginCaptureStat(AZStd::string_view statName)
        {
            StatsEntry& existingStat = m_stats[statName];
            if (existingStat.m_operationStartTime != timepoint())
            {
                // prevent double 'Begins'
                return;
            }
            existingStat.m_operationStartTime = AZStd::chrono::high_resolution_clock::now();
        }

        void StatsCaptureImpl::EndCaptureStat(AZStd::string_view statName)
        {
            StatsEntry& existingStat = m_stats[statName];
            if (existingStat.m_operationStartTime != timepoint())
            {
                existingStat.m_cumulativeTime = AZStd::chrono::high_resolution_clock::now() - existingStat.m_operationStartTime;
                existingStat.m_operationCount = existingStat.m_operationCount + 1;
                existingStat.m_operationStartTime = timepoint(); // reset the start time so that double 'Ends' are ignored.
            }
        }

        void StatsCaptureImpl::Dump()
        {
            timepoint startTimeStamp = AZStd::chrono::high_resolution_clock::now();

            auto settingsRegistry = AZ::SettingsRegistry::Get();

            int maxCumulativeStats = 5; // default max cumulative stats to show
            int maxIndividualStats = 5; // default max individual files to show

            if (settingsRegistry)
            {
                AZ::u64 cumulativeStats = static_cast<AZ::u64>(maxCumulativeStats);
                AZ::u64 individualStats = static_cast<AZ::u64>(maxIndividualStats);
                settingsRegistry->Get(m_dumpHumanReadableStats,   "/Amazon/AssetProcessor/Settings/Stats/HumanReadable");
                settingsRegistry->Get(m_dumpMachineReadableStats, "/Amazon/AssetProcessor/Settings/Stats/MachineReadable");
                settingsRegistry->Get(cumulativeStats,          "/Amazon/AssetProcessor/Settings/Stats/MaxCumulativeStats");
                settingsRegistry->Get(individualStats,          "/Amazon/AssetProcessor/Settings/Stats/MaxIndividualStats");
                maxCumulativeStats = static_cast<int>(cumulativeStats);
                maxIndividualStats = static_cast<int>(individualStats);
            }

            if ((!m_dumpHumanReadableStats)&&(!m_dumpMachineReadableStats))
            {
                return;
            }

            AZStd::vector<AZStd::string> allCreateJobs; // individual
            AZStd::vector<AZStd::string> allCreateJobsByBuilder; // bucketed by builder
            AZStd::vector<AZStd::string> allProcessJobs;  // individual
            AZStd::vector<AZStd::string> allProcessJobsByPlatform; // bucketed by platform
            AZStd::vector<AZStd::string> allProcessJobsByJobKey; // bucketed by type of job (job key)
            AZStd::vector<AZStd::string> allHashFiles;

            // capture only existing keys as we will be expanding the stats
            // this approach avoids mutating an iterator.   
            AZStd::vector<AZStd::string> statKeys;
            for (const auto& element : m_stats)
            {
                statKeys.push_back(element.first);
            }

            for (const AZStd::string& statKey : statKeys)
            {
                const StatsEntry& statistic = m_stats[statKey];
                // Createjobs stats encode like (CreateJobs,sourcefilepath,builderid)
                if (AZ::StringFunc::StartsWith(statKey, "CreateJobs,", true))
                {
                    allCreateJobs.push_back(statKey);
                    AZStd::vector<AZStd::string> tokens;
                    AZ::StringFunc::Tokenize(statKey, tokens, ",", false, false);

                    // look up the builder so you can get its name:
                    AZStd::string_view builderName = tokens[2];
                
                    // synthesize a stat to track per-builder createjobs times:
                    {
                        AZStd::string newStatKey = AZStd::string::format("CreateJobsByBuilder,%.*s", AZ_STRING_ARG(builderName));

                        auto insertion = m_stats.insert(newStatKey);
                        StatsEntry& statToSynth = insertion.first->second;
                        statToSynth.m_cumulativeTime += statistic.m_cumulativeTime;
                        statToSynth.m_operationCount += statistic.m_operationCount;
                        if (insertion.second)
                        {
                            allCreateJobsByBuilder.push_back(newStatKey);
                        }
                    }
                    // synthesize a stat to track total createjobs times:
                    {
                        StatsEntry& statToSynth = m_stats["CreateJobsTotal"];
                        statToSynth.m_cumulativeTime += statistic.m_cumulativeTime;
                        statToSynth.m_operationCount += statistic.m_operationCount;
                    }
                }
                else if (AZ::StringFunc::StartsWith(statKey, "ProcessJob,", true))
                {
                    allProcessJobs.push_back(statKey);
                    // processjob has the format ProcessJob,sourcename,jobkey,platformname
                    AZStd::vector<AZStd::string> tokens;
                    AZ::StringFunc::Tokenize(statKey, tokens, ",", false, false);
                    AZStd::string_view jobKey = tokens[2];
                    AZStd::string_view platformName = tokens[3];

                    // synthesize a stat to record process time accumulated by job key platform
                    {
                        AZStd::string newStatKey = AZStd::string::format("ProcessJobsByPlatform,%.*s", AZ_STRING_ARG(platformName));
                        auto insertion = m_stats.insert(newStatKey);
                        StatsEntry& statToSynth = insertion.first->second;
                        statToSynth.m_cumulativeTime += statistic.m_cumulativeTime;
                        statToSynth.m_operationCount += statistic.m_operationCount;
                        if (insertion.second)
                        {
                            allProcessJobsByPlatform.push_back(newStatKey);
                        }
                    }

                    // synthesize a stat to record process time accumulated job key total across all platforms
                    {
                        AZStd::string newStatKey = AZStd::string::format("ProcessJobsByJobKey,%.*s", AZ_STRING_ARG(jobKey));
                        auto insertion = m_stats.insert(newStatKey);
                        StatsEntry& statToSynth = insertion.first->second;
                        statToSynth.m_cumulativeTime += statistic.m_cumulativeTime;
                        statToSynth.m_operationCount += statistic.m_operationCount;
                        if (insertion.second)
                        {
                            allProcessJobsByJobKey.push_back(newStatKey);
                        }
                    }
                    // synthesize a stat to track total processjob times:
                    {
                        StatsEntry& statToSynth = m_stats["ProcessJobsTotal"];
                        statToSynth.m_cumulativeTime += statistic.m_cumulativeTime;
                        statToSynth.m_operationCount += statistic.m_operationCount;
                    }
                }
                else if (AZ::StringFunc::StartsWith(statKey, "HashFile,", true))
                {
                    allHashFiles.push_back(statKey);
                    // processjob has the format ProcessJob,sourcename,jobkey,platformname
                    // synthesize a stat to track total hash times:
                    StatsEntry& statToSynth = m_stats["HashFileTotal"];
                    statToSynth.m_cumulativeTime += statistic.m_cumulativeTime;
                    statToSynth.m_operationCount += statistic.m_operationCount;
                }
            }
            
            StatsEntry& gemLoadStat = m_stats["LoadingModules"];
            PrintStat("LoadingGems", gemLoadStat.m_cumulativeTime, 1);
            // analysis-related stats

            StatsEntry& totalScanTime = m_stats["AssetScanning"];
            PrintStat("AssetScanning", totalScanTime.m_cumulativeTime, totalScanTime.m_operationCount);
            StatsEntry& totalHashTime = m_stats["HashFileTotal"];
            PrintStat("HashFileTotal", totalHashTime.m_cumulativeTime, totalHashTime.m_operationCount);
            PrintStatsArray(allHashFiles, maxIndividualStats, "longest individual file hashes:");
        
            // CreateJobs stats
            StatsEntry& totalCreateJobs = m_stats["CreateJobsTotal"];
            if (totalCreateJobs.m_operationCount)
            {
                PrintStat("CreateJobsTotal", totalCreateJobs.m_cumulativeTime, totalCreateJobs.m_operationCount);
                PrintStatsArray(allCreateJobs, maxIndividualStats, "longest individual CreateJobs");
                PrintStatsArray(allCreateJobsByBuilder, maxCumulativeStats, "longest CreateJobs By builder");
            }
            
            // ProcessJobs stats
            StatsEntry& totalProcessJobs = m_stats["ProcessJobsTotal"];
            if (totalProcessJobs.m_operationCount)
            {
                PrintStat("ProcessJobsTotal", totalProcessJobs.m_cumulativeTime, totalProcessJobs.m_operationCount);
                PrintStatsArray(allProcessJobs, maxIndividualStats, "longest individual ProcessJob");
                PrintStatsArray(allProcessJobsByJobKey, maxCumulativeStats, "cumulative time spent in ProcessJob by JobKey");
                PrintStatsArray(allProcessJobsByPlatform, maxCumulativeStats, "cumulative time spent in ProcessJob by Platform");
            }
            duration costToGenerateStats =  AZStd::chrono::high_resolution_clock::now() - startTimeStamp;
            PrintStat("ComputeStatsTime", costToGenerateStats, 1);
        }

        // Public interface:
        static StatsCaptureImpl* g_instance = nullptr;

        //! call this one time before capturing stats.
        void Initialize()
        {
            if (g_instance)
            {
                AZ_Assert(false, "An instance of StatsCaptureImpl already exists.");
                return;
            }
            g_instance = aznew StatsCaptureImpl();
        }

        //! Call this one time as part of shutting down.
        //! note that while it is an error to double-initialize, it is intentionally
        //! not an error to call any other function when uninitialized, allowing this system
        //! to essentially be "turned off" just by not initializing it in the first place.
        void Shutdown()
        {
            if (g_instance)
            {
                delete g_instance;
                g_instance = nullptr;
            }
        }

        //! Start the clock running for a particular stat name.
        void BeginCaptureStat(AZStd::string_view statName)
        {
            if (g_instance)
            {
                g_instance->BeginCaptureStat(statName);
            }
        }

        //! Stop the clock running for a particular stat name.
        void EndCaptureStat(AZStd::string_view statName)
        {
            if (g_instance)
            {
                g_instance->EndCaptureStat(statName);
            }
        }

        //! Do additional processing and then write the cumulative stats to log.
        //! Note that since this is an AP-specific system, the analysis done in the dump function
        //! is going to make a lot of assumptions about the way the data is encoded.
        void Dump()
        {
            if (g_instance)
            {
                g_instance->Dump();
            }
        }
    }
}
