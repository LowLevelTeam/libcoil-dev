#include <coil/object_file.h>
#include <coil/binary_format.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace coil;

// Helper function to get section type name
std::string getSectionTypeName(SectionType type) {
    switch (type) {
        case SectionType::CODE: return "CODE";
        case SectionType::DATA: return "DATA";
        case SectionType::RODATA: return "RODATA";
        case SectionType::BSS: return "BSS";
        case SectionType::SYMTAB: return "SYMTAB";
        case SectionType::STRTAB: return "STRTAB";
        case SectionType::RELOC: return "RELOC";
        case SectionType::DEBUG: return "DEBUG";
        case SectionType::COMMENT: return "COMMENT";
        case SectionType::NOTE: return "NOTE";
        case SectionType::SPECIAL: return "SPECIAL";
        default: return "UNKNOWN";
    }
}

// Helper function to get symbol type name
std::string getSymbolTypeName(SymbolType type) {
    switch (type) {
        case SymbolType::NOTYPE: return "NOTYPE";
        case SymbolType::FUNCTION: return "FUNCTION";
        case SymbolType::DATA: return "DATA";
        case SymbolType::SECTION: return "SECTION";
        case SymbolType::FILE: return "FILE";
        case SymbolType::COMMON: return "COMMON";
        case SymbolType::TLS: return "TLS";
        default: return "UNKNOWN";
    }
}

// Helper function to get symbol binding name
std::string getSymbolBindingName(SymbolBinding binding) {
    switch (binding) {
        case SymbolBinding::LOCAL: return "LOCAL";
        case SymbolBinding::GLOBAL: return "GLOBAL";
        case SymbolBinding::WEAK: return "WEAK";
        case SymbolBinding::UNIQUE: return "UNIQUE";
        default: return "UNKNOWN";
    }
}

// Helper function to get symbol visibility name
std::string getSymbolVisibilityName(SymbolVisibility visibility) {
    switch (visibility) {
        case SymbolVisibility::DEFAULT: return "DEFAULT";
        case SymbolVisibility::INTERNAL: return "INTERNAL";
        case SymbolVisibility::HIDDEN: return "HIDDEN";
        case SymbolVisibility::PROTECTED: return "PROTECTED";
        default: return "UNKNOWN";
    }
}

// Helper function to get relocation type name
std::string getRelocationTypeName(RelocationType type) {
    switch (type) {
        case RelocationType::ABS32: return "ABS32";
        case RelocationType::ABS64: return "ABS64";
        case RelocationType::PCREL32: return "PCREL32";
        case RelocationType::PCREL64: return "PCREL64";
        case RelocationType::GOTREL: return "GOTREL";
        case RelocationType::PLTREL: return "PLTREL";
        default: return "UNKNOWN";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <object_file>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    
    // Open the object file
    ObjectFile objFile(filename);
    
    if (!objFile.isValid()) {
        std::cout << "Failed to open object file: " << objFile.getError() << std::endl;
        return 1;
    }
    
    // Print header information
    std::cout << "COIL Object File: " << filename << std::endl;
    std::cout << "==============================================" << std::endl;
    
    const ObjectHeader& header = objFile.getHeader();
    
    std::cout << "Header:" << std::endl;
    std::cout << "  Version: " << ((header.version >> 16) & 0xFF) << "."
                               << ((header.version >> 8) & 0xFF) << "."
                               << (header.version & 0xFF) << std::endl;
                               
    std::cout << "  Flags: 0x" << std::hex << header.flags << std::dec;
    if (header.flags & static_cast<uint32_t>(ObjectFileFlag::EXECUTABLE)) std::cout << " EXECUTABLE";
    if (header.flags & static_cast<uint32_t>(ObjectFileFlag::SHARED_OBJECT)) std::cout << " SHARED_OBJECT";
    if (header.flags & static_cast<uint32_t>(ObjectFileFlag::POS_INDEPENDENT)) std::cout << " POS_INDEPENDENT";
    if (header.flags & static_cast<uint32_t>(ObjectFileFlag::DEBUG_INFO)) std::cout << " DEBUG_INFO";
    if (header.flags & static_cast<uint32_t>(ObjectFileFlag::RELOCATABLE)) std::cout << " RELOCATABLE";
    std::cout << std::endl;
    
    std::cout << "  Target: PU=" << header.target_pu
                      << ", Arch=" << header.target_arch
                      << ", Mode=" << header.target_mode << std::endl;
                      
    std::cout << "  Entry Point: 0x" << std::hex << header.entry_point << std::dec << std::endl;
    std::cout << "  Endianness: " << (header.endianness ? "Big" : "Little") << std::endl;
    
    // Print sections
    const auto& sections = objFile.getSections();
    std::cout << std::endl << "Sections: " << sections.size() << std::endl;
    std::cout << "  Idx  Name                 Type        Size        Offset      Align  Flags" << std::endl;
    std::cout << "  -----------------------------------------------------------------------" << std::endl;
    
    for (size_t i = 0; i < sections.size(); i++) {
        const Section& section = sections[i];
        std::string name = objFile.getString(section.getNameIndex());
        
        std::cout << "  " << std::setw(4) << std::left << i 
                  << " " << std::setw(20) << name
                  << " " << std::setw(10) << getSectionTypeName(section.getType())
                  << " " << std::setw(10) << section.getSize()
                  << " " << std::setw(10) << std::hex << "0x" << section.getEntry().offset << std::dec
                  << " " << std::setw(6) << section.getAlignment()
                  << " ";
                  
        if (section.hasFlag(SectionFlag::WRITABLE)) std::cout << "W";
        if (section.hasFlag(SectionFlag::EXECUTABLE)) std::cout << "X";
        if (section.hasFlag(SectionFlag::INITIALIZED)) std::cout << "I";
        if (section.hasFlag(SectionFlag::ALLOC)) std::cout << "A";
        
        std::cout << std::endl;
    }
    
    // Print symbols
    const auto& symbols = objFile.getSymbols();
    std::cout << std::endl << "Symbols: " << symbols.size() << std::endl;
    std::cout << "  Idx  Name                 Section  Value       Size        Type       Binding    Vis" << std::endl;
    std::cout << "  -------------------------------------------------------------------------------------" << std::endl;
    
    for (size_t i = 0; i < symbols.size(); i++) {
        const Symbol& symbol = symbols[i];
        std::string name = objFile.getString(symbol.getNameIndex());
        
        std::cout << "  " << std::setw(4) << std::left << i 
                  << " " << std::setw(20) << name
                  << " " << std::setw(8) << symbol.getSectionIndex()
                  << " " << std::setw(10) << std::hex << "0x" << symbol.getValue() << std::dec
                  << " " << std::setw(10) << symbol.getSize()
                  << " " << std::setw(10) << getSymbolTypeName(symbol.getType())
                  << " " << std::setw(10) << getSymbolBindingName(symbol.getBinding())
                  << " " << getSymbolVisibilityName(symbol.getVisibility())
                  << std::endl;
    }
    
    // Print relocations
    const auto& relocations = objFile.getRelocations();
    if (!relocations.empty()) {
        std::cout << std::endl << "Relocations: " << relocations.size() << std::endl;
        std::cout << "  Offset              Symbol   Type        Addend" << std::endl;
        std::cout << "  --------------------------------------------------" << std::endl;
        
        for (const auto& reloc : relocations) {
            std::cout << "  " << std::setw(18) << std::hex << "0x" << reloc.getOffset() << std::dec
                      << " " << std::setw(8) << reloc.getSymbolIndex()
                      << " " << std::setw(10) << getRelocationTypeName(reloc.getType())
                      << " " << reloc.getAddend()
                      << std::endl;
        }
    }
    
    // If this is an executable with a .text section, disassemble it
    uint32_t textIdx = objFile.findSection(".text");
    if (textIdx != static_cast<uint32_t>(-1)) {
        const Section& textSection = objFile.getSection(textIdx);
        BinaryReader reader(textSection.getData());
        
        if (reader.isValid()) {
            std::vector<Instruction> instructions = reader.getInstructions();
            
            std::cout << std::endl << "Disassembly of .text section (" << instructions.size() << " instructions):" << std::endl;
            
            for (size_t i = 0; i < instructions.size(); i++) {
                std::cout << "  " << instructions[i].toString() << std::endl;
            }
        }
    }
    
    return 0;
}
