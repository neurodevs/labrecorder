#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include "recording.h"
#include "../xdfwriter/xdfwriter.h"
#include <lsl_cpp.h>

recording* global_recording = nullptr;

extern "C" {
    recording* recording_create(const char* filename, 
                                const char** watchfor) 
    {

        std::vector<std::string> watchfor_vec;
        if (watchfor && *watchfor) {
            while (*watchfor) {
                watchfor_vec.emplace_back(*watchfor);
                watchfor++;
            }
        }

        std::cout << "Filename: " << filename << std::endl;
        std::cout << "Number of watchfor predicates: " << watchfor_vec.size() << std::endl;

        global_recording = new recording(
            std::string(filename), 
            {}, 
            watchfor_vec, 
            {}, 
            true
        );

        std::cout << "Recording started!" << std::endl;

        std::thread record_thread([]() {
            std::cout << "Background thread started." << std::endl;
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        record_thread.join();

        return global_recording;
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