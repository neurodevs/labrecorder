#include "recording.h"
#include "../xdfwriter/xdfwriter.h"

extern "C" {
    recording* recording_create(const char* filename, 
                                const lsl::stream_info* streams, 
                                const char** watchfor, 
                                const std::map<std::string, int>* syncOptions,
                                bool collect_offsets = true) 
    {
        return new recording(
            std::string(filename), 
            std::vector<lsl::stream_info>(streams, streams + 1), 
            std::vector<std::string>(watchfor, watchfor + 1), 
            *syncOptions, 
            collect_offsets
        );
    }

    void recording_delete(recording* instance) 
    {
        delete instance;
    }

    void recording_stop(recording* instance) 
    {
        instance->requestStop();
    }
}