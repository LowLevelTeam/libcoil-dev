#ifndef COIL_TEST_HELPER_H
#define COIL_TEST_HELPER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cassert>
#include <functional>
#include <chrono>
#include <map>
#include <cstdlib>
#include <stdexcept>

#include "coil/binary_format.h"
#include "coil/type_system.h"
#include "coil/instruction_set.h"
#include "coil/variable_system.h"
#include "coil/error_codes.h"
#include "coil/utils/validation.h"
#include "coil/utils/binary_utils.h"

// Global variables for debug tracking
static std::string lastErrorMessage = "";
static bool verboseTestOutput = false;

// Enhanced assertion macros with colored output and error tracking
#define TEST_ASSERT(condition) \
    if (!(condition)) { \
        std::cerr << "\033[1;31mAssertion failed:\033[0m " << #condition << std::endl; \
        std::cerr << "  at \033[1m" << __FILE__ << ":" << __LINE__ << "\033[0m" << std::endl; \
        if (!lastErrorMessage.empty()) { \
            std::cerr << "\033[1;33mLast error message:\033[0m " << lastErrorMessage << std::endl; \
        } \
        return false; \
    }

#define TEST_ASSERT_MSG(condition, message) \
    if (!(condition)) { \
        std::cerr << "\033[1;31mAssertion failed:\033[0m " << #condition << std::endl; \
        std::cerr << "  \033[1;33mDetails:\033[0m " << message << std::endl; \
        std::cerr << "  at \033[1m" << __FILE__ << ":" << __LINE__ << "\033[0m" << std::endl; \
        if (!lastErrorMessage.empty()) { \
            std::cerr << "\033[1;33mLast error message:\033[0m " << lastErrorMessage << std::endl; \
        } \
        return false; \
    }

// Validation helper macro that automatically reports errors
#define VALIDATE_AND_REPORT(validation_call, error_manager) \
    { \
        bool isValid = (validation_call); \
        if (!isValid) { \
            std::cerr << "\033[1;31mValidation failed with " << error_manager.getErrors().size() << " errors:\033[0m" << std::endl; \
            for (const auto& error : error_manager.getErrors()) { \
                std::cerr << "  - " << error.toString() << std::endl; \
                if (lastErrorMessage.empty()) { lastErrorMessage = error.toString(); } \
            } \
        } \
        TEST_ASSERT_MSG(isValid, "Validation failed"); \
    }

// Run multiple test cases and report failures with timing and colored output
inline int runTests(const std::vector<std::pair<std::string, std::function<bool()>>>& tests) {
    int failures = 0;
    int total = 0;
    
    // Check if verbose output is enabled via environment variable
    const char* verboseEnv = std::getenv("COIL_TEST_VERBOSE");
    if (verboseEnv && (std::string(verboseEnv) == "1" || std::string(verboseEnv) == "true")) {
        verboseTestOutput = true;
        std::cout << "\033[1;34m[INFO]\033[0m Verbose test output enabled" << std::endl;
    }
    
    std::cout << "\n\033[1;36m====== Running " << tests.size() << " Tests ======\033[0m\n" << std::endl;
    
    for (const auto& test : tests) {
        // Clear the last error message before each test
        lastErrorMessage = "";
        
        if (verboseTestOutput) {
            std::cout << "\033[1;34m[TEST START]\033[0m " << test.first << std::endl;
        } else {
            std::cout << "Running test: " << test.first << "... ";
            std::cout.flush(); // Ensure output appears immediately
        }
        
        // Capture start time
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Run the test
        bool passed = false;
        try {
            passed = test.second();
        } catch (const std::exception& e) {
            if (verboseTestOutput) {
                std::cerr << "\033[1;31m[EXCEPTION]\033[0m Test threw an exception: " << e.what() << std::endl;
            } else {
                std::cerr << "\nTest threw an exception: " << e.what() << std::endl;
            }
            passed = false;
        } catch (...) {
            if (verboseTestOutput) {
                std::cerr << "\033[1;31m[EXCEPTION]\033[0m Test threw an unknown exception" << std::endl;
            } else {
                std::cerr << "\nTest threw an unknown exception" << std::endl;
            }
            passed = false;
        }
        
        // Calculate test duration
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        if (passed) {
            if (verboseTestOutput) {
                std::cout << "\033[1;32m[TEST PASSED]\033[0m " << test.first 
                          << " (" << duration << "ms)" << std::endl;
            } else {
                std::cout << "\033[1;32mPASSED\033[0m (" << duration << "ms)" << std::endl;
            }
        } else {
            if (verboseTestOutput) {
                std::cout << "\033[1;31m[TEST FAILED]\033[0m " << test.first 
                          << " (" << duration << "ms)" << std::endl;
            } else {
                std::cout << "\033[1;31mFAILED\033[0m (" << duration << "ms)" << std::endl;
            }
            failures++;
        }
        
        if (verboseTestOutput) {
            std::cout << std::string(40, '-') << std::endl;
        }
        
        total++;
    }
    
    std::cout << "\n\033[1;36m====== Test Summary ======\033[0m" << std::endl;
    std::cout << "  Total tests: " << total << std::endl;
    std::cout << "  \033[1;32mPassed: " << (total - failures) << "\033[0m" << std::endl;
    if (failures > 0) {
        std::cout << "  \033[1;31mFailed: " << failures << "\033[0m" << std::endl;
    } else {
        std::cout << "  Failed: 0" << std::endl;
    }
    
    return failures;
}

// Enhanced binary data printing with annotation capability
inline void printBinaryData(const std::vector<uint8_t>& data, size_t start = 0, size_t count = (size_t)-1, 
                            std::map<size_t, std::string> annotations = {}) {
    if (count == (size_t)-1) count = data.size();
    count = std::min(count, data.size() - start);
    
    std::cout << "\033[1;33mBinary data (" << count << " bytes):\033[0m" << std::endl;
    for (size_t i = 0; i < count; i += 16) {
        // Print offset
        std::cout << "\033[1;36m" << std::setw(4) << std::setfill('0') << std::hex << i + start << ":\033[0m ";
        
        // Print hex values
        for (size_t j = 0; j < 16 && i + j < count; j++) {
            size_t currentOffset = i + j + start;
            
            // Highlight annotated bytes
            if (annotations.find(currentOffset) != annotations.end()) {
                std::cout << "\033[1;33m";
            }
            
            std::cout << std::setw(2) << std::setfill('0') << std::hex 
                      << static_cast<int>(data[currentOffset]) << " ";
            
            if (annotations.find(currentOffset) != annotations.end()) {
                std::cout << "\033[0m";
            }
            
            if (j == 7) std::cout << " "; // Extra space in the middle
        }
        
        // Fill remaining space if less than 16 bytes
        size_t remaining = (i + 16 <= count) ? 0 : 16 - (count - i);
        for (size_t j = 0; j < remaining; j++) {
            std::cout << "   ";
            if (j == 7) std::cout << " "; // Extra space in the middle
        }
        
        std::cout << "  ";
        
        // Print ASCII representation
        for (size_t j = 0; j < 16 && i + j < count; j++) {
            size_t currentOffset = i + j + start;
            char c = data[currentOffset];
            
            // Highlight annotated bytes
            if (annotations.find(currentOffset) != annotations.end()) {
                std::cout << "\033[1;33m";
            }
            
            std::cout << (isprint(c) ? c : '.');
            
            if (annotations.find(currentOffset) != annotations.end()) {
                std::cout << "\033[0m";
            }
        }
        
        std::cout << std::endl;
        
        // Print annotations for this line
        for (size_t j = 0; j < 16 && i + j < count; j++) {
            size_t currentOffset = i + j + start;
            if (annotations.find(currentOffset) != annotations.end()) {
                std::cout << "  \033[1;33m" << std::hex << currentOffset << ":\033[0m " 
                          << annotations[currentOffset] << std::endl;
            }
        }
    }
    std::cout << std::dec; // Reset to decimal
}

// Helper function to dump error manager contents
inline void dumpErrorManager(const coil::ErrorManager& errorManager, const std::string& label = "Errors") {
    auto errors = errorManager.getErrors();
    if (errors.empty()) {
        std::cout << "\033[1;32mNo errors in ErrorManager\033[0m" << std::endl;
        return;
    }
    
    std::cout << "\033[1;31m" << label << " (" << errors.size() << "):\033[0m" << std::endl;
    for (const auto& error : errors) {
        std::cout << "  - " << error.toString() << std::endl;
        
        // Capture the last error message for assertions
        lastErrorMessage = error.toString();
    }
}

// Helper function to write binary data to a file
inline bool writeBinaryFile(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "\033[1;31mError opening file for writing:\033[0m " << filename << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    
    if (!file) {
        std::cerr << "\033[1;31mError writing to file:\033[0m " << filename << std::endl;
        return false;
    }
    
    return true;
}

// Helper function to read binary data from a file
inline std::vector<uint8_t> readBinaryFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "\033[1;31mError opening file for reading:\033[0m " << filename << std::endl;
        return {};
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "\033[1;31mError reading from file:\033[0m " << filename << std::endl;
        return {};
    }
    
    return buffer;
}

// Function to analyze CoilHeader structure and print potential issues
inline void analyzeCoilHeader(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(coil::CoilHeader)) {
        std::cerr << "\033[1;31mData too small to contain a CoilHeader (need at least "
                  << sizeof(coil::CoilHeader) << " bytes, got " << data.size() << ")\033[0m" << std::endl;
        return;
    }
    
    try {
        size_t offset = 0;
        coil::CoilHeader header = coil::CoilHeader::decode(data, offset);
        
        // Print header details
        std::cout << "\033[1;34mCoilHeader Analysis:\033[0m" << std::endl;
        std::cout << "  Magic: " << header.magic[0] << header.magic[1] 
                  << header.magic[2] << header.magic[3] << std::endl;
        std::cout << "  Version: " << (int)header.major << "." << (int)header.minor 
                  << "." << (int)header.patch << std::endl;
        std::cout << "  Flags: 0x" << std::hex << (int)header.flags << std::dec << std::endl;
        std::cout << "  Symbol offset: " << header.symbol_offset << std::endl;
        std::cout << "  Section offset: " << header.section_offset << std::endl;
        std::cout << "  Reloc offset: " << header.reloc_offset << std::endl;
        std::cout << "  Debug offset: " << header.debug_offset << std::endl;
        std::cout << "  File size: " << header.file_size << std::endl;
        
        // Basic validation checks
        if (header.magic[0] != 'C' || header.magic[1] != 'O' || 
            header.magic[2] != 'I' || header.magic[3] != 'L') {
            std::cerr << "  \033[1;31mInvalid magic number\033[0m" << std::endl;
        }
        
        if (header.file_size > data.size()) {
            std::cerr << "  \033[1;31mHeader specifies file_size (" << header.file_size 
                      << ") larger than actual data size (" << data.size() << ")\033[0m" << std::endl;
        }
        
        if (header.symbol_offset >= data.size() ||
            header.section_offset >= data.size() ||
            (header.reloc_offset > 0 && header.reloc_offset >= data.size()) ||
            (header.debug_offset > 0 && header.debug_offset >= data.size())) {
            std::cerr << "  \033[1;31mOne or more offsets point outside the data bounds\033[0m" << std::endl;
        }
        
        // Final verdict
        bool valid = header.isValid();
        if (valid) {
            std::cout << "  \033[1;32mHeader validation passed\033[0m" << std::endl;
        } else {
            std::cerr << "  \033[1;31mHeader validation failed\033[0m" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "\033[1;31mFailed to decode header: " << e.what() << "\033[0m" << std::endl;
    }
}

// Helper to create a simple valid COIL object for testing
inline coil::CoilObject createTestCoilObject() {
    coil::CoilObject obj;
    
    // Create symbols
    coil::Symbol textSection;
    textSection.name = ".text";
    textSection.name_length = static_cast<uint16_t>(textSection.name.length());
    textSection.attributes = coil::SymbolFlags::LOCAL;
    textSection.value = 0;
    textSection.section_index = 0; // Will be updated
    textSection.processor_type = 0;
    
    coil::Symbol dataSection;
    dataSection.name = ".data";
    dataSection.name_length = static_cast<uint16_t>(dataSection.name.length());
    dataSection.attributes = coil::SymbolFlags::LOCAL;
    dataSection.value = 0;
    dataSection.section_index = 0; // Will be updated
    dataSection.processor_type = 0;
    
    // Add symbols to the object
    uint16_t textSectionSymIndex = obj.addSymbol(textSection);
    uint16_t dataSectionSymIndex = obj.addSymbol(dataSection);
    
    // Create sections
    coil::Section textSect;
    textSect.name_index = textSectionSymIndex;
    textSect.attributes = coil::SectionFlags::EXECUTABLE | coil::SectionFlags::READABLE;
    textSect.offset = 0;
    textSect.size = 0;
    textSect.address = 0;
    textSect.alignment = 16;
    textSect.processor_type = 0;
    
    coil::Section dataSect;
    dataSect.name_index = dataSectionSymIndex;
    dataSect.attributes = coil::SectionFlags::READABLE | coil::SectionFlags::WRITABLE | coil::SectionFlags::INITIALIZED;
    dataSect.offset = 0;
    dataSect.size = 0;
    dataSect.address = 0;
    dataSect.alignment = 8;
    dataSect.processor_type = 0;
    
    // Add sections to the object
    uint16_t textSectIndex = obj.addSection(textSect);
    uint16_t dataSectIndex = obj.addSection(dataSect);
    
    // Update symbols with correct section indices
    obj.setSymbolSectionIndex(textSectionSymIndex, textSectIndex);
    obj.setSymbolSectionIndex(dataSectionSymIndex, dataSectIndex);
    
    return obj;
}

// Enhanced object validation with detailed error reporting
inline bool validateCoilObjectWithDebug(const coil::CoilObject& object, 
                                        coil::ErrorManager& errorManager,
                                        bool dumpOnFailure = true) {
    // Clear previous errors
    errorManager.clear();
    
    // Validate the object
    bool isValid = coil::utils::Validation::validateCoilObject(object, errorManager);
    
    // If validation failed and dump is requested, print detailed debug info
    if (!isValid && dumpOnFailure) {
        std::cerr << "\033[1;31mCoilObject validation failed with " 
                  << errorManager.getErrors().size() << " errors\033[0m" << std::endl;
        
        // Print all errors
        dumpErrorManager(errorManager, "Validation Errors");
        
        // Print object statistics
        std::cerr << "\033[1;34mObject Statistics:\033[0m" << std::endl;
        std::cerr << "  Symbol count: " << object.getSymbolCount() << std::endl;
        std::cerr << "  Section count: " << object.getSectionCount() << std::endl;
        std::cerr << "  Relocation count: " << object.getRelocationCount() << std::endl;
        
        // Print symbols
        std::cerr << "\033[1;34mSymbols:\033[0m" << std::endl;
        for (uint16_t i = 0; i < object.getSymbolCount(); i++) {
            const auto& symbol = object.getSymbol(i);
            std::cerr << "  [" << i << "] Name: " << symbol.name 
                      << ", Section: " << symbol.section_index 
                      << ", Attrs: 0x" << std::hex << symbol.attributes 
                      << std::dec << std::endl;
        }
        
        // Print sections
        std::cerr << "\033[1;34mSections:\033[0m" << std::endl;
        for (uint16_t i = 0; i < object.getSectionCount(); i++) {
            const auto& section = object.getSection(i);
            std::cerr << "  [" << i << "] Name index: " << section.name_index 
                      << ", Size: " << section.size 
                      << ", Attrs: 0x" << std::hex << section.attributes 
                      << std::dec << std::endl;
            
            // For executable sections, try to dissect instructions
            if (section.attributes & coil::SectionFlags::EXECUTABLE && section.size > 0) {
                std::cerr << "    Section data (executable):" << std::endl;
                try {
                    size_t offset = 0;
                    int instrIndex = 0;
                    
                    // Track problem areas with annotations
                    std::map<size_t, std::string> problemAreas;
                    
                    // Try to decode instructions
                    while (offset < section.data.size()) {
                        size_t startOffset = offset;
                        
                        try {
                            coil::Instruction instr = coil::Instruction::decode(section.data, offset);
                            std::cerr << "      [" << instrIndex << "] Offset " << startOffset 
                                      << ": " << instr.getInstructionName() 
                                      << " (" << (int)instr.getOpcode() << ") with " 
                                      << instr.getOperands().size() << " operands" 
                                      << std::endl;
                        } catch (const std::exception& e) {
                            std::cerr << "      [" << instrIndex << "] \033[1;31mFailed to decode at offset " 
                                      << startOffset << ": " << e.what() << "\033[0m" << std::endl;
                            
                            // Mark this as a problem area
                            problemAreas[startOffset] = "Decode failure: " + std::string(e.what());
                            
                            // Skip to the next byte to try to continue
                            offset = startOffset + 1;
                        }
                        
                        instrIndex++;
                        
                        // Prevent infinite loops
                        if (offset <= startOffset) {
                            std::cerr << "      \033[1;31mNo progress in decoding, stopping\033[0m" << std::endl;
                            break;
                        }
                        
                        // Limit the number of instructions to display
                        if (instrIndex >= 20) {
                            std::cerr << "      \033[1;33m(truncated after 20 instructions)\033[0m" << std::endl;
                            break;
                        }
                    }
                    
                    // Dump the binary data with annotations
                    if (!problemAreas.empty()) {
                        std::cerr << "    Annotated binary dump:" << std::endl;
                        printBinaryData(section.data, 0, section.data.size(), problemAreas);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "    \033[1;31mFailed to analyze section data: " 
                              << e.what() << "\033[0m" << std::endl;
                    printBinaryData(section.data);
                }
            } else if (section.size > 0) {
                // Just dump the first 64 bytes of non-executable sections
                std::cerr << "    Section data preview:" << std::endl;
                printBinaryData(section.data, 0, std::min(section.data.size(), (size_t)64));
                if (section.data.size() > 64) {
                    std::cerr << "    \033[1;33m(truncated, total size: " 
                              << section.data.size() << " bytes)\033[0m" << std::endl;
                }
            }
        }
        
        // Print relocations
        if (object.getRelocationCount() > 0) {
            std::cerr << "\033[1;34mRelocations:\033[0m" << std::endl;
            for (uint16_t i = 0; i < object.getRelocationCount(); i++) {
                const auto& reloc = object.getRelocation(i);
                std::cerr << "  [" << i << "] Section: " << reloc.section_index 
                          << ", Symbol: " << reloc.symbol_index 
                          << ", Offset: " << reloc.offset 
                          << ", Type: " << (int)reloc.type 
                          << ", Size: " << (int)reloc.size << std::endl;
            }
        }
    }
    
    return isValid;
}

// Special validation debug function just for validation tests
inline bool validateAndDebug(const std::function<bool(coil::ErrorManager&)>& validationFunc, 
                            const std::string& testName) {
    coil::ErrorManager errorManager;
    bool isValid = validationFunc(errorManager);
    
    if (!isValid) {
        std::cerr << "\033[1;31mValidation failed: " << testName << "\033[0m" << std::endl;
        dumpErrorManager(errorManager, "Validation Errors");
    }
    
    return isValid;
}

// Special validation for section validation tests
inline bool validateSection(const coil::CoilObject& obj, uint16_t sectionIndex) {
    coil::ErrorManager errorManager;
    bool isValid = coil::utils::Validation::validateSectionData(obj, sectionIndex, errorManager);
    
    if (!isValid) {
        std::cerr << "\033[1;31mSection validation failed for section " << sectionIndex << "\033[0m" << std::endl;
        dumpErrorManager(errorManager, "Section Validation Errors");
        
        // Print section data
        const auto& section = obj.getSection(sectionIndex);
        std::cerr << "\033[1;34mSection Details:\033[0m" << std::endl;
        std::cerr << "  Size: " << section.size << std::endl;
        std::cerr << "  Attributes: 0x" << std::hex << section.attributes << std::dec << std::endl;
        std::cerr << "  Data:" << std::endl;
        printBinaryData(section.data);
    }
    
    return isValid;
}

#endif // COIL_TEST_HELPER_H