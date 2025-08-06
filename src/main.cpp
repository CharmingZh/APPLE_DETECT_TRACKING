#include "ImageTracker.h"
#include "config/Configuration.h"
#include "SimpleSerial.h"
#include <iostream>
#include <vector>
#include <string>

#ifdef _WIN32
#include <windows.h>
void enable_virtual_terminal_processing() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) return;
}
#endif

int main(int argc, char* argv[]) {
    #ifdef _WIN32
        enable_virtual_terminal_processing();
    #endif

    SimpleSerial serial("COM3", 9600);
    if (!serial.isConnected()) {
        std::cerr << "Serial connection failed. Continuing without hardware control." << std::endl;
    }

    if constexpr (Config::USE_LIVE_CAMERA) {
        std::cout << "--- Starting in LIVE CAMERA mode ---" << std::endl;
        try {
            ImageTracker::Settings settings;
            ImageTracker tracker(settings);
            tracker.runFromCamera(&serial);
        } catch (const std::exception& e) {
            std::cerr << "[FATAL ERROR] in live camera mode: " << e.what() << std::endl;
        }
    } else {
        std::cout << "--- Starting in LOCAL DATASET mode ---" << std::endl;
        for (int index : Config::DATASET_INDICES_TO_RUN) {
            if (index >= Config::DATASETS_PATH.size()) {
                std::cerr << "[Warning] Index " << index << " is out of bounds. Skipping." << std::endl;
                continue;
            }
            std::string current_folder = Config::DATASETS_PATH[index];
            std::cout << "\n\n--- Processing Folder: " << current_folder << " ---" << std::endl;
            try {
                ImageTracker::Settings settings;
                settings.input_path = current_folder;
                ImageTracker tracker(settings);
                tracker.runFromDataset(&serial);
            } catch (const std::exception& e) {
                std::cerr << "[FATAL ERROR] in folder " << current_folder << ": " << e.what() << std::endl;
            }
        }
    }

    std::cout << "\n\n--- All processing finished. ---" << std::endl;
    if (!Config::USE_LIVE_CAMERA) {
        cv::waitKey(0);
    }
    return 0;
}