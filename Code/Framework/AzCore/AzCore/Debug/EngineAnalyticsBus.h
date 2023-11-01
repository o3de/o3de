#pragma once

#if defined(CARBONATED)
// aefiov engine analytics bus to enable stats event from the engine, game's analytics processes those events

namespace AZ
{
    namespace Debug
    {
        /**
         * AnalyticsInterface declares basic analytics event that can be processed elsewhere
         */
        class EngineAnalyticsInterface
            : public AZ::EBusTraits
        {
        public:
            virtual ~EngineAnalyticsInterface() {}

            virtual bool Event(const AZStd::string& name, const AZStd::map<AZStd::string, AZStd::string>& map) = 0;
        };

        typedef AZ::EBus<EngineAnalyticsInterface> EngineAnalyticsBus;
    }
}

#endif // CARBONATED
