#include <coil/linker.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <unordered_set>
#include <filesystem>

namespace coil {

LinkResult::bool saveToFile(const std::string& filename) const {
    if (!success || !outputFile) {
        return false;
    }
    
    return outputFile->saveToFile(filename);
}

std::vector<uint8_t> LinkResult::getBinary() const {
    if (!success || !outputFile) {
        return {};
    }
    
    return outputFile->getBinary();
}

Linker::Linker(const LinkOptions& options)
    : options_(options), conflictResolution_(SymbolConflictResolution::TAKE_STRONGEST) {
}

void Linker::addObjectFile(std::shared_ptr<ObjectFile> objectFile) {
    if (!objectFile || !objectFile->isValid()) {
        return;
    }
    
    InputFile input;
    input.objectFile = objectFile;
    inputFiles_.push_back(input);
}

bool Linker::addObjectFileFromPath(const std::string& path) {
    // First try the path as-is
    std::ifstream file(path, std::ios::binary);
    if (file) {
        auto objectFile = std::make_shared<ObjectFile>(path);
        if (objectFile->isValid()) {
            addObjectFile(objectFile);
            return true;
        }
    }
    
    // If that failed, try the search paths
    for (const auto& searchPath : options_.searchPaths) {
        std::string fullPath = searchPath;
        if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') {
            fullPath += '/';
        }
        fullPath += path;
        
        std::ifstream searchFile(fullPath, std::ios::binary);
        if (searchFile) {
            auto objectFile = std::make_shared<ObjectFile>(fullPath);
            if (objectFile->isValid()) {
                addObjectFile(objectFile);
                return true;
            }
        }
    }
    
    return false;
}

void Linker::setSymbolConflictResolution(SymbolConflictResolution strategy) {
    conflictResolution_ = strategy;
}

void Linker::setSymbolBinding(const std::string& symbolName, SymbolBinding binding) {
    symbolBindingOverrides_[symbolName] = binding;
}

void Linker::setSymbolVisibility(const std::string& symbolName, SymbolVisibility visibility) {
    symbolVisibilityOverrides_[symbolName] = visibility;
}

void Linker::setEntryPointSymbol(const std::string& symbolName) {
    entryPointSymbol_ = symbolName;
}

void Linker::setSectionLoadAddress(const std::string& sectionName, uint64_t address) {
    sectionAddressOverrides_[sectionName] = address;
}

LinkResult Linker::link() {
    LinkResult result;
    result.success = false;
    
    // Initialize the output file
    outputFile_ = std::make_shared<ObjectFile>();
    
    try {
        // Step 1: Validate input files
        if (!validateInputFiles()) {
            throw std::runtime_error("Input file validation failed: " + errorMessage_);
        }
        
        // Step 2: Merge string tables
        if (!mergeStringTables()) {
            throw std::runtime_error("String table merge failed: " + errorMessage_);
        }
        
        // Step 3: Resolve symbols
        if (!resolveSymbols()) {
            throw std::runtime_error("Symbol resolution failed: " + errorMessage_);
        }
        
        // Step 4: Merge sections
        if (!mergeSections()) {
            throw std::runtime_error("Section merge failed: " + errorMessage_);
        }
        
        // Step 5: Process relocations
        if (!processRelocations()) {
            throw std::runtime_error("Relocation processing failed: " + errorMessage_);
        }
        
        // Step 6: Generate output
        if (!generateOutput()) {
            throw std::runtime_error("Output generation failed: " + errorMessage_);
        }
        
        // Success!
        result.success = true;
        result.outputFile = outputFile_;
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }
    
    return result;
}

void Linker::reset() {
    inputFiles_.clear();
    mergedSections_.clear();
    symbolTable_.clear();
    symbolBindingOverrides_.clear();
    symbolVisibilityOverrides_.clear();
    sectionAddressOverrides_.clear();
    entryPointSymbol_.clear();
    errorMessage_.clear();
    outputFile_.reset();
}

bool Linker::validateInputFiles() {
    // Check if we have any input files
    if (inputFiles_.empty()) {
        errorMessage_ = "No input files provided";
        return false;
    }
    
    // Check architectures
    uint32_t firstPu = inputFiles_[0].objectFile->getHeader().target_pu;
    uint32_t firstArch = inputFiles_[0].objectFile->getHeader().target_arch;
    uint32_t firstMode = inputFiles_[0].objectFile->getHeader().target_mode;
    
    for (size_t i = 1; i < inputFiles_.size(); i++) {
        const auto& header = inputFiles_[i].objectFile->getHeader();
        
        if (header.target_pu != firstPu && !options_.allowMismatchedArch) {
            errorMessage_ = "Mismatched processing unit in input file " + std::to_string(i);
            return false;
        }
        
        if (header.target_arch != firstArch && !options_.allowMismatchedArch) {
            errorMessage_ = "Mismatched architecture in input file " + std::to_string(i);
            return false;
        }
        
        if (header.target_mode != firstMode && !options_.allowMismatchedArch) {
            errorMessage_ = "Mismatched mode in input file " + std::to_string(i);
            return false;
        }
    }
    
    return true;
}

bool Linker::mergeStringTables() {
    // Add strings from all input files to the output file's string table
    for (auto& inputFile : inputFiles_) {
        const auto& strings = inputFile.objectFile->getSections();
        
        // Map old string indices to new ones
        for (const auto& symbol : inputFile.objectFile->getSymbols()) {
            std::string name = inputFile.objectFile->getString(symbol.getNameIndex());
            if (!name.empty()) {
                uint32_t newIndex = outputFile_->addString(name);
                inputFile.stringMap[symbol.getNameIndex()] = newIndex;
            }
        }
        
        // Also map section names
        for (const auto& section : inputFile.objectFile->getSections()) {
            std::string name = inputFile.objectFile->getString(section.getNameIndex());
            if (!name.empty()) {
                uint32_t newIndex = outputFile_->addString(name);
                inputFile.stringMap[section.getNameIndex()] = newIndex;
            }
        }
    }
    
    return true;
}

bool Linker::resolveSymbols() {
    // First pass: collect all symbols
    struct SymbolInfo {
        InputFile* file;
        uint32_t index;
        Symbol symbol;
        std::string name;
    };
    
    std::unordered_map<std::string, std::vector<SymbolInfo>> symbolsByName;
    
    for (auto& inputFile : inputFiles_) {
        const auto& symbols = inputFile.objectFile->getSymbols();
        
        for (size_t i = 0; i < symbols.size(); i++) {
            const Symbol& symbol = symbols[i];
            std::string name = inputFile.objectFile->getString(symbol.getNameIndex());
            
            // Skip empty names and local symbols
            if (name.empty() || symbol.getBinding() == SymbolBinding::LOCAL) {
                continue;
            }
            
            symbolsByName[name].push_back({&inputFile, static_cast<uint32_t>(i), symbol, name});
        }
    }
    
    // Second pass: resolve symbols
    for (auto& pair : symbolsByName) {
        const std::string& name = pair.first;
        auto& candidates = pair.second;
        
        // Check for binding override
        auto bindingIt = symbolBindingOverrides_.find(name);
        if (bindingIt != symbolBindingOverrides_.end()) {
            for (auto& candidate : candidates) {
                if (candidate.symbol.getBinding() != SymbolBinding::LOCAL) {
                    // Override the binding
                    candidate.symbol = Symbol(
                        name,
                        candidate.symbol.getSectionIndex(),
                        candidate.symbol.getValue(),
                        candidate.symbol.getSize(),
                        candidate.symbol.getType(),
                        bindingIt->second,
                        candidate.symbol.getVisibility()
                    );
                }
            }
        }
        
        // Check for visibility override
        auto visibilityIt = symbolVisibilityOverrides_.find(name);
        if (visibilityIt != symbolVisibilityOverrides_.end()) {
            for (auto& candidate : candidates) {
                if (candidate.symbol.getBinding() != SymbolBinding::LOCAL) {
                    // Override the visibility
                    candidate.symbol = Symbol(
                        name,
                        candidate.symbol.getSectionIndex(),
                        candidate.symbol.getValue(),
                        candidate.symbol.getSize(),
                        candidate.symbol.getType(),
                        candidate.symbol.getBinding(),
                        visibilityIt->second
                    );
                }
            }
        }
        
        // Classify symbols
        std::vector<SymbolInfo*> strongDefs;
        std::vector<SymbolInfo*> weakDefs;
        std::vector<SymbolInfo*> commonSymbols;
        std::vector<SymbolInfo*> undefinedRefs;
        
        for (auto& candidate : candidates) {
            if (candidate.symbol.getSectionIndex() == 0) {
                // Undefined reference
                undefinedRefs.push_back(&candidate);
            } else if (candidate.symbol.getType() == SymbolType::COMMON) {
                // Common symbol
                commonSymbols.push_back(&candidate);
            } else if (candidate.symbol.getBinding() == SymbolBinding::GLOBAL) {
                // Strong definition
                strongDefs.push_back(&candidate);
            } else if (candidate.symbol.getBinding() == SymbolBinding::WEAK) {
                // Weak definition
                weakDefs.push_back(&candidate);
            }
        }
        
        // Apply resolution rules
        Symbol resolvedSymbol = Symbol(name, 0, 0, 0, SymbolType::NOTYPE, SymbolBinding::LOCAL, SymbolVisibility::DEFAULT);
        bool symbolResolved = false;
        
        // Multiple strong definitions are an error
        if (strongDefs.size() > 1) {
            if (conflictResolution_ == SymbolConflictResolution::ERROR) {
                errorMessage_ = "Multiple strong definitions of symbol: " + name;
                return false;
            } else if (conflictResolution_ == SymbolConflictResolution::TAKE_FIRST) {
                // Use the first definition
                resolvedSymbol = strongDefs[0]->symbol;
                symbolResolved = true;
            } else {
                // Must be TAKE_STRONGEST, which is already the same as TAKE_FIRST for strong symbols
                resolvedSymbol = strongDefs[0]->symbol;
                symbolResolved = true;
            }
        } else if (strongDefs.size() == 1) {
            // One strong definition
            resolvedSymbol = strongDefs[0]->symbol;
            symbolResolved = true;
        } else if (weakDefs.size() > 0) {
            // One or more weak definitions
            resolvedSymbol = weakDefs[0]->symbol;
            symbolResolved = true;
        } else if (commonSymbols.size() > 0) {
            // One or more common symbols
            // Find the largest common symbol
            size_t maxSize = 0;
            SymbolInfo* maxSymbol = nullptr;
            
            for (auto* symbol : commonSymbols) {
                if (symbol->symbol.getSize() > maxSize) {
                    maxSize = symbol->symbol.getSize();
                    maxSymbol = symbol;
                }
            }
            
            resolvedSymbol = maxSymbol->symbol;
            symbolResolved = true;
        } else if (!undefinedRefs.empty()) {
            // Only undefined references
            if (options_.resolveAllSymbols && options_.createExecutable) {
                errorMessage_ = "Undefined symbol: " + name;
                return false;
            }
            
            // Keep the symbol as undefined
            resolvedSymbol = undefinedRefs[0]->symbol;
            symbolResolved = true;
        }
        
        // Add the resolved symbol to the symbol table
        if (symbolResolved) {
            // Set name index in output file
            uint32_t nameIndex = outputFile_->addString(name);
            resolvedSymbol.setNameIndex(nameIndex);
            
            // Add to symbol table
            symbolTable_[name] = resolvedSymbol;
        }
    }
    
    return true;
}

bool Linker::mergeSections() {
    // Combine sections with the same name from all input files
    for (auto& inputFile : inputFiles_) {
        const auto& sections = inputFile.objectFile->getSections();
        
        for (size_t i = 0; i < sections.size(); i++) {
            const Section& section = sections[i];
            std::string name = inputFile.objectFile->getString(section.getNameIndex());
            
            // Skip special sections if stripping debug
            if (options_.stripDebug) {
                if (name.find(".debug") == 0 || name.find(".comment") == 0) {
                    continue;
                }
            }
            
            // Get or create the merged section
            auto it = mergedSections_.find(name);
            if (it == mergedSections_.end()) {
                MergedSection mergedSection;
                mergedSection.name = name;
                mergedSection.type = static_cast<SectionType>(section.getType());
                mergedSection.flags = section.getFlags();
                mergedSection.size = 0;
                mergedSection.alignment = section.getAlignment();
                
                // Check for section address override
                auto addrIt = sectionAddressOverrides_.find(name);
                if (addrIt != sectionAddressOverrides_.end()) {
                    mergedSection.loadAddress = addrIt->second;
                } else {
                    mergedSection.loadAddress = 0;
                }
                
                it = mergedSections_.insert(std::make_pair(name, mergedSection)).first;
            }
            
            // Check section compatibility
            MergedSection& mergedSection = it->second;
            
            if (static_cast<SectionType>(section.getType()) != mergedSection.type) {
                errorMessage_ = "Incompatible section types for section: " + name;
                return false;
            }
            
            uint32_t incompatibleFlags = static_cast<uint32_t>(SectionFlag::WRITABLE) |
                                         static_cast<uint32_t>(SectionFlag::EXECUTABLE);
            
            if ((section.getFlags() & incompatibleFlags) != (mergedSection.flags & incompatibleFlags)) {
                errorMessage_ = "Incompatible section flags for section: " + name;
                return false;
            }
            
            // Update merged section properties
            mergedSection.alignment = std::max(mergedSection.alignment, section.getAlignment());
            mergedSection.flags |= section.getFlags(); // Combine flags
            
            // Add this section to the source list
            mergedSection.sourceList.push_back(std::make_pair(&inputFile, static_cast<uint32_t>(i)));
            
            // Map section index
            inputFile.sectionMap[i] = mergedSection.sourceList.size() - 1;
        }
    }
    
    return true;
}

bool Linker::processRelocations() {
    // Process relocations from all input files
    for (auto& inputFile : inputFiles_) {
        const auto& relocations = inputFile.objectFile->getRelocations();
        
        for (const auto& relocation : relocations) {
            // Get the symbol being referenced
            uint32_t symbolIndex = relocation.getSymbolIndex();
            const Symbol& symbol = inputFile.objectFile->getSymbol(symbolIndex);
            std::string symbolName = inputFile.objectFile->getString(symbol.getNameIndex());
            
            // Get the section being modified
            uint32_t sectionIndex = relocation.getOffset() >> 32; // Top 32 bits are section index
            uint32_t offsetInSection = relocation.getOffset() & 0xFFFFFFFF; // Bottom 32 bits are offset
            
            // Get the corresponding merged section
            auto sectionIt = inputFile.sectionMap.find(sectionIndex);
            if (sectionIt == inputFile.sectionMap.end()) {
                errorMessage_ = "Invalid section index in relocation: " + std::to_string(sectionIndex);
                return false;
            }
            
            // Find the resolved symbol
            auto symbolIt = symbolTable_.find(symbolName);
            if (symbolIt == symbolTable_.end()) {
                errorMessage_ = "Symbol not found in symbol table: " + symbolName;
                return false;
            }
            
            const Symbol& resolvedSymbol = symbolIt->second;
            
            // If this is a simple relocation and the symbol is defined, we can apply it now
            if (options_.createExecutable && resolvedSymbol.getSectionIndex() != 0) {
                // TODO: Apply relocation directly in the merged section data
                
                // For now, just skip it - we'll include relocations in the output
            }
            
            // For shared objects or undefined symbols, add the relocation to the output
            if (!options_.createExecutable || resolvedSymbol.getSectionIndex() == 0 || options_.keepRelocations) {
                // TODO: Add the relocation to the output file
            }
        }
    }
    
    return true;
}

bool Linker::generateOutput() {
    // Generate the output header
    ObjectHeader& header = const_cast<ObjectHeader&>(outputFile_->getHeader());
    
    // Copy info from first input file
    if (!inputFiles_.empty()) {
        const ObjectHeader& firstHeader = inputFiles_[0].objectFile->getHeader();
        header.target_pu = firstHeader.target_pu;
        header.target_arch = firstHeader.target_arch;
        header.target_mode = firstHeader.target_mode;
    }
    
    // Set appropriate flags
    if (options_.createExecutable) {
        header.flags |= static_cast<uint32_t>(ObjectFileFlag::EXECUTABLE);
    } else {
        header.flags |= static_cast<uint32_t>(ObjectFileFlag::SHARED_OBJECT);
    }
    
    // Add the merged sections to the output file
    for (const auto& [name, mergedSection] : mergedSections_) {
        // Create a new section
        SectionEntry entry;
        entry.type = static_cast<uint32_t>(mergedSection.type);
        entry.flags = mergedSection.flags;
        entry.align = mergedSection.alignment;
        entry.name_idx = outputFile_->addString(name);
        
        // Calculate total size including padding for alignment
        uint64_t totalSize = 0;
        
        for (const auto& [file, sectionIndex] : mergedSection.sourceList) {
            const Section& section = file->objectFile->getSection(sectionIndex);
            
            // Align the current offset
            totalSize = (totalSize + section.getAlignment() - 1) & ~(section.getAlignment() - 1);
            
            // Add section size
            totalSize += section.getSize();
        }
        
        entry.size = totalSize;
        
        // For BSS sections, we don't include data
        std::vector<uint8_t> sectionData;
        
        if (mergedSection.type != SectionType::BSS) {
            // Allocate space for the section data
            sectionData.resize(totalSize);
            
            // Copy data from input sections
            uint64_t currentOffset = 0;
            
            for (const auto& [file, sectionIndex] : mergedSection.sourceList) {
                const Section& section = file->objectFile->getSection(sectionIndex);
                
                // Align the current offset
                uint64_t alignedOffset = (currentOffset + section.getAlignment() - 1) & ~(section.getAlignment() - 1);
                
                // Add padding if needed
                for (uint64_t i = currentOffset; i < alignedOffset; i++) {
                    sectionData[i] = 0;
                }
                
                // Copy section data
                const std::vector<uint8_t>& data = section.getData();
                std::copy(data.begin(), data.end(), sectionData.begin() + alignedOffset);
                
                // Update current offset
                currentOffset = alignedOffset + section.getSize();
            }
        }
        
        // Create and add the section
        Section outputSection(entry, sectionData);
        outputFile_->addSection(outputSection);
    }
    
    // Add the resolved symbols to the output file
    for (const auto& [name, symbol] : symbolTable_) {
        outputFile_->addSymbol(symbol);
    }
    
    // Set entry point
    if (!entryPointSymbol_.empty()) {
        // Find the entry point symbol
        auto it = symbolTable_.find(entryPointSymbol_);
        if (it != symbolTable_.end() && it->second.getSectionIndex() != 0) {
            // Set the entry point to the symbol's value
            uint64_t entryPoint = it->second.getValue();
            
            // If the symbol is in a section with a load address, add the load address
            uint32_t sectionIndex = it->second.getSectionIndex();
            const Section& section = outputFile_->getSection(sectionIndex);
            std::string sectionName = outputFile_->getString(section.getNameIndex());
            
            auto loadAddrIt = sectionAddressOverrides_.find(sectionName);
            if (loadAddrIt != sectionAddressOverrides_.end()) {
                entryPoint += loadAddrIt->second;
            }
            
            outputFile_->setEntryPoint(entryPoint);
        } else {
            errorMessage_ = "Entry point symbol not found or undefined: " + entryPointSymbol_;
            return false;
        }
    }
    
    return true;
}

LinkResult linkFiles(const std::vector<std::string>& inputFiles, 
                   const std::string& outputFile, 
                   const LinkOptions& options) {
    Linker linker(options);
    
    // Add input files
    for (const auto& inputFile : inputFiles) {
        if (!linker.addObjectFileFromPath(inputFile)) {
            LinkResult result;
            result.success = false;
            result.error = "Failed to add input file: " + inputFile;
            return result;
        }
    }
    
    // Perform linking
    LinkResult result = linker.link();
    
    // Write output file
    if (result.success && !outputFile.empty()) {
        if (!result.saveToFile(outputFile)) {
            result.success = false;
            result.error = "Failed to write output file: " + outputFile;
        }
    }
    
    return result;
}

LinkResult mergeObjectFiles(const std::vector<std::string>& inputFiles, 
                          const std::string& outputFile) {
    // Create link options for merging
    LinkOptions options;
    options.createExecutable = false;
    options.stripDebug = false;
    options.resolveAllSymbols = false;
    options.keepRelocations = true;
    
    return linkFiles(inputFiles, outputFile, options);
}

} // namespace coil
