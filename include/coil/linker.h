#ifndef COIL_LINKER_H
#define COIL_LINKER_H

#include <coil/object_file.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace coil {

/**
 * @brief Link options for controlling the linking process
 */
struct LinkOptions {
    bool stripDebug = false;           // Strip debug information
    bool resolveAllSymbols = true;     // Require all symbols to be resolved
    bool allowMismatchedArch = false;  // Allow objects with different architectures
    bool createExecutable = true;      // Create executable (vs shared object or relocatable)
    bool keepRelocations = false;      // Keep relocation information in output
    uint64_t baseAddress = 0x400000;   // Base address for loadable sections
    std::vector<std::string> searchPaths; // Paths to search for libraries
    
    // Constructor with default values
    LinkOptions() = default;
};

/**
 * @brief Result of the linking process
 */
struct LinkResult {
    bool success = false;              // Whether linking succeeded
    std::string error;                 // Error message if failed
    std::shared_ptr<ObjectFile> outputFile; // The resulting linked file
    
    // Helper methods
    bool saveToFile(const std::string& filename) const;
    std::vector<uint8_t> getBinary() const;
};

/**
 * @brief Symbol resolution conflict handling
 */
enum class SymbolConflictResolution {
    ERROR,            // Generate an error
    TAKE_FIRST,       // Use the first definition encountered
    TAKE_STRONGEST,   // Use the strongest binding (GLOBAL > WEAK > COMMON)
    MERGE             // Merge symbols (for COMMON or UNIQUE)
};

/**
 * @brief Handles the linking process for COIL object files
 */
class Linker {
public:
    // Constructor with options
    explicit Linker(const LinkOptions& options = LinkOptions());
    
    // Add an object file to link
    void addObjectFile(std::shared_ptr<ObjectFile> objectFile);
    
    // Add an object file from a file path
    bool addObjectFileFromPath(const std::string& path);
    
    // Set symbol conflict resolution strategy
    void setSymbolConflictResolution(SymbolConflictResolution strategy);
    
    // Set symbol binding override
    void setSymbolBinding(const std::string& symbolName, SymbolBinding binding);
    
    // Set symbol visibility override
    void setSymbolVisibility(const std::string& symbolName, SymbolVisibility visibility);
    
    // Set entry point by symbol name
    void setEntryPointSymbol(const std::string& symbolName);
    
    // Set load address for a section
    void setSectionLoadAddress(const std::string& sectionName, uint64_t address);
    
    // Perform the linking process
    LinkResult link();
    
    // Clear all input files and settings
    void reset();
    
private:
    // Helper methods for linking
    bool validateInputFiles();
    bool mergeStringTables();
    bool resolveSymbols();
    bool mergeSections();
    bool processRelocations();
    bool generateOutput();
    
    // Symbol resolution helpers
    bool resolveSymbol(const std::string& name, Symbol& result);
    bool handleSymbolConflict(const Symbol& sym1, const Symbol& sym2, Symbol& result);
    
    // Tracking structures
    struct InputFile {
        std::shared_ptr<ObjectFile> objectFile;
        std::unordered_map<uint32_t, uint32_t> stringMap; // Original index -> merged index
        std::unordered_map<uint32_t, uint32_t> sectionMap; // Original index -> merged index
        std::unordered_map<uint32_t, uint32_t> symbolMap; // Original index -> merged index
    };
    
    struct MergedSection {
        std::string name;
        SectionType type;
        uint32_t flags;
        uint64_t size;
        uint64_t alignment;
        uint64_t loadAddress;
        std::vector<std::pair<InputFile*, uint32_t>> sourceList; // Source file and section index
    };
    
    LinkOptions options_;
    std::vector<InputFile> inputFiles_;
    std::unordered_map<std::string, MergedSection> mergedSections_;
    std::unordered_map<std::string, Symbol> symbolTable_;
    std::unordered_map<std::string, SymbolBinding> symbolBindingOverrides_;
    std::unordered_map<std::string, SymbolVisibility> symbolVisibilityOverrides_;
    std::unordered_map<std::string, uint64_t> sectionAddressOverrides_;
    SymbolConflictResolution conflictResolution_ = SymbolConflictResolution::TAKE_STRONGEST;
    std::string entryPointSymbol_;
    std::string errorMessage_;
    std::shared_ptr<ObjectFile> outputFile_;
};

/**
 * @brief Utility function to link multiple object files
 */
LinkResult linkFiles(const std::vector<std::string>& inputFiles, 
                    const std::string& outputFile, 
                    const LinkOptions& options = LinkOptions());

/**
 * @brief Merge multiple object files into a single relocatable object file
 */
LinkResult mergeObjectFiles(const std::vector<std::string>& inputFiles, 
                           const std::string& outputFile);

} // namespace coil

#endif // COIL_LINKER_H
