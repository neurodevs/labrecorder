#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <cstddef>

#include "recording.h"
#include "../xdfwriter/xdfwriter.h"
#include <lsl_cpp.h>

// Global registry of active recordings, keyed by their raw pointer.
// The raw pointer is the opaque handle returned to Node.js
// The unique_ptr owns the lifetime on the C++ side.
static std::unordered_map<recording*, std::unique_ptr<recording>> g_recordings;
static std::mutex g_recordings_mutex;

extern "C" {

    // Create a new recording.
    //
    // filename        path to the .xdf file
    // watchfor        null-terminated array of const char* predicates; last element must be nullptr
    // watchfor_count  number of entries in watchfor array
    //
    // Returns: opaque recording* handle, or nullptr on failure.
    recording* recording_create(const char* filename, 
                                const char** watchfor, 
                                std::size_t watchfor_count) {
        if (!filename || !*filename) {
            std::cerr << "[recording_create] Invalid filename.\n";
            return nullptr;
        }

        std::vector<lsl::stream_info> stream_vec;

        std::vector<std::string> watchfor_vec;
        
        if (watchfor && watchfor_count > 0) {
            watchfor_vec.reserve(watchfor_count);
            for (std::size_t i = 0; i < watchfor_count; ++i) {
                const char* s = watchfor[i];
                if (!s) {
                    std::cerr << "[recording_create] Null watchfor entry at index "
                            << i << "\n";
                    return nullptr;
                }
                watchfor_vec.emplace_back(s);
            }
        }

        std::map<std::string, int> sync_options; // currently empty; fill as needed

        std::unique_ptr<recording> rec;
        try {
            rec = std::make_unique<recording>(
                std::string(filename),
                stream_vec,
                watchfor_vec,
                sync_options,
                true  // collect_offsets
            );
        } catch (const std::exception& ex) {
            std::cerr << "[recording_create] Exception constructing recording: "
                      << ex.what() << "\n";
            return nullptr;
        } catch (...) {
            std::cerr << "[recording_create] Unknown exception constructing recording.\n";
            return nullptr;
        }

        recording* handle = rec.get();

        {
            std::lock_guard<std::mutex> lock(g_recordings_mutex);
            auto [it, inserted] = g_recordings.emplace(handle, std::move(rec));
            if (!inserted) {
                std::cerr << "[recording_create] Failed to insert recording into registry.\n";
                return nullptr;
            }
        }

        std::cout << "Recording created and registered.\n";
        return handle;
    }

    // Request a graceful stop of the recording.
    // The recording implementation should observe the shutdown flag and let
    // its internal threads exit, then flush and close in its destructor.
    void recording_stop(recording* instance) {
        if (!instance) {
            std::cerr << "[recording_stop] Null instance.\n";
            return;
        }

        std::lock_guard<std::mutex> lock(g_recordings_mutex);
        auto it = g_recordings.find(instance);
        if (it == g_recordings.end()) {
            std::cerr << "[recording_stop] Instance not found in registry.\n";
            return;
        }

        try {
            it->second->requestStop();
        } catch (const std::exception& ex) {
            std::cerr << "[recording_stop] Exception in requestStop(): "
                      << ex.what() << "\n";
        } catch (...) {
            std::cerr << "[recording_stop] Unknown exception in requestStop().\n";
        }

        std::cout << "Recording stopped.\n";
    }

    // Destroy a recording instance.
    // This removes it from the registry and lets the unique_ptr run the destructor.
    // The destructor is where you should join threads and close the XDF file.
    void recording_delete(recording* instance) {
        if (!instance) {
            std::cerr << "[recording_delete] Null instance.\n";
            return;
        }

        std::unique_ptr<recording> owned;

        {
            std::lock_guard<std::mutex> lock(g_recordings_mutex);
            auto it = g_recordings.find(instance);
            if (it == g_recordings.end()) {
                std::cerr << "[recording_delete] Instance not found in registry.\n";
                return;
            }
            owned = std::move(it->second);
            g_recordings.erase(it);
        }

        // owned goes out of scope here; ~recording() runs.
        // That should stop the recording, join any threads, and close the file.
        std::cout << "Recording destroyed.\n";
    }
}