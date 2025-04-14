#include <catch2/catch_all.hpp>
#include <cstdio>
#include <string>
#include <vector>
#include "coil/obj.hpp"
#include "coil/log.hpp"
#include "coil/err.hpp"
#include "coil/stream.hpp"

// For capturing log output
class CaptureBuffer {
public:
    static constexpr size_t BUFFER_SIZE = 4096;
    
    CaptureBuffer() {
        memset(buffer, 0, BUFFER_SIZE);
        fp = fmemopen(buffer, BUFFER_SIZE, "w+");
    }
    
    ~CaptureBuffer() {
        if (fp) fclose(fp);
    }
    
    FILE* getFile() const { return fp; }
    
    const char* getBuffer() const { return buffer; }
    
    void clear() {
        fclose(fp);
        memset(buffer, 0, BUFFER_SIZE);
        fp = fmemopen(buffer, BUFFER_SIZE, "w+");
    }
    
    bool contains(const std::string& str) const {
        return strstr(buffer, str.c_str()) != nullptr;
    }
    
private:
    char buffer[BUFFER_SIZE];
    FILE* fp;
};

// Create a test context
coil::Context createObjTestContext() {
    static CaptureBuffer capture;
    static coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Info, false);
    static coil::ErrorManager errorMgr(logger);
    return {logger, errorMgr};
}

// Create a memory stream with ELF data for testing
coil::MemoryStream* createElfTestStream(const coil::Context& ctx) {
    // Create a simple 64-bit ELF file (just the header)
    std::vector<uint8_t> elfData = {
        // ELF magic bytes
        0x7F, 'E', 'L', 'F',
        // Class (64-bit)
        2,
        // Data encoding (little-endian)
        1,
        // Version
        1,
        // OS ABI (System V)
        0,
        // ABI Version
        0,
        // Padding
        0, 0, 0, 0, 0, 0, 0,
        // Type (ET_REL - relocatable file)
        1, 0,
        // Machine (EM_X86_64 - AMD x86-64)
        0x3E, 0,
        // Version
        1, 0, 0, 0,
        // Entry point
        0, 0, 0, 0, 0, 0, 0, 0,
        // Program header offset
        0, 0, 0, 0, 0, 0, 0, 0,
        // Section header offset
        0x40, 0, 0, 0, 0, 0, 0, 0,
        // Flags
        0, 0, 0, 0,
        // ELF header size
        0x40, 0,
        // Program header entry size
        0, 0,
        // Program header entries count
        0, 0,
        // Section header entry size
        0x40, 0,
        // Section header entries count
        1, 0,
        // String table index
        0, 0
    };
    
    // Add a simple section header (null section)
    std::vector<uint8_t> nullSection = {
        // Name
        0, 0, 0, 0,
        // Type (SHT_NULL)
        0, 0, 0, 0,
        // Flags
        0, 0, 0, 0, 0, 0, 0, 0,
        // Address
        0, 0, 0, 0, 0, 0, 0, 0,
        // Offset
        0, 0, 0, 0, 0, 0, 0, 0,
        // Size
        0, 0, 0, 0, 0, 0, 0, 0,
        // Link
        0, 0, 0, 0,
        // Info
        0, 0, 0, 0,
        // Alignment
        0, 0, 0, 0, 0, 0, 0, 0,
        // Entry size
        0, 0, 0, 0, 0, 0, 0, 0
    };
    
    // Combine the ELF header and section header
    elfData.insert(elfData.end(), nullSection.begin(), nullSection.end());
    
    // Create a memory stream with the ELF data
    return coil::MemoryStream::create(
        elfData.data(), elfData.size(), 
        coil::StreamFlags::Read | coil::StreamFlags::Write, 
        ctx);
}

TEST_CASE("ELF utility functions", "[elf][obj]") {
    SECTION("ELF file type names") {
        REQUIRE(std::string(coil::elf::getFileTypeName(coil::elf::ET_NONE)) == "None");
        REQUIRE(std::string(coil::elf::getFileTypeName(coil::elf::ET_REL)) == "Relocatable");
        REQUIRE(std::string(coil::elf::getFileTypeName(coil::elf::ET_EXEC)) == "Executable");
        REQUIRE(std::string(coil::elf::getFileTypeName(coil::elf::ET_DYN)) == "Shared object");
        REQUIRE(std::string(coil::elf::getFileTypeName(coil::elf::ET_CORE)) == "Core");
    }
    
    SECTION("ELF machine type names") {
        REQUIRE(std::string(coil::elf::getMachineTypeName(coil::elf::EM_NONE)) == "None");
        REQUIRE(std::string(coil::elf::getMachineTypeName(coil::elf::EM_386)) == "Intel 80386");
        REQUIRE(std::string(coil::elf::getMachineTypeName(coil::elf::EM_ARM)) == "ARM");
        REQUIRE(std::string(coil::elf::getMachineTypeName(coil::elf::EM_X86_64)) == "AMD x86-64");
        REQUIRE(std::string(coil::elf::getMachineTypeName(coil::elf::EM_AARCH64)) == "ARM AARCH64");
    }
    
    SECTION("ELF section type names") {
        REQUIRE(std::string(coil::elf::getSectionTypeName(coil::elf::SHT_NULL)) == "NULL");
        REQUIRE(std::string(coil::elf::getSectionTypeName(coil::elf::SHT_PROGBITS)) == "PROGBITS");
        REQUIRE(std::string(coil::elf::getSectionTypeName(coil::elf::SHT_SYMTAB)) == "SYMTAB");
        REQUIRE(std::string(coil::elf::getSectionTypeName(coil::elf::SHT_STRTAB)) == "STRTAB");
    }
    
    SECTION("ELF section flags string") {
        REQUIRE(coil::elf::getSectionFlagsString(0) == "");
        REQUIRE(coil::elf::getSectionFlagsString(coil::elf::SHF_WRITE) == "W");
        REQUIRE(coil::elf::getSectionFlagsString(coil::elf::SHF_ALLOC) == "A");
        REQUIRE(coil::elf::getSectionFlagsString(coil::elf::SHF_EXECINSTR) == "X");
        REQUIRE(coil::elf::getSectionFlagsString(coil::elf::SHF_WRITE | coil::elf::SHF_ALLOC) == "WA");
        REQUIRE(coil::elf::getSectionFlagsString(coil::elf::SHF_WRITE | coil::elf::SHF_ALLOC | coil::elf::SHF_EXECINSTR) == "WAX");
    }
    
    SECTION("ELF symbol binding names") {
        REQUIRE(std::string(coil::elf::getSymbolBindingName(coil::elf::STB_LOCAL)) == "LOCAL");
        REQUIRE(std::string(coil::elf::getSymbolBindingName(coil::elf::STB_GLOBAL)) == "GLOBAL");
        REQUIRE(std::string(coil::elf::getSymbolBindingName(coil::elf::STB_WEAK)) == "WEAK");
    }
    
    SECTION("ELF symbol type names") {
        REQUIRE(std::string(coil::elf::getSymbolTypeName(coil::elf::STT_NOTYPE)) == "NOTYPE");
        REQUIRE(std::string(coil::elf::getSymbolTypeName(coil::elf::STT_OBJECT)) == "OBJECT");
        REQUIRE(std::string(coil::elf::getSymbolTypeName(coil::elf::STT_FUNC)) == "FUNC");
        REQUIRE(std::string(coil::elf::getSymbolTypeName(coil::elf::STT_SECTION)) == "SECTION");
        REQUIRE(std::string(coil::elf::getSymbolTypeName(coil::elf::STT_FILE)) == "FILE");
    }
    
    SECTION("ELF relocation type names") {
        REQUIRE(std::string(coil::elf::getRelocationTypeName(coil::elf::EM_X86_64, coil::elf::R_X86_64_NONE)) == "R_X86_64_NONE");
        REQUIRE(std::string(coil::elf::getRelocationTypeName(coil::elf::EM_X86_64, coil::elf::R_X86_64_64)) == "R_X86_64_64");
        REQUIRE(std::string(coil::elf::getRelocationTypeName(coil::elf::EM_ARM, coil::elf::R_ARM_NONE)) == "R_ARM_NONE");
        REQUIRE(std::string(coil::elf::getRelocationTypeName(coil::elf::EM_ARM, coil::elf::R_ARM_ABS32)) == "R_ARM_ABS32");
    }
}

TEST_CASE("ELF object creation and manipulation", "[elf][obj]") {
    auto ctx = createObjTestContext();
    
    SECTION("Create a new ELF object") {
        // Create a 64-bit, little-endian, relocatable ELF file for x86-64
        std::unique_ptr<coil::ElfObject> obj(coil::ElfObject::create(
            true, true, coil::elf::ET_REL, coil::elf::EM_X86_64, ctx));
        
        REQUIRE(obj != nullptr);
        
        // Check header
        const coil::ElfHeader& header = obj->getHeader();
        REQUIRE(header.is64Bit());
        REQUIRE(header.isLittleEndian());
        REQUIRE(header.type == coil::elf::ET_REL);
        REQUIRE(header.machine == coil::elf::EM_X86_64);
        
        // Should have 2 sections by default (null section and .shstrtab)
        REQUIRE(header.shnum == 2);
        REQUIRE(obj->getSections().size() == 2);
        
        // Check null section
        const coil::ElfSection& nullSection = obj->getSection(0);
        REQUIRE(nullSection.getName() == "");
        REQUIRE(nullSection.getType() == coil::elf::SHT_NULL);
        
        // Check string table section
        const coil::ElfSection& strSection = obj->getSection(1);
        REQUIRE(strSection.getName() == ".shstrtab");
        REQUIRE(strSection.getType() == coil::elf::SHT_STRTAB);
    }
    
    SECTION("Add sections to an ELF object") {
        // Create a new ELF object
        std::unique_ptr<coil::ElfObject> obj(coil::ElfObject::create(
            true, true, coil::elf::ET_REL, coil::elf::EM_X86_64, ctx));
        
        REQUIRE(obj != nullptr);
        
        // Add a text section
        const char* textData = "Hello, ELF world!";
        coil::ElfSection* textSection = obj->addSection(
            ".text", 
            coil::elf::SHT_PROGBITS, 
            coil::elf::SHF_ALLOC | coil::elf::SHF_EXECINSTR,
            reinterpret_cast<const uint8_t*>(textData),
            strlen(textData),
            0);
        
        REQUIRE(textSection != nullptr);
        REQUIRE(textSection->getName() == ".text");
        REQUIRE(textSection->getType() == coil::elf::SHT_PROGBITS);
        REQUIRE(textSection->getFlags() == (coil::elf::SHF_ALLOC | coil::elf::SHF_EXECINSTR));
        REQUIRE(textSection->getSize() == strlen(textData));
        
        // Add a data section
        uint32_t dataValue = 42;
        coil::ElfSection* dataSection = obj->addSection(
            ".data",
            coil::elf::SHT_PROGBITS,
            coil::elf::SHF_ALLOC | coil::elf::SHF_WRITE,
            reinterpret_cast<const uint8_t*>(&dataValue),
            sizeof(dataValue),
            0);
        
        REQUIRE(dataSection != nullptr);
        REQUIRE(dataSection->getName() == ".data");
        REQUIRE(dataSection->getType() == coil::elf::SHT_PROGBITS);
        REQUIRE(dataSection->getFlags() == (coil::elf::SHF_ALLOC | coil::elf::SHF_WRITE));
        REQUIRE(dataSection->getSize() == sizeof(dataValue));
        
        // Add a BSS section (no data)
        coil::ElfSection* bssSection = obj->addSection(
            ".bss",
            coil::elf::SHT_NOBITS,
            coil::elf::SHF_ALLOC | coil::elf::SHF_WRITE,
            nullptr,
            128,  // Size of the BSS section
            0);
        
        REQUIRE(bssSection != nullptr);
        REQUIRE(bssSection->getName() == ".bss");
        REQUIRE(bssSection->getType() == coil::elf::SHT_NOBITS);
        REQUIRE(bssSection->getFlags() == (coil::elf::SHF_ALLOC | coil::elf::SHF_WRITE));
        REQUIRE(bssSection->getSize() == 128);
        REQUIRE(bssSection->getData() == nullptr);  // NOBITS sections have no data
        
        // Check that we have 5 sections total now
        // (null section, .shstrtab, .text, .data, .bss)
        REQUIRE(obj->getHeader().shnum == 5);
    }
    
    SECTION("Create a memory stream and save ELF object") {
        // Create a new ELF object
        std::unique_ptr<coil::ElfObject> obj(coil::ElfObject::create(
            true, true, coil::elf::ET_REL, coil::elf::EM_X86_64, ctx));
        
        // Add a simple text section
        const char* textData = "Hello, ELF world!";
        obj->addSection(
            ".text", 
            coil::elf::SHT_PROGBITS, 
            coil::elf::SHF_ALLOC | coil::elf::SHF_EXECINSTR,
            reinterpret_cast<const uint8_t*>(textData),
            strlen(textData),
            0);
        
        // Create a memory stream to save to
        std::unique_ptr<coil::MemoryStream> stream(coil::MemoryStream::create(
            nullptr, 1024, coil::StreamFlags::Read | coil::StreamFlags::Write, ctx));
        
        REQUIRE(stream != nullptr);
        
        // Save the ELF object to the stream
        REQUIRE(obj->save(*stream));
        
        // Check that data was written
        REQUIRE(stream->getWriteOffset() > 0);
        
        // Create a new ELF object from the stream
        stream->resetReadPosition();
        std::unique_ptr<coil::ElfObject> loadedObj(coil::ElfObject::load(*stream, ctx));
        
        REQUIRE(loadedObj != nullptr);
        
        // Check that it has the same properties
        const coil::ElfHeader& header = loadedObj->getHeader();
        REQUIRE(header.is64Bit());
        REQUIRE(header.isLittleEndian());
        REQUIRE(header.type == coil::elf::ET_REL);
        REQUIRE(header.machine == coil::elf::EM_X86_64);
        
        // Should have the same number of sections
        REQUIRE(loadedObj->getSections().size() == obj->getSections().size());
        
        // Check for the text section
        const coil::ElfSection* textSection = loadedObj->getSectionByName(".text");
        REQUIRE(textSection != nullptr);
        REQUIRE(textSection->getSize() == strlen(textData));
        
        // Verify the text section data
        const uint8_t* data = textSection->getData();
        REQUIRE(data != nullptr);
        REQUIRE(memcmp(data, textData, strlen(textData)) == 0);
    }
}

TEST_CASE("ELF string table", "[elf][obj]") {
    auto ctx = createObjTestContext();
    
    // Create a new ELF object
    std::unique_ptr<coil::ElfObject> obj(coil::ElfObject::create(
        true, true, coil::elf::ET_REL, coil::elf::EM_X86_64, ctx));
    
    // Get the string table section
    const coil::ElfSection& strSection = obj->getSection(obj->getHeader().shstrndx);
    
    // Create a string table from it
    coil::ElfStringTable strTable(strSection);
    
    SECTION("String table initial state") {
        // Should have the empty string at index 0
        REQUIRE(strTable.getString(0) == "");
        REQUIRE(strTable.getSize() > 0);
    }
    
    SECTION("Add strings to the table") {
        // Add some strings
        uint32_t offset1 = strTable.addString("First string");
        uint32_t offset2 = strTable.addString("Second string");
        uint32_t offset3 = strTable.addString("Third string");
        
        // Check that they were added correctly
        REQUIRE(offset1 > 0);
        REQUIRE(offset2 > offset1);
        REQUIRE(offset3 > offset2);
        
        REQUIRE(strTable.getString(offset1) == "First string");
        REQUIRE(strTable.getString(offset2) == "Second string");
        REQUIRE(strTable.getString(offset3) == "Third string");
        
        // Adding an existing string should return the same offset
        uint32_t offsetDup = strTable.addString("Second string");
        REQUIRE(offsetDup == offset2);
    }
}

TEST_CASE("ELF utility functions 2", "[elf][obj]") {
    auto ctx = createObjTestContext();
    
    // Create a simple 64-bit ELF object
    std::unique_ptr<coil::ElfObject> obj(coil::ElfObject::create(
        true, true, coil::elf::ET_REL, coil::elf::EM_X86_64, ctx));
    
    // Add a text section
    const char* code = "\x48\x89\xe5\x48\x83\xec\x10\xc3"; // Some x86-64 assembly
    obj->addSection(
        ".text", 
        coil::elf::SHT_PROGBITS, 
        coil::elf::SHF_ALLOC | coil::elf::SHF_EXECINSTR,
        reinterpret_cast<const uint8_t*>(code),
        8,
        0);
    
    SECTION("Get ELF type string") {
        std::string typeStr = coil::obj_util::getElfTypeString(obj->getHeader());
        REQUIRE(typeStr.find("Relocatable") != std::string::npos);
        REQUIRE(typeStr.find("AMD x86-64") != std::string::npos);
        REQUIRE(typeStr.find("64-bit") != std::string::npos);
        REQUIRE(typeStr.find("little-endian") != std::string::npos);
    }
    
    SECTION("Dump ELF info") {
        CaptureBuffer capture;
        coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Info, false);
        
        coil::obj_util::dumpElfInfo(*obj, logger);
        
        // Check that the output contains various information
        REQUIRE(capture.contains("ELF File:"));
        REQUIRE(capture.contains("Relocatable"));
        REQUIRE(capture.contains("x86-64"));
        REQUIRE(capture.contains("Sections:"));
        REQUIRE(capture.contains(".text"));
        REQUIRE(capture.contains(".shstrtab"));
    }
    
    SECTION("Disassemble section") {
        const coil::ElfSection* textSection = obj->getSectionByName(".text");
        REQUIRE(textSection != nullptr);
        
        std::string disasm = coil::obj_util::disassembleSection(*textSection, obj->getHeader().machine);
        
        // Check that it contains the section name and some bytes
        REQUIRE(disasm.find("Disassembly of section .text") != std::string::npos);
        REQUIRE(disasm.find("48 89 e5") != std::string::npos);
    }
}