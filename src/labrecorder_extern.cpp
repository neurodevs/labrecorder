#include "recording.h"
#include "../xdfwriter/xdfwriter.h"

extern "C" {
    recording* recording_create(const std::string filename, 
                                const std::vector<lsl::stream_info> streams, 
                                const std::vector<std::string> watchfor, 
                                const std::map<std::string, int> syncOptions, 
                                bool collect_offsets = true) 
    {
        return new recording(filename, streams, watchfor, syncOptions, collect_offsets);
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