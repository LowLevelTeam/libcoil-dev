#pragma once

#include "coil/log.hpp"
#include "coil/err.hpp"
#include "coil/mem.hpp"
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <unordered_map>
#include <functional>

namespace coil {

/**
 * @brief Argument type
 */
enum class ArgType {
    Flag,        ///< Boolean flag (no value)
    String,      ///< String value
    Int,         ///< Integer value
    Float,       ///< Float value
    Positional   ///< Positional argument (not associated with a flag)
};

/**
 * @brief Argument value
 */
using ArgValue = std::variant<bool, std::string, int, float>;

/**
 * @brief Argument definition
 */
struct ArgDefinition {
    char shortName = 0;              ///< Short name (e.g., 'h' for -h)
    std::string longName;            ///< Long name (e.g., "help" for --help)
    std::string description;         ///< Description for help text
    ArgType type = ArgType::String;  ///< Argument type
    bool required = false;           ///< Whether the argument is required
    ArgValue defaultValue;           ///< Default value
    std::string metavar;             ///< Metavariable name for help text
};

/**
 * @brief Modern C++ Argument Parser class
 */
class ArgParser {
public:
    /**
     * @brief Create an argument parser
     * 
     * @param programName Program name
     * @param programDescription Program description
     * @param epilog Text to display after help
     * @param arena Memory arena for allocations
     * @param logger Logger
     * @param errorMgr Error manager
     * @return std::shared_ptr<ArgParser>
     */
    static std::shared_ptr<ArgParser> create(
        const std::string& programName,
        const std::string& programDescription = "",
        const std::string& epilog = "",
        MemoryArenaPtr arena = nullptr,
        std::shared_ptr<Logger> logger = nullptr,
        std::shared_ptr<ErrorManager> errorMgr = nullptr);
    
    /**
     * @brief Add a flag argument
     * 
     * @param shortName Short name
     * @param longName Long name
     * @param description Description
     * @param defaultValue Default value
     * @return true on success
     */
    bool addFlag(
        char shortName,
        const std::string& longName,
        const std::string& description,
        bool defaultValue = false);
    
    /**
     * @brief Add a string argument
     * 
     * @param shortName Short name
     * @param longName Long name
     * @param description Description
     * @param metavar Metavariable name
     * @param defaultValue Default value
     * @param required Whether the argument is required
     * @return true on success
     */
    bool addString(
        char shortName,
        const std::string& longName,
        const std::string& description,
        const std::string& metavar,
        const std::string& defaultValue = "",
        bool required = false);
    
    /**
     * @brief Add an integer argument
     * 
     * @param shortName Short name
     * @param longName Long name
     * @param description Description
     * @param metavar Metavariable name
     * @param defaultValue Default value
     * @param required Whether the argument is required
     * @return true on success
     */
    bool addInt(
        char shortName,
        const std::string& longName,
        const std::string& description,
        const std::string& metavar,
        int defaultValue = 0,
        bool required = false);
    
    /**
     * @brief Add a float argument
     * 
     * @param shortName Short name
     * @param longName Long name
     * @param description Description
     * @param metavar Metavariable name
     * @param defaultValue Default value
     * @param required Whether the argument is required
     * @return true on success
     */
    bool addFloat(
        char shortName,
        const std::string& longName,
        const std::string& description,
        const std::string& metavar,
        float defaultValue = 0.0f,
        bool required = false);
    
    /**
     * @brief Add a positional argument
     * 
     * @param name Name
     * @param description Description
     * @param required Whether the argument is required
     * @return true on success
     */
    bool addPositional(
        const std::string& name,
        const std::string& description,
        bool required = true);
    
    /**
     * @brief Parse command line arguments
     * 
     * @param argc Argument count
     * @param argv Argument values
     * @return true on success
     */
    bool parse(int argc, char** argv);
    
    /**
     * @brief Print help text
     */
    void printHelp() const;
    
    /**
     * @brief Get a flag argument
     * 
     * @param name Argument name
     * @return bool Flag value
     */
    bool getFlag(const std::string& name) const;
    
    /**
     * @brief Get a string argument
     * 
     * @param name Argument name
     * @return std::string String value
     */
    std::string getString(const std::string& name) const;
    
    /**
     * @brief Get an integer argument
     * 
     * @param name Argument name
     * @return int Integer value
     */
    int getInt(const std::string& name) const;
    
    /**
     * @brief Get a float argument
     * 
     * @param name Argument name
     * @return float Float value
     */
    float getFloat(const std::string& name) const;
    
    /**
     * @brief Check if an argument was provided
     * 
     * @param name Argument name
     * @return true if provided
     */
    bool wasProvided(const std::string& name) const;
    
    /**
     * @brief Get positional arguments
     * 
     * @param count Pointer to store count
     * @return char const* const* Array of positional arguments
     */
     char const* const* getPositional(size_t* count) const;
    
    /**
     * @brief Check if help was requested
     * 
     * @return true if help was requested
     */
    bool helpRequested() const;
    
    /**
     * @brief Validate required arguments
     * 
     * @return true if all required arguments are provided
     */
    bool validate() const;
    
    /**
     * @brief Add standard arguments (help, verbose, quiet)
     */
    void addStandardArgs();
    
    /**
     * @brief Destructor
     */
    ~ArgParser();
    
    // Delete copy and move constructors/assignments
    ArgParser(const ArgParser&) = delete;
    ArgParser& operator=(const ArgParser&) = delete;
    ArgParser(ArgParser&&) = delete;
    ArgParser& operator=(ArgParser&&) = delete;

private:
    ArgParser(
        const std::string& programName,
        const std::string& programDescription,
        const std::string& epilog,
        MemoryArenaPtr arena,
        std::shared_ptr<Logger> logger,
        std::shared_ptr<ErrorManager> errorMgr);
    
    std::optional<size_t> findArgument(const std::string& name) const;
    
    std::string programName_;
    std::string programDescription_;
    std::string epilog_;
    MemoryArenaPtr arena_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<ErrorManager> errorMgr_;
    
    std::vector<ArgDefinition> args_;
    std::vector<ArgValue> values_;
    std::vector<bool> provided_;
    
    std::vector<const char*> positionalArgs_;
    bool helpRequested_ = false;
};

} // namespace coil