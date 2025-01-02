#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "recording.h"
#include "../xdfwriter/xdfwriter.h"
#include <lsl_cpp.h>

std::unordered_map<recording*, std::shared_ptr<recording>> recording_map;
std::mutex map_mutex;

extern "C" {
    recording* recording_create(const char* filename, const char** watchfor) 
    {
        std::vector<std::string> watchfor_vec;
        if (watchfor && *watchfor) {
            while (*watchfor) {
                watchfor_vec.emplace_back(*watchfor);
                watchfor++;
            }
        }


        // Create a shared_ptr to manage the recording instance
        std::shared_ptr<recording> local_recording = std::make_shared<recording>(
            std::string(filename), 
            std::vector<lsl::stream_info>{},
            watchfor_vec,
            std::map<std::string, int>{},
            true
        );

        std::cout << "Recording XDF to: " << filename << std::endl;

        // Store the shared_ptr in the global map to keep the recording alive
        {
            std::lock_guard<std::mutex> lock(map_mutex);
            recording_map[local_recording.get()] = local_recording;
        }

        std::thread record_thread([local_recording]() {
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (local_recording.use_count() == 1) {
                    break;
                }
            }
        });

        record_thread.detach();

        return local_recording.get();
    }

    void recording_delete(recording* instance) 
    {
        std::cout << "Deleting recording instance..." << std::endl;
        std::lock_guard<std::mutex> lock(map_mutex);
        auto it = recording_map.find(instance);

        if (it != recording_map.end()) {
            // Erase the shared_ptr from the map, allowing it to be destroyed if no other references exist
            recording_map.erase(it);
        } else {
            std::cout << "Recording instance not found in map." << std::endl;
        }
        // Note: Do not manually delete the instance as it's managed by shared_ptr
    }

    void recording_stop(recording* instance) 
    {
        std::cout << "Stopping recording instance..." << std::endl;
        if (instance) {
            instance->requestStop();
        } else {
            std::cout << "Invalid recording instance pointer." << std::endl;
        }
    }
}