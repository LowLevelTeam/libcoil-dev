#include "coil/obj.hpp"
#include <cstring>
#include <algorithm>
#include <unordered_map>

namespace coil {

namespace elf {

const char* getFileTypeName(uint16_t type) {
    switch (type) {
        case ET_NONE: return "None";
        case ET_REL: return "Relocatable";
        case ET_EXEC: return "Executable";
        case ET_DYN: return "Shared object";
        case ET_CORE: return "Core";
        default: return "Unknown";
    }
}

const char* getMachineTypeName(uint16_t machine) {
    switch (machine) {
        case EM_NONE: return "None";
        case EM_386: return "Intel 80386";
        case EM_ARM: return "ARM";
        case EM_X86_64: return "AMD x86-64";
        case EM_AARCH64: return "ARM AARCH64";
        default: return "Unknown";
    }
}

const char* getSectionTypeName(uint32_t type) {
    switch (type) {
        case SHT_NULL: return "NULL";
        case SHT_PROGBITS: return "PROGBITS";
        case SHT_SYMTAB: return "SYMTAB";
        case SHT_STRTAB: return "STRTAB";
        case SHT_RELA: return "RELA";
        case SHT_HASH: return "HASH";
        case SHT_DYNAMIC: return "DYNAMIC";
        case SHT_NOTE: return "NOTE";
        case SHT_NOBITS: return "NOBITS";
        case SHT_REL: return "REL";
        case SHT_DYNSYM: return "DYNSYM";
        default: return "Unknown";
    }
}

std::string getSectionFlagsString(uint64_t flags) {
    std::string result;
    
    if (flags & SHF_WRITE) result += "W";
    if (flags & SHF_ALLOC) result += "A";
    if (flags & SHF_EXECINSTR) result += "X";
    if (flags & SHF_MERGE) result += "M";
    if (flags & SHF_STRINGS) result += "S";
    
    return result;
}

const char* getSymbolBindingName(uint8_t binding) {
    switch (binding) {
        case STB_LOCAL: return "LOCAL";
        case STB_GLOBAL: return "GLOBAL";
        case STB_WEAK: return "WEAK";
        default: return "Unknown";
    }
}

const char* getSymbolTypeName(uint8_t type) {
    switch (type) {
        case STT_NOTYPE: return "NOTYPE";
        case STT_OBJECT: return "OBJECT";
        case STT_FUNC: return "FUNC";
        case STT_SECTION: return "SECTION";
        case STT_FILE: return "FILE";
        default: return "Unknown";
    }
}

const char* getRelocationTypeName(uint16_t machine, uint32_t type) {
    // Only handle a few common architectures and relocation types
    if (machine == EM_X86_64) {
        switch (type) {
            case R_X86_64_NONE: return "R_X86_64_NONE";
            case R_X86_64_64: return "R_X86_64_64";
            case R_X86_64_PC32: return "R_X86_64_PC32";
            case R_X86_64_GOT32: return "R_X86_64_GOT32";
            case R_X86_64_PLT32: return "R_X86_64_PLT32";
            default: return "Unknown";
        }
    } else if (machine == EM_ARM) {
        switch (type) {
            case R_ARM_NONE: return "R_ARM_NONE";
            case R_ARM_PC24: return "R_ARM_PC24";
            case R_ARM_ABS32: return "R_ARM_ABS32";
            case R_ARM_REL32: return "R_ARM_REL32";
            default: return "Unknown";
        }
    }
    
    return "Unknown";
}

} // namespace elf

// ElfSection implementation
ElfSection::ElfSection(ElfObject& parent, const ElfSectionHeader& header, const uint8_t* data, std::string name)
    : parent_(parent)
    , header_(header)
    , name_(std::move(name)) {
    
    // Skip data copy for SHT_NOBITS sections
    if (header.type != elf::SHT_NOBITS && data && header.size > 0) {
        data_ = std::shared_ptr<uint8_t[]>(new uint8_t[header.size]);
        std::memcpy(data_.get(), data, header.size);
    }
}

void ElfSection::setData(const uint8_t* data, uint64_t size) {
    if (header_.type == elf::SHT_NOBITS) {
        return; // Can't set data for NOBITS sections
    }
    
    data_ = std::shared_ptr<uint8_t[]>(new uint8_t[size]);
    std::memcpy(data_.get(), data, size);
    header_.size = size;
}

std::string ElfSection::getString(uint32_t offset) const {
    if (header_.type != elf::SHT_STRTAB || !data_) {
        return "";
    }
    
    if (offset >= header_.size) {
        return "";
    }
    
    return std::string(reinterpret_cast<const char*>(data_.get() + offset));
}

ElfSymbolEntry ElfSection::getSymbol(uint32_t index) const {
    ElfSymbolEntry symbol = {};
    
    if (header_.type != elf::SHT_SYMTAB && header_.type != elf::SHT_DYNSYM) {
        return symbol;
    }
    
    // Calculate the offset based on entry size
    size_t offset = index * header_.entsize;
    
    if (offset + header_.entsize > header_.size || !data_) {
        return symbol;
    }
    
    const uint8_t* symbolData = data_.get() + offset;
    
    // The layout depends on whether it's a 32-bit or 64-bit ELF
    bool is64Bit = parent_.getHeader().is64Bit();
    bool isLittleEndian = parent_.getHeader().isLittleEndian();
    
    if (is64Bit) {
        // 64-bit layout
        if (isLittleEndian) {
            symbol.name = *reinterpret_cast<const uint32_t*>(symbolData);
            symbol.info = *(symbolData + 4);
            symbol.other = *(symbolData + 5);
            symbol.shndx = *reinterpret_cast<const uint16_t*>(symbolData + 6);
            symbol.value = *reinterpret_cast<const uint64_t*>(symbolData + 8);
            symbol.size = *reinterpret_cast<const uint64_t*>(symbolData + 16);
        } else {
            // Big-endian - need byte swapping
            // For simplicity, omitting byte swapping implementation
            // In a real implementation, this would handle byte swapping
        }
    } else {
        // 32-bit layout
        if (isLittleEndian) {
            symbol.name = *reinterpret_cast<const uint32_t*>(symbolData);
            symbol.value = *reinterpret_cast<const uint32_t*>(symbolData + 4);
            symbol.size = *reinterpret_cast<const uint32_t*>(symbolData + 8);
            symbol.info = *(symbolData + 12);
            symbol.other = *(symbolData + 13);
            symbol.shndx = *reinterpret_cast<const uint16_t*>(symbolData + 14);
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    }
    
    return symbol;
}

ElfRelEntry ElfSection::getRel(uint32_t index) const {
    ElfRelEntry rel = {};
    
    if (header_.type != elf::SHT_REL) {
        return rel;
    }
    
    // Calculate the offset based on entry size
    size_t offset = index * header_.entsize;
    
    if (offset + header_.entsize > header_.size || !data_) {
        return rel;
    }
    
    const uint8_t* relData = data_.get() + offset;
    
    // The layout depends on whether it's a 32-bit or 64-bit ELF
    bool is64Bit = parent_.getHeader().is64Bit();
    bool isLittleEndian = parent_.getHeader().isLittleEndian();
    
    if (is64Bit) {
        // 64-bit layout
        if (isLittleEndian) {
            rel.offset = *reinterpret_cast<const uint64_t*>(relData);
            rel.info = *reinterpret_cast<const uint64_t*>(relData + 8);
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    } else {
        // 32-bit layout
        if (isLittleEndian) {
            rel.offset = *reinterpret_cast<const uint32_t*>(relData);
            rel.info = *reinterpret_cast<const uint32_t*>(relData + 4);
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    }
    
    return rel;
}

ElfRelaEntry ElfSection::getRela(uint32_t index) const {
    ElfRelaEntry rela = {};
    
    if (header_.type != elf::SHT_RELA) {
        return rela;
    }
    
    // Calculate the offset based on entry size
    size_t offset = index * header_.entsize;
    
    if (offset + header_.entsize > header_.size || !data_) {
        return rela;
    }
    
    const uint8_t* relaData = data_.get() + offset;
    
    // The layout depends on whether it's a 32-bit or 64-bit ELF
    bool is64Bit = parent_.getHeader().is64Bit();
    bool isLittleEndian = parent_.getHeader().isLittleEndian();
    
    if (is64Bit) {
        // 64-bit layout
        if (isLittleEndian) {
            rela.offset = *reinterpret_cast<const uint64_t*>(relaData);
            rela.info = *reinterpret_cast<const uint64_t*>(relaData + 8);
            rela.addend = *reinterpret_cast<const int64_t*>(relaData + 16);
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    } else {
        // 32-bit layout
        if (isLittleEndian) {
            rela.offset = *reinterpret_cast<const uint32_t*>(relaData);
            rela.info = *reinterpret_cast<const uint32_t*>(relaData + 4);
            rela.addend = *reinterpret_cast<const int32_t*>(relaData + 8);
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    }
    
    return rela;
}

uint32_t ElfSection::getEntryCount() const {
    if (header_.entsize == 0) {
        return 0;
    }
    
    return static_cast<uint32_t>(header_.size / header_.entsize);
}

void ElfSection::setSymbol(uint32_t index, const ElfSymbolEntry& symbol) {
    if (header_.type != elf::SHT_SYMTAB && header_.type != elf::SHT_DYNSYM) {
        return;
    }
    
    size_t offset = index * header_.entsize;
    
    if (offset + header_.entsize > header_.size || !data_) {
        return;
    }
    
    uint8_t* symbolData = data_.get() + offset;
    
    // The layout depends on whether it's a 32-bit or 64-bit ELF
    bool is64Bit = parent_.getHeader().is64Bit();
    bool isLittleEndian = parent_.getHeader().isLittleEndian();
    
    if (is64Bit) {
        // 64-bit layout
        if (isLittleEndian) {
            *reinterpret_cast<uint32_t*>(symbolData) = symbol.name;
            *(symbolData + 4) = symbol.info;
            *(symbolData + 5) = symbol.other;
            *reinterpret_cast<uint16_t*>(symbolData + 6) = symbol.shndx;
            *reinterpret_cast<uint64_t*>(symbolData + 8) = symbol.value;
            *reinterpret_cast<uint64_t*>(symbolData + 16) = symbol.size;
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    } else {
        // 32-bit layout
        if (isLittleEndian) {
            *reinterpret_cast<uint32_t*>(symbolData) = symbol.name;
            *reinterpret_cast<uint32_t*>(symbolData + 4) = static_cast<uint32_t>(symbol.value);
            *reinterpret_cast<uint32_t*>(symbolData + 8) = static_cast<uint32_t>(symbol.size);
            *(symbolData + 12) = symbol.info;
            *(symbolData + 13) = symbol.other;
            *reinterpret_cast<uint16_t*>(symbolData + 14) = symbol.shndx;
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    }
}

void ElfSection::setRel(uint32_t index, const ElfRelEntry& rel) {
    if (header_.type != elf::SHT_REL) {
        return;
    }
    
    size_t offset = index * header_.entsize;
    
    if (offset + header_.entsize > header_.size || !data_) {
        return;
    }
    
    uint8_t* relData = data_.get() + offset;
    
    // The layout depends on whether it's a 32-bit or 64-bit ELF
    bool is64Bit = parent_.getHeader().is64Bit();
    bool isLittleEndian = parent_.getHeader().isLittleEndian();
    
    if (is64Bit) {
        // 64-bit layout
        if (isLittleEndian) {
            *reinterpret_cast<uint64_t*>(relData) = rel.offset;
            *reinterpret_cast<uint64_t*>(relData + 8) = rel.info;
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    } else {
        // 32-bit layout
        if (isLittleEndian) {
            *reinterpret_cast<uint32_t*>(relData) = static_cast<uint32_t>(rel.offset);
            *reinterpret_cast<uint32_t*>(relData + 4) = static_cast<uint32_t>(rel.info);
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    }
}

void ElfSection::setRela(uint32_t index, const ElfRelaEntry& rela) {
    if (header_.type != elf::SHT_RELA) {
        return;
    }
    
    size_t offset = index * header_.entsize;
    
    if (offset + header_.entsize > header_.size || !data_) {
        return;
    }
    
    uint8_t* relaData = data_.get() + offset;
    
    // The layout depends on whether it's a 32-bit or 64-bit ELF
    bool is64Bit = parent_.getHeader().is64Bit();
    bool isLittleEndian = parent_.getHeader().isLittleEndian();
    
    if (is64Bit) {
        // 64-bit layout
        if (isLittleEndian) {
            *reinterpret_cast<uint64_t*>(relaData) = rela.offset;
            *reinterpret_cast<uint64_t*>(relaData + 8) = rela.info;
            *reinterpret_cast<int64_t*>(relaData + 16) = rela.addend;
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    } else {
        // 32-bit layout
        if (isLittleEndian) {
            *reinterpret_cast<uint32_t*>(relaData) = static_cast<uint32_t>(rela.offset);
            *reinterpret_cast<uint32_t*>(relaData + 4) = static_cast<uint32_t>(rela.info);
            *reinterpret_cast<int32_t*>(relaData + 8) = static_cast<int32_t>(rela.addend);
        } else {
            // Big-endian - need byte swapping
            // Omitted for simplicity
        }
    }
}

// ElfObject implementation
ElfObject::ElfObject(const Context& ctx)
    : ctx_(ctx) {
    // Initialize ELF header
    std::memset(&header_, 0, sizeof(header_));
    
    // Set the ELF magic bytes
    header_.ident[0] = elf::ELFMAG0;
    header_.ident[1] = elf::ELFMAG1;
    header_.ident[2] = elf::ELFMAG2;
    header_.ident[3] = elf::ELFMAG3;
}

ElfObject* ElfObject::load(Stream& stream, const Context& ctx) {
    if (!stream.isReadable()) {
        ctx.errorManager.addError(ErrorCode::IO, stream.getPosition(), 
                               "Stream is not readable");
        return nullptr;
    }
    
    // Create a new ELF object
    std::unique_ptr<ElfObject> obj(new ElfObject(ctx));
    
    // Read the ELF identification
    StreamReader reader(stream);
    if (reader.read(obj->header_.ident.data(), elf::EI_NIDENT) != elf::EI_NIDENT) {
        ctx.errorManager.addError(ErrorCode::IO, stream.getPosition(), 
                               "Failed to read ELF identification");
        return nullptr;
    }
    
    // Check the ELF magic bytes
    if (obj->header_.ident[0] != elf::ELFMAG0 ||
        obj->header_.ident[1] != elf::ELFMAG1 ||
        obj->header_.ident[2] != elf::ELFMAG2 ||
        obj->header_.ident[3] != elf::ELFMAG3) {
        ctx.errorManager.addError(ErrorCode::Format, stream.getPosition(), 
                               "Invalid ELF magic bytes");
        return nullptr;
    }
    
    // Check the ELF class (32/64 bit)
    if (obj->header_.ident[4] != elf::ELFCLASS32 && obj->header_.ident[4] != elf::ELFCLASS64) {
        ctx.errorManager.addError(ErrorCode::Format, stream.getPosition(), 
                               "Invalid ELF class");
        return nullptr;
    }
    
    // Check the ELF data encoding (little/big endian)
    if (obj->header_.ident[5] != elf::ELFDATA2LSB && obj->header_.ident[5] != elf::ELFDATA2MSB) {
        ctx.errorManager.addError(ErrorCode::Format, stream.getPosition(), 
                               "Invalid ELF data encoding");
        return nullptr;
    }
    
    // Read the rest of the header based on the ELF class
    bool is64Bit = obj->header_.ident[4] == elf::ELFCLASS64;
    // bool isLittleEndian = obj->header_.ident[5] == elf::ELFDATA2LSB;
    
    if (is64Bit) {
        // 64-bit ELF header
        if (!reader.readUint16(&obj->header_.type) ||
            !reader.readUint16(&obj->header_.machine) ||
            !reader.readUint32(&obj->header_.version) ||
            !reader.readUint64(&obj->header_.entry) ||
            !reader.readUint64(&obj->header_.phoff) ||
            !reader.readUint64(&obj->header_.shoff) ||
            !reader.readUint32(&obj->header_.flags) ||
            !reader.readUint16(&obj->header_.ehsize) ||
            !reader.readUint16(&obj->header_.phentsize) ||
            !reader.readUint16(&obj->header_.phnum) ||
            !reader.readUint16(&obj->header_.shentsize) ||
            !reader.readUint16(&obj->header_.shnum) ||
            !reader.readUint16(&obj->header_.shstrndx)) {
            ctx.errorManager.addError(ErrorCode::IO, stream.getPosition(), 
                                   "Failed to read 64-bit ELF header");
            return nullptr;
        }
    } else {
        // 32-bit ELF header
        uint32_t entry32, phoff32, shoff32;
        
        if (!reader.readUint16(&obj->header_.type) ||
            !reader.readUint16(&obj->header_.machine) ||
            !reader.readUint32(&obj->header_.version) ||
            !reader.readUint32(&entry32) ||
            !reader.readUint32(&phoff32) ||
            !reader.readUint32(&shoff32) ||
            !reader.readUint32(&obj->header_.flags) ||
            !reader.readUint16(&obj->header_.ehsize) ||
            !reader.readUint16(&obj->header_.phentsize) ||
            !reader.readUint16(&obj->header_.phnum) ||
            !reader.readUint16(&obj->header_.shentsize) ||
            !reader.readUint16(&obj->header_.shnum) ||
            !reader.readUint16(&obj->header_.shstrndx)) {
            ctx.errorManager.addError(ErrorCode::IO, stream.getPosition(), 
                                   "Failed to read 32-bit ELF header");
            return nullptr;
        }
        
        // Convert 32-bit values to 64-bit
        obj->header_.entry = entry32;
        obj->header_.phoff = phoff32;
        obj->header_.shoff = shoff32;
    }
    
    // Check if we need to byte-swap the values for big-endian
    // For simplicity, we'll assume little-endian host and skip this step
    
    // Load sections
    if (!obj->loadSections(stream)) {
        ctx.errorManager.addError(ErrorCode::IO, stream.getPosition(), 
                               "Failed to load ELF sections");
        return nullptr;
    }
    
    // Load section data
    if (!obj->loadSectionData(stream)) {
        ctx.errorManager.addError(ErrorCode::IO, stream.getPosition(), 
                               "Failed to load ELF section data");
        return nullptr;
    }
    
    // Load section names
    if (!obj->loadSectionNames()) {
        ctx.errorManager.addError(ErrorCode::IO, stream.getPosition(), 
                               "Failed to load ELF section names");
        return nullptr;
    }
    
    return obj.release();
}

ElfObject* ElfObject::create(bool is64Bit, bool isLittleEndian, uint16_t type, 
                             uint16_t machine, const Context& ctx) {
    // Create a new ELF object
    std::unique_ptr<ElfObject> obj(new ElfObject(ctx));
    
    // Set up the ELF header
    obj->header_.ident[4] = is64Bit ? elf::ELFCLASS64 : elf::ELFCLASS32;
    obj->header_.ident[5] = isLittleEndian ? elf::ELFDATA2LSB : elf::ELFDATA2MSB;
    obj->header_.ident[6] = 1; // Version
    obj->header_.type = type;
    obj->header_.machine = machine;
    obj->header_.version = 1;
    obj->header_.entry = 0;
    obj->header_.phoff = 0;
    obj->header_.shoff = 0;
    obj->header_.flags = 0;
    obj->header_.ehsize = is64Bit ? 64 : 52;
    obj->header_.phentsize = 0;
    obj->header_.phnum = 0;
    obj->header_.shentsize = is64Bit ? 64 : 40;
    obj->header_.shnum = 0;
    obj->header_.shstrndx = 0;
    
    // Create a null section (index 0 is always reserved)
    ElfSectionHeader nullHeader = {};
    obj->sections_.push_back(std::make_unique<ElfSection>(*obj, nullHeader, nullptr, ""));
    obj->header_.shnum = 1;
    
    // Create a string table section for section names
    uint8_t initialStringTable[] = {0}; // Initial empty string
    
    ElfSectionHeader strHeader = {};
    strHeader.type = elf::SHT_STRTAB;
    strHeader.size = 1;
    strHeader.addralign = 1;
    
    obj->sections_.push_back(std::make_unique<ElfSection>(*obj, strHeader, initialStringTable, ".shstrtab"));
    obj->header_.shstrndx = 1;
    obj->header_.shnum = 2;
    
    return obj.release();
}

bool ElfObject::loadSections(Stream& stream) {
    if (header_.shnum == 0 || header_.shoff == 0) {
        return true; // No sections to load
    }
    
    // Seek to the section header table
    if (stream.eof()) {
        return false;
    }
    
    // We need to manually seek since Stream doesn't have a seek method
    // This is simplified - in a real implementation, you would have proper seeking
    StreamReader reader(stream);
    
    // Skip to the section header table
    uint64_t bytesToSkip = header_.shoff;
    char skipBuffer[1024];
    
    while (bytesToSkip > 0) {
        size_t toSkip = std::min(bytesToSkip, static_cast<uint64_t>(sizeof(skipBuffer)));
        size_t skipped = reader.read(skipBuffer, toSkip);
        
        if (skipped == 0) {
            return false; // End of stream reached
        }
        
        bytesToSkip -= skipped;
    }
    
    // Read section headers
    sections_.clear();
    
    for (uint16_t i = 0; i < header_.shnum; i++) {
        ElfSectionHeader sectionHeader = {};
        
        // Read section header based on ELF class
        if (header_.is64Bit()) {
            // 64-bit section header
            uint64_t flags64, addr64, offset64, size64, addralign64, entsize64;
            
            if (!reader.readUint32(&sectionHeader.name) ||
                !reader.readUint32(&sectionHeader.type) ||
                !reader.readUint64(&flags64) ||
                !reader.readUint64(&addr64) ||
                !reader.readUint64(&offset64) ||
                !reader.readUint64(&size64) ||
                !reader.readUint32(&sectionHeader.link) ||
                !reader.readUint32(&sectionHeader.info) ||
                !reader.readUint64(&addralign64) ||
                !reader.readUint64(&entsize64)) {
                return false;
            }
            
            sectionHeader.flags = flags64;
            sectionHeader.addr = addr64;
            sectionHeader.offset = offset64;
            sectionHeader.size = size64;
            sectionHeader.addralign = addralign64;
            sectionHeader.entsize = entsize64;
        } else {
            // 32-bit section header
            uint32_t flags32, addr32, offset32, size32, addralign32, entsize32;
            
            if (!reader.readUint32(&sectionHeader.name) ||
                !reader.readUint32(&sectionHeader.type) ||
                !reader.readUint32(&flags32) ||
                !reader.readUint32(&addr32) ||
                !reader.readUint32(&offset32) ||
                !reader.readUint32(&size32) ||
                !reader.readUint32(&sectionHeader.link) ||
                !reader.readUint32(&sectionHeader.info) ||
                !reader.readUint32(&addralign32) ||
                !reader.readUint32(&entsize32)) {
                return false;
            }
            
            sectionHeader.flags = flags32;
            sectionHeader.addr = addr32;
            sectionHeader.offset = offset32;
            sectionHeader.size = size32;
            sectionHeader.addralign = addralign32;
            sectionHeader.entsize = entsize32;
        }
        
        // Add section to the list (with empty name for now)
        sections_.push_back(std::make_unique<ElfSection>(*this, sectionHeader, nullptr, ""));
    }
    
    return true;
}

bool ElfObject::loadSectionData(Stream& stream) {
    for (const auto& section : sections_) {
        const auto& header = section->getHeader();
        
        // Skip sections with no data
        if (header.type == elf::SHT_NOBITS || header.size == 0 || header.offset == 0) {
            continue;
        }
        
        // Seek to the section data
        // This is simplified - in a real implementation, you would have proper seeking
        stream.resetReadPosition();
        StreamReader reader(stream);
        
        // Skip to the section data
        uint64_t bytesToSkip = header.offset;
        char skipBuffer[1024];
        
        while (bytesToSkip > 0) {
            size_t toSkip = std::min(bytesToSkip, static_cast<uint64_t>(sizeof(skipBuffer)));
            size_t skipped = reader.read(skipBuffer, toSkip);
            
            if (skipped == 0) {
                return false; // End of stream reached
            }
            
            bytesToSkip -= skipped;
        }
        
        // Read the section data
        auto data = std::make_shared<uint8_t>(header.size);
        
        if (reader.read(data.get(), header.size) != header.size) {
            return false;
        }
        
        // Set the section data
        section->setData(data.get(), header.size);
    }
    
    return true;
}

bool ElfObject::loadSectionNames() {
    // Make sure we have a valid string table index
    if (header_.shstrndx >= sections_.size()) {
        return false;
    }
    
    // Get the string table section
    const auto& stringSection = sections_[header_.shstrndx];
    
    // Make sure it's a string table
    if (stringSection->getHeader().type != elf::SHT_STRTAB) {
        return false;
    }
    
    // Update section names
    for (const auto& section : sections_) {
        uint32_t nameOffset = section->getHeader().name;
        std::string name = stringSection->getString(nameOffset);
        
        // Use a more advanced technique to update the section name
        // This is a bit of a hack, but it works for our demonstration
        const_cast<std::string&>(section->getName()) = name;
    }
    
    return true;
}

const ElfSection& ElfObject::getSection(uint16_t index) const {
    if (index >= sections_.size()) {
        static ElfSectionHeader emptyHeader = {};
        static ElfSection emptySection(*const_cast<ElfObject*>(this), emptyHeader, nullptr, "");
        return emptySection;
    }
    
    return *sections_[index];
}

const ElfSection* ElfObject::getSectionByName(const std::string& name) const {
    for (const auto& section : sections_) {
        if (section->getName() == name) {
            return section.get();
        }
    }
    
    return nullptr;
}

ElfSection* ElfObject::addSection(const std::string& name, uint32_t type, uint64_t flags, 
                                 const uint8_t* data, uint64_t size, uint64_t entsize) {
    // Find the string table section
    const auto& stringSection = sections_[header_.shstrndx];
    
    // Add the section name to the string table
    ElfStringTable strTable(*stringSection);
    uint32_t nameOffset = strTable.addString(name);
    
    // Update the string table section
    sections_[header_.shstrndx]->setData(strTable.getData(), strTable.getSize());
    
    // Create the new section header
    ElfSectionHeader sectionHeader = {};
    sectionHeader.name = nameOffset;
    sectionHeader.type = type;
    sectionHeader.flags = flags;
    sectionHeader.size = size;
    sectionHeader.entsize = entsize;
    
    // For NOBITS sections, data is not stored in the file
    if (type == elf::SHT_NOBITS) {
        data = nullptr;
        size = 0;
    }
    
    // Add the new section
    sections_.push_back(std::make_unique<ElfSection>(*this, sectionHeader, data, name));
    header_.shnum = static_cast<uint16_t>(sections_.size());
    
    return sections_.back().get();
}

bool ElfObject::save(Stream& stream) {
    if (!stream.isWritable()) {
        ctx_.errorManager.addError(ErrorCode::IO, stream.getPosition(), 
                                "Stream is not writable");
        return false;
    }
    
    // Calculate layout
    uint64_t filePos = 0;
    
    // ELF header
    uint64_t headerSize = header_.is64Bit() ? 64 : 52;
    filePos += headerSize;
    
    // Align to 8 bytes for section data
    filePos = (filePos + 7) & ~7;
    
    // Section data
    for (const auto& section : sections_) {
        if (section->getHeader().type == elf::SHT_NOBITS) {
            continue; // No data for NOBITS sections
        }
        
        // Align section data to its alignment
        uint64_t align = section->getHeader().addralign;
        if (align > 0) {
            filePos = (filePos + align - 1) & ~(align - 1);
        }
        
        // Update section offset
        const_cast<ElfSectionHeader&>(section->getHeader()).offset = filePos;
        
        // Advance file position
        filePos += section->getSize();
    }
    
    // Align to 8 bytes for section headers
    filePos = (filePos + 7) & ~7;
    
    // Update section header offset
    header_.shoff = filePos;
    
    // Write ELF header
    StreamWriter writer(stream);
    
    // Write identification
    if (writer.write(header_.ident.data(), elf::EI_NIDENT) != elf::EI_NIDENT) {
        return false;
    }
    
    // Write rest of header based on ELF class
    if (header_.is64Bit()) {
        // 64-bit ELF header
        if (!writer.writeUint16(header_.type) ||
            !writer.writeUint16(header_.machine) ||
            !writer.writeUint32(header_.version) ||
            !writer.writeUint64(header_.entry) ||
            !writer.writeUint64(header_.phoff) ||
            !writer.writeUint64(header_.shoff) ||
            !writer.writeUint32(header_.flags) ||
            !writer.writeUint16(header_.ehsize) ||
            !writer.writeUint16(header_.phentsize) ||
            !writer.writeUint16(header_.phnum) ||
            !writer.writeUint16(header_.shentsize) ||
            !writer.writeUint16(header_.shnum) ||
            !writer.writeUint16(header_.shstrndx)) {
            return false;
        }
    } else {
        // 32-bit ELF header
        if (!writer.writeUint16(header_.type) ||
            !writer.writeUint16(header_.machine) ||
            !writer.writeUint32(header_.version) ||
            !writer.writeUint32(static_cast<uint32_t>(header_.entry)) ||
            !writer.writeUint32(static_cast<uint32_t>(header_.phoff)) ||
            !writer.writeUint32(static_cast<uint32_t>(header_.shoff)) ||
            !writer.writeUint32(header_.flags) ||
            !writer.writeUint16(header_.ehsize) ||
            !writer.writeUint16(header_.phentsize) ||
            !writer.writeUint16(header_.phnum) ||
            !writer.writeUint16(header_.shentsize) ||
            !writer.writeUint16(header_.shnum) ||
            !writer.writeUint16(header_.shstrndx)) {
            return false;
        }
    }
    
    // Write padding to align section data
    uint64_t paddingSize = (headerSize + 7) & ~7 - headerSize;
    uint8_t padding[8] = {0};
    
    if (paddingSize > 0 && writer.write(padding, paddingSize) != paddingSize) {
        return false;
    }
    
    // Write section data
    for (const auto& section : sections_) {
        if (section->getHeader().type == elf::SHT_NOBITS) {
            continue; // No data for NOBITS sections
        }
        
        // Calculate padding needed for alignment
        uint64_t currentPos = headerSize + paddingSize;
        for (const auto& prevSection : sections_) {
            if (prevSection.get() == section.get()) {
                break;
            }
            
            if (prevSection->getHeader().type != elf::SHT_NOBITS) {
                currentPos += prevSection->getSize();
            }
        }
        
        uint64_t align = section->getHeader().addralign;
        if (align > 0) {
            uint64_t alignedPos = (currentPos + align - 1) & ~(align - 1);
            uint64_t alignPadding = alignedPos - currentPos;
            
            if (alignPadding > 0 && writer.write(padding, alignPadding) != alignPadding) {
                return false;
            }
        }
        
        // Write section data
        if (section->getData() && section->getSize() > 0) {
            if (writer.write(section->getData(), section->getSize()) != section->getSize()) {
                return false;
            }
        }
    }
    
    // Write padding to align section headers
    uint64_t currentPos = headerSize + paddingSize;
    for (const auto& section : sections_) {
        if (section->getHeader().type != elf::SHT_NOBITS) {
            currentPos += section->getSize();
        }
    }
    
    uint64_t alignedPos = (currentPos + 7) & ~7;
    uint64_t alignPadding = alignedPos - currentPos;
    
    if (alignPadding > 0 && writer.write(padding, alignPadding) != alignPadding) {
        return false;
    }
    
    // Write section headers
    for (const auto& section : sections_) {
        const auto& header = section->getHeader();
        
        // Write section header based on ELF class
        if (header_.is64Bit()) {
            // 64-bit section header
            if (!writer.writeUint32(header.name) ||
                !writer.writeUint32(header.type) ||
                !writer.writeUint64(header.flags) ||
                !writer.writeUint64(header.addr) ||
                !writer.writeUint64(header.offset) ||
                !writer.writeUint64(header.size) ||
                !writer.writeUint32(header.link) ||
                !writer.writeUint32(header.info) ||
                !writer.writeUint64(header.addralign) ||
                !writer.writeUint64(header.entsize)) {
                return false;
            }
        } else {
            // 32-bit section header
            if (!writer.writeUint32(header.name) ||
                !writer.writeUint32(header.type) ||
                !writer.writeUint32(static_cast<uint32_t>(header.flags)) ||
                !writer.writeUint32(static_cast<uint32_t>(header.addr)) ||
                !writer.writeUint32(static_cast<uint32_t>(header.offset)) ||
                !writer.writeUint32(static_cast<uint32_t>(header.size)) ||
                !writer.writeUint32(header.link) ||
                !writer.writeUint32(header.info) ||
                !writer.writeUint32(static_cast<uint32_t>(header.addralign)) ||
                !writer.writeUint32(static_cast<uint32_t>(header.entsize))) {
                return false;
            }
        }
    }
    
    return true;
}

void ElfObject::forEachSymbol(std::function<void(const ElfSection&, const ElfSymbolEntry&, const std::string&)> callback) const {
    for (const auto& section : sections_) {
        // Skip non-symbol table sections
        if (section->getHeader().type != elf::SHT_SYMTAB && section->getHeader().type != elf::SHT_DYNSYM) {
            continue;
        }
        
        // Get the associated string table
        const ElfSection* strTabSection = nullptr;
        if (section->getHeader().link < sections_.size()) {
            strTabSection = sections_[section->getHeader().link].get();
        }
        
        if (!strTabSection || strTabSection->getHeader().type != elf::SHT_STRTAB) {
            continue;
        }
        
        // Iterate through symbols
        uint32_t symbolCount = section->getEntryCount();
        
        for (uint32_t i = 0; i < symbolCount; i++) {
            ElfSymbolEntry symbol = section->getSymbol(i);
            std::string name = strTabSection->getString(symbol.name);
            
            callback(*section, symbol, name);
        }
    }
}

void ElfObject::forEachRelocation(std::function<void(const ElfSection&, const ElfSection&, const ElfRelEntry&)> relCallback,
                                 std::function<void(const ElfSection&, const ElfSection&, const ElfRelaEntry&)> relaCallback) const {
    for (const auto& section : sections_) {
        // Handle REL sections
        if (section->getHeader().type == elf::SHT_REL && relCallback) {
            // Get the section being relocated
            const ElfSection* targetSection = nullptr;
            if (section->getHeader().info < sections_.size()) {
                targetSection = sections_[section->getHeader().info].get();
            }
            
            if (!targetSection) {
                continue;
            }
            
            // Iterate through relocations
            uint32_t relCount = section->getEntryCount();
            
            for (uint32_t i = 0; i < relCount; i++) {
                ElfRelEntry rel = section->getRel(i);
                relCallback(*section, *targetSection, rel);
            }
        }
        
        // Handle RELA sections
        if (section->getHeader().type == elf::SHT_RELA && relaCallback) {
            // Get the section being relocated
            const ElfSection* targetSection = nullptr;
            if (section->getHeader().info < sections_.size()) {
                targetSection = sections_[section->getHeader().info].get();
            }
            
            if (!targetSection) {
                continue;
            }
            
            // Iterate through relocations
            uint32_t relaCount = section->getEntryCount();
            
            for (uint32_t i = 0; i < relaCount; i++) {
                ElfRelaEntry rela = section->getRela(i);
                relaCallback(*section, *targetSection, rela);
            }
        }
    }
}

std::pair<const ElfSection*, const ElfSymbolEntry*> ElfObject::findSymbol(const std::string& name) const {
    for (const auto& section : sections_) {
        // Skip non-symbol table sections
        if (section->getHeader().type != elf::SHT_SYMTAB && section->getHeader().type != elf::SHT_DYNSYM) {
            continue;
        }
        
        // Get the associated string table
        const ElfSection* strTabSection = nullptr;
        if (section->getHeader().link < sections_.size()) {
            strTabSection = sections_[section->getHeader().link].get();
        }
        
        if (!strTabSection || strTabSection->getHeader().type != elf::SHT_STRTAB) {
            continue;
        }
        
        // Iterate through symbols
        uint32_t symbolCount = section->getEntryCount();
        
        for (uint32_t i = 0; i < symbolCount; i++) {
            ElfSymbolEntry symbol = section->getSymbol(i);
            std::string symbolName = strTabSection->getString(symbol.name);
            
            if (symbolName == name) {
                // Return a static local in order to avoid returning a pointer to a temporary value
                static ElfSymbolEntry foundSymbol;
                foundSymbol = symbol;
                return {section.get(), &foundSymbol};
            }
        }
    }
    
    return {nullptr, nullptr};
}

// ElfStringTable implementation
ElfStringTable::ElfStringTable(const ElfSection& section) {
    // Initialize with existing data
    const uint8_t* sectionData = section.getData();
    uint64_t sectionSize = section.getSize();
    
    if (sectionData && sectionSize > 0) {
        data_.resize(sectionSize);
        std::memcpy(data_.data(), sectionData, sectionSize);
    } else {
        // Ensure we at least have a null byte for the empty string
        data_.push_back(0);
    }
}

std::string ElfStringTable::getString(uint32_t offset) const {
    if (offset >= data_.size()) {
        return "";
    }
    
    return std::string(reinterpret_cast<const char*>(data_.data() + offset));
}

uint32_t ElfStringTable::addString(const std::string& str) {
    // Check if the string already exists
    for (uint32_t offset = 0; offset < data_.size(); /*no increment*/) {
        std::string existingStr = getString(offset);
        
        if (existingStr == str) {
            return offset;
        }
        
        // Move to the next string
        offset += existingStr.length() + 1;
    }
    
    // If not found, add it
    uint32_t offset = static_cast<uint32_t>(data_.size());
    
    // Append the string plus a null terminator
    data_.insert(data_.end(), str.begin(), str.end());
    data_.push_back(0);
    
    return offset;
}

// Utility functions
namespace obj_util {

std::string getElfTypeString(const ElfHeader& header) {
    std::string result;
    
    // Type
    result += elf::getFileTypeName(header.type);
    result += ", ";
    
    // Machine
    result += elf::getMachineTypeName(header.machine);
    result += ", ";
    
    // Class
    result += header.is64Bit() ? "64-bit" : "32-bit";
    result += ", ";
    
    // Endianness
    result += header.isLittleEndian() ? "little-endian" : "big-endian";
    
    return result;
}

void dumpElfInfo(const ElfObject& obj, Logger& logger) {
    const ElfHeader& header = obj.getHeader();
    
    // Log basic ELF info
    COIL_INFO(logger, "ELF File: %s", getElfTypeString(header).c_str());
    COIL_INFO(logger, "Sections: %u", header.shnum);
    
    // Log sections
    for (uint16_t i = 0; i < header.shnum; i++) {
        const ElfSection& section = obj.getSection(i);
        const ElfSectionHeader& sectionHeader = section.getHeader();
        
        COIL_INFO(logger, "  [%2u] %-20s %-12s %5s %8llu bytes", 
                i, 
                section.getName().c_str(), 
                elf::getSectionTypeName(sectionHeader.type),
                elf::getSectionFlagsString(sectionHeader.flags).c_str(),
                sectionHeader.size);
    }
    
    // Log symbols
    COIL_INFO(logger, "Symbols:");
    
    obj.forEachSymbol([&](const ElfSection& section, const ElfSymbolEntry& symbol, const std::string& name) {
        COIL_INFO(logger, "  %-20s %8llx %5llu %-8s %-8s %s",
                name.c_str(),
                symbol.value,
                symbol.size,
                elf::getSymbolBindingName(symbol.getBinding()),
                elf::getSymbolTypeName(symbol.getType()),
                symbol.shndx == 0 ? "UND" : obj.getSection(symbol.shndx).getName().c_str());
    });
    
    // Log relocations
    COIL_INFO(logger, "Relocations:");
    
    obj.forEachRelocation(
        // REL relocations
        [&](const ElfSection& section, const ElfSection& targetSection, const ElfRelEntry& rel) {
            // Get the symbol for this relocation
            uint32_t symbolIndex = rel.getSymbol();
            std::string symbolName = "<unknown>";
            
            // Find the symbol table and string table
            const ElfSection* symtabSection = nullptr;
            if (section.getHeader().link < obj.getSections().size()) {
                symtabSection = obj.getSectionByName(".symtab");
            }
            
            if (symtabSection) {
                // Get the string table
                const ElfSection* strtabSection = nullptr;
                if (symtabSection->getHeader().link < obj.getSections().size()) {
                    strtabSection = &obj.getSection(symtabSection->getHeader().link);
                }
                
                if (strtabSection && symbolIndex < symtabSection->getEntryCount()) {
                    ElfSymbolEntry symbol = symtabSection->getSymbol(symbolIndex);
                    symbolName = strtabSection->getString(symbol.name);
                }
            }
            
            COIL_INFO(logger, "  %-20s +%08llx %s",
                    targetSection.getName().c_str(),
                    rel.offset,
                    symbolName.c_str());
        },
        
        // RELA relocations
        [&](const ElfSection& section, const ElfSection& targetSection, const ElfRelaEntry& rela) {
            // Similar to REL relocations, but with an addend
            uint32_t symbolIndex = rela.getSymbol();
            std::string symbolName = "<unknown>";
            
            // Find the symbol table and string table
            const ElfSection* symtabSection = nullptr;
            if (section.getHeader().link < obj.getSections().size()) {
                symtabSection = obj.getSectionByName(".symtab");
            }
            
            if (symtabSection) {
                // Get the string table
                const ElfSection* strtabSection = nullptr;
                if (symtabSection->getHeader().link < obj.getSections().size()) {
                    strtabSection = &obj.getSection(symtabSection->getHeader().link);
                }
                
                if (strtabSection && symbolIndex < symtabSection->getEntryCount()) {
                    ElfSymbolEntry symbol = symtabSection->getSymbol(symbolIndex);
                    symbolName = strtabSection->getString(symbol.name);
                }
            }
            
            COIL_INFO(logger, "  %-20s +%08llx %s+%lld",
                    targetSection.getName().c_str(),
                    rela.offset,
                    symbolName.c_str(),
                    rela.addend);
        }
    );
}

std::string disassembleSection(const ElfSection& section, uint16_t machine) {
    (void)machine;
    // This is a placeholder - a real implementation would use a disassembler library
    std::string result = "Disassembly of section " + section.getName() + ":\n";
    
    // For demonstration, just dump hex bytes
    const uint8_t* data = section.getData();
    uint64_t size = section.getSize();
    
    if (!data || size == 0) {
        return result + "  <no data>\n";
    }
    
    for (uint64_t i = 0; i < size; i += 16) {
        char line[128];
        char hexPart[64] = {0};
        char asciiPart[32] = {0};
        
        for (uint64_t j = 0; j < 16 && i + j < size; j++) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02x ", data[i + j]);
            strcat(hexPart, hex);
            
            char ascii = (data[i + j] >= 32 && data[i + j] <= 126) ? data[i + j] : '.';
            asciiPart[j] = ascii;
            asciiPart[j + 1] = 0;
        }
        
        snprintf(line, sizeof(line), "  %08lx: %-48s %s\n", i, hexPart, asciiPart);
        result += line;
    }
    
    return result;
}

bool isCompatible(const ElfObject& obj) {
    const ElfHeader& header = obj.getHeader();
    
    // Check architecture
    #if defined(__x86_64__)
    if (header.machine != elf::EM_X86_64) {
        return false;
    }
    #elif defined(__i386__)
    if (header.machine != elf::EM_386) {
        return false;
    }
    #elif defined(__arm__)
    if (header.machine != elf::EM_ARM) {
        return false;
    }
    #elif defined(__aarch64__)
    if (header.machine != elf::EM_AARCH64) {
        return false;
    }
    #else
    // Unknown architecture
    return false;
    #endif
    
    // Check endianness
    #if defined(__LITTLE_ENDIAN__)
    if (!header.isLittleEndian()) {
        return false;
    }
    #elif defined(__BIG_ENDIAN__)
    if (header.isLittleEndian()) {
        return false;
    }
    #endif
    
    // Check if it's an executable or shared object
    if (header.type != elf::ET_EXEC && header.type != elf::ET_DYN) {
        return false;
    }
    
    return true;
}

bool applyRelocations(ElfObject& obj, ElfSection& targetSection, const ElfSection& relocationSection) {
    // This is a simplified implementation that would need to be expanded for a real ELF manipulator
    
    // Get the symbol table
    const ElfSection* symbolTable = nullptr;
    if (relocationSection.getHeader().link < obj.getSections().size()) {
        symbolTable = &obj.getSection(relocationSection.getHeader().link);
    }
    
    if (!symbolTable || (symbolTable->getHeader().type != elf::SHT_SYMTAB && 
                         symbolTable->getHeader().type != elf::SHT_DYNSYM)) {
        return false;
    }
    
    // Get the string table
    const ElfSection* stringTable = nullptr;
    if (symbolTable->getHeader().link < obj.getSections().size()) {
        stringTable = &obj.getSection(symbolTable->getHeader().link);
    }
    
    if (!stringTable || stringTable->getHeader().type != elf::SHT_STRTAB) {
        return false;
    }
    
    // Process relocations
    uint32_t relCount = relocationSection.getEntryCount();
    uint16_t machine = obj.getHeader().machine;
    
    // Check if it's REL or RELA
    if (relocationSection.getHeader().type == elf::SHT_REL) {
        // Process REL entries
        for (uint32_t i = 0; i < relCount; i++) {
            ElfRelEntry rel = relocationSection.getRel(i);
            uint32_t symbolIndex = rel.getSymbol();
            uint32_t type = rel.getType();
            
            // Get the symbol
            ElfSymbolEntry symbol = symbolTable->getSymbol(symbolIndex);
            std::string symbolName = stringTable->getString(symbol.name);
            
            // Apply the relocation
            // This would depend on the architecture and relocation type
            // For simplicity, we'll just log what we're doing
            
            // In a real implementation, you would modify the target section's data here
            
            // For example, for x86_64 R_X86_64_64 (absolute 64-bit):
            if (machine == elf::EM_X86_64 && type == elf::R_X86_64_64) {
                if (rel.offset + 8 <= targetSection.getSize()) {
                    // Apply relocation: S
                    uint64_t value = symbol.value;
                    
                    // Modify the target section data
                    uint8_t* data = const_cast<uint8_t*>(targetSection.getData());
                    if (data) {
                        *reinterpret_cast<uint64_t*>(data + rel.offset) = value;
                    }
                }
            }
        }
    } else if (relocationSection.getHeader().type == elf::SHT_RELA) {
        // Process RELA entries
        for (uint32_t i = 0; i < relCount; i++) {
            ElfRelaEntry rela = relocationSection.getRela(i);
            uint32_t symbolIndex = rela.getSymbol();
            uint32_t type = rela.getType();
            
            // Get the symbol
            ElfSymbolEntry symbol = symbolTable->getSymbol(symbolIndex);
            std::string symbolName = stringTable->getString(symbol.name);
            
            // Apply the relocation
            // For example, for x86_64 R_X86_64_64 (absolute 64-bit):
            if (machine == elf::EM_X86_64 && type == elf::R_X86_64_64) {
                if (rela.offset + 8 <= targetSection.getSize()) {
                    // Apply relocation: S + A
                    uint64_t value = symbol.value + rela.addend;
                    
                    // Modify the target section data
                    uint8_t* data = const_cast<uint8_t*>(targetSection.getData());
                    if (data) {
                        *reinterpret_cast<uint64_t*>(data + rela.offset) = value;
                    }
                }
            }
        }
    } else {
        // Not a relocation section
        return false;
    }
    
    return true;
}

} // namespace obj_util

} // namespace coil