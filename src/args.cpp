#include "coil/args.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cctype>

namespace coil {

ArgParser::ArgParser(
    const std::string& programName,
    const std::string& programDescription,
    const std::string& epilog,
    MemoryArenaPtr arena,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<ErrorManager> errorMgr)
    : programName_(programName)
    , programDescription_(programDescription)
    , epilog_(epilog)
    , arena_(arena ? arena : globalArena)
    , logger_(logger ? logger : defaultLogger)
    , errorMgr_(errorMgr ? errorMgr : defaultErrorManager) {
}

std::shared_ptr<ArgParser> ArgParser::create(
    const std::string& programName,
    const std::string& programDescription,
    const std::string& epilog,
    MemoryArenaPtr arena,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<ErrorManager> errorMgr) {
    
    return std::shared_ptr<ArgParser>(new ArgParser(
        programName, programDescription, epilog, arena, logger, errorMgr));
}

std::optional<size_t> ArgParser::findArgument(const std::string& name) const {
    // Check if this is a short or long option
    if (name.empty()) return std::nullopt;
    
    if (name[0] == '-') {
        // This is an option
        if (name.size() > 1 && name[1] == '-') {
            // Long option (--name)
            std::string longName = name.substr(2);
            for (size_t i = 0; i < args_.size(); ++i) {
                if (args_[i].longName == longName) {
                    return i;
                }
            }
        } else {
            // Short option (-n)
            char shortName = name[1];
            for (size_t i = 0; i < args_.size(); ++i) {
                if (args_[i].shortName == shortName) {
                    return i;
                }
            }
        }
    } else {
        // This is a name without dashes
        for (size_t i = 0; i < args_.size(); ++i) {
            if (args_[i].longName == name || 
                (args_[i].shortName != 0 && name.size() == 1 && args_[i].shortName == name[0])) {
                return i;
            }
        }
    }
    
    return std::nullopt;
}

bool ArgParser::addFlag(
    char shortName,
    const std::string& longName,
    const std::string& description,
    bool defaultValue) {
    
    ArgDefinition arg;
    arg.shortName = shortName;
    arg.longName = longName;
    arg.description = description;
    arg.type = ArgType::Flag;
    arg.required = false;
    arg.defaultValue = defaultValue;
    arg.metavar = "";
    
    args_.push_back(arg);
    values_.push_back(defaultValue);
    provided_.push_back(false);
    
    return true;
}

bool ArgParser::addString(
    char shortName,
    const std::string& longName,
    const std::string& description,
    const std::string& metavar,
    const std::string& defaultValue,
    bool required) {
    
    ArgDefinition arg;
    arg.shortName = shortName;
    arg.longName = longName;
    arg.description = description;
    arg.type = ArgType::String;
    arg.required = required;
    arg.defaultValue = defaultValue;
    arg.metavar = metavar.empty() ? "STRING" : metavar;
    
    args_.push_back(arg);
    values_.push_back(defaultValue);
    provided_.push_back(false);
    
    return true;
}

bool ArgParser::addInt(
    char shortName,
    const std::string& longName,
    const std::string& description,
    const std::string& metavar,
    int defaultValue,
    bool required) {
    
    ArgDefinition arg;
    arg.shortName = shortName;
    arg.longName = longName;
    arg.description = description;
    arg.type = ArgType::Int;
    arg.required = required;
    arg.defaultValue = defaultValue;
    arg.metavar = metavar.empty() ? "NUMBER" : metavar;
    
    args_.push_back(arg);
    values_.push_back(defaultValue);
    provided_.push_back(false);
    
    return true;
}

bool ArgParser::addFloat(
    char shortName,
    const std::string& longName,
    const std::string& description,
    const std::string& metavar,
    float defaultValue,
    bool required) {
    
    ArgDefinition arg;
    arg.shortName = shortName;
    arg.longName = longName;
    arg.description = description;
    arg.type = ArgType::Float;
    arg.required = required;
    arg.defaultValue = defaultValue;
    arg.metavar = metavar.empty() ? "NUMBER" : metavar;
    
    args_.push_back(arg);
    values_.push_back(defaultValue);
    provided_.push_back(false);
    
    return true;
}

bool ArgParser::addPositional(
    const std::string& name,
    const std::string& description,
    bool required) {
    
    ArgDefinition arg;
    arg.shortName = 0;
    arg.longName = name;
    arg.description = description;
    arg.type = ArgType::Positional;
    arg.required = required;
    arg.defaultValue = std::string();
    arg.metavar = name;
    
    args_.push_back(arg);
    values_.push_back(std::string());
    provided_.push_back(false);
    
    return true;
}

bool ArgParser::parse(int argc, char** argv) {
    if (argc <= 0 || !argv) return false;
    
    // Count positional arguments
    size_t maxPositional = 0;
    for (const auto& arg : args_) {
        if (arg.type == ArgType::Positional) {
            maxPositional++;
        }
    }
    
    // Prepare to collect positional arguments
    positionalArgs_.clear();
    
    // Skip program name
    int i = 1;
    
    // Process all arguments
    while (i < argc) {
        std::string arg = argv[i];
        
        // Check if it's an option
        if (arg[0] == '-') {
            // Handle '--' (end of options)
            if (arg == "--") {
                i++;
                break;  // Everything after this is a positional argument
            }
            
            // Look up the argument
            auto argIndex = findArgument(arg);
            
            if (!argIndex) {
                if (errorMgr_) {
                    StreamPosition pos;
                    pos.fileName = "args";
                    errorMgr_->addError(ErrorCode::Argument, pos, 
                                      "Unknown argument: " + arg);
                }
                return false;
            }
            
            // Mark as provided
            provided_[*argIndex] = true;
            
            // Handle based on type
            switch (args_[*argIndex].type) {
                case ArgType::Flag: {
                    // Flags just need to be set to true
                    values_[*argIndex] = true;
                    
                    // Check if it's help
                    if ((args_[*argIndex].shortName == 'h' && arg == "-h") ||
                        (args_[*argIndex].longName == "help" && arg == "--help")) {
                        helpRequested_ = true;
                    }
                    break;
                } case ArgType::String:
                case ArgType::Int:
                case ArgType::Float: {
                    // These all need a value, either in the next argument or after =
                    if (i + 1 >= argc) {
                        if (errorMgr_) {
                            StreamPosition pos;
                            pos.fileName = "args";
                            errorMgr_->addError(ErrorCode::Argument, pos, 
                                             "Missing value for argument: " + arg);
                        }
                        return false;
                    }
                    
                    std::string value = argv[++i];
                    
                    if (args_[*argIndex].type == ArgType::String) {
                        values_[*argIndex] = value;
                    } else if (args_[*argIndex].type == ArgType::Int) {
                        try {
                            values_[*argIndex] = std::stoi(value);
                        } catch (const std::exception& e) {
                            if (errorMgr_) {
                                StreamPosition pos;
                                pos.fileName = "args";
                                errorMgr_->addError(ErrorCode::Argument, pos, 
                                                 "Invalid integer value for argument " + 
                                                 arg + ": " + value);
                            }
                            return false;
                        }
                    } else if (args_[*argIndex].type == ArgType::Float) {
                        try {
                            values_[*argIndex] = std::stof(value);
                        } catch (const std::exception& e) {
                            if (errorMgr_) {
                                StreamPosition pos;
                                pos.fileName = "args";
                                errorMgr_->addError(ErrorCode::Argument, pos, 
                                                 "Invalid float value for argument " + 
                                                 arg + ": " + value);
                            }
                            return false;
                        }
                    }
                    break;
                } case ArgType::Positional:
                    // Should not get here, positional args don't start with -
                    break;
            }
        } else {
            // Positional argument
            if (positionalArgs_.size() < maxPositional) {
                // Find the next positional argument definition
                size_t posIndex = 0;
                for (size_t j = 0; j < args_.size(); j++) {
                    if (args_[j].type == ArgType::Positional) {
                        if (posIndex == positionalArgs_.size()) {
                            // This is the one we want
                            values_[j] = std::string(arg);
                            provided_[j] = true;
                            break;
                        }
                        posIndex++;
                    }
                }
                
                positionalArgs_.push_back(argv[i]);
            } else {
                if (errorMgr_) {
                    StreamPosition pos;
                    pos.fileName = "args";
                    errorMgr_->addError(ErrorCode::Argument, pos, 
                                      "Too many positional arguments, got: " + arg);
                }
                return false;
            }
        }
        
        i++;
    }
    
    // Handle any remaining arguments as positional (after --)
    while (i < argc) {
        if (positionalArgs_.size() < maxPositional) {
            // Find the next positional argument definition
            size_t posIndex = 0;
            for (size_t j = 0; j < args_.size(); j++) {
                if (args_[j].type == ArgType::Positional) {
                    if (posIndex == positionalArgs_.size()) {
                        // This is the one we want
                        values_[j] = std::string(argv[i]);
                        provided_[j] = true;
                        break;
                    }
                    posIndex++;
                }
            }
            
            positionalArgs_.push_back(argv[i]);
        } else {
            if (errorMgr_) {
                StreamPosition pos;
                pos.fileName = "args";
                errorMgr_->addError(ErrorCode::Argument, pos, 
                                  "Too many positional arguments, got: " + std::string(argv[i]));
            }
            return false;
        }
        
        i++;
    }
    
    return true;
}

void ArgParser::printHelp() const {
    // Calculate the length of the longest argument
    size_t maxArgLen = 0;
    for (const auto& arg : args_) {
        size_t len = 0;
        
        if (arg.type == ArgType::Positional) {
            len = arg.metavar.size();
        } else {
            len = arg.shortName ? 2 : 0;  // -x
            len += (arg.shortName && !arg.longName.empty()) ? 2 : 0;  // ,
            len += !arg.longName.empty() ? 2 + arg.longName.size() : 0;  // --long
            
            if (arg.type != ArgType::Flag) {
                len += 1 + arg.metavar.size();  // =META
            }
        }
        
        maxArgLen = std::max(maxArgLen, len);
    }
    
    // Add padding
    maxArgLen += 2;
    
    // Print usage
    std::cout << "Usage: " << (programName_.empty() ? "program" : programName_);
    
    // Print options in usage
    for (const auto& arg : args_) {
        if (arg.type != ArgType::Positional) {
            if (arg.required) {
                std::cout << " ";
            } else {
                std::cout << " [";
            }
            
            if (arg.shortName) {
                std::cout << "-" << arg.shortName;
            } else if (!arg.longName.empty()) {
                std::cout << "--" << arg.longName;
            }
            
            if (arg.type != ArgType::Flag) {
                std::cout << " " << arg.metavar;
            }
            
            if (!arg.required) {
                std::cout << "]";
            }
        }
    }
    
    // Print positional arguments in usage
    for (const auto& arg : args_) {
        if (arg.type == ArgType::Positional) {
            if (arg.required) {
                std::cout << " " << arg.metavar;
            } else {
                std::cout << " [" << arg.metavar << "]";
            }
        }
    }
    
    std::cout << "\n\n";
    
    // Print description
    if (!programDescription_.empty()) {
        std::cout << programDescription_ << "\n\n";
    }
    
    // Print arguments
    std::cout << "Options:\n";
    
    for (const auto& arg : args_) {
        std::cout << "  ";
        
        size_t len = 0;
        
        if (arg.type == ArgType::Positional) {
            std::cout << arg.metavar;
            len = arg.metavar.size();
        } else {
            if (arg.shortName) {
                std::cout << "-" << arg.shortName;
                len += 2;
            }
            
            if (arg.shortName && !arg.longName.empty()) {
                std::cout << ", ";
                len += 2;
            }
            
            if (!arg.longName.empty()) {
                std::cout << "--" << arg.longName;
                len += 2 + arg.longName.size();
            }
            
            if (arg.type != ArgType::Flag) {
                std::cout << "=" << arg.metavar;
                len += 1 + arg.metavar.size();
            }
        }
        
        // Pad to align descriptions
        for (size_t j = len; j < maxArgLen; j++) {
            std::cout << " ";
        }
        
        std::cout << arg.description;
        
        // For non-flags, show default values
        if (arg.type != ArgType::Flag && arg.type != ArgType::Positional && !arg.required) {
            std::cout << " (default: ";
            
            if (arg.type == ArgType::String) {
                std::cout << "\"" << std::get<std::string>(arg.defaultValue) << "\"";
            } else if (arg.type == ArgType::Int) {
                std::cout << std::get<int>(arg.defaultValue);
            } else if (arg.type == ArgType::Float) {
                std::cout << std::get<float>(arg.defaultValue);
            }
            
            std::cout << ")";
        }
        
        std::cout << "\n";
    }
    
    // Print epilog
    if (!epilog_.empty()) {
        std::cout << "\n" << epilog_ << "\n";
    }
}

bool ArgParser::getFlag(const std::string& name) const {
    auto index = findArgument(name);
    if (!index || args_[*index].type != ArgType::Flag) {
        return false;
    }
    
    return std::get<bool>(values_[*index]);
}

std::string ArgParser::getString(const std::string& name) const {
    auto index = findArgument(name);
    if (!index || args_[*index].type != ArgType::String) {
        return "";
    }
    
    return std::get<std::string>(values_[*index]);
}

int ArgParser::getInt(const std::string& name) const {
    auto index = findArgument(name);
    if (!index || args_[*index].type != ArgType::Int) {
        return 0;
    }
    
    return std::get<int>(values_[*index]);
}

float ArgParser::getFloat(const std::string& name) const {
    auto index = findArgument(name);
    if (!index || args_[*index].type != ArgType::Float) {
        return 0.0f;
    }
    
    return std::get<float>(values_[*index]);
}

bool ArgParser::wasProvided(const std::string& name) const {
    auto index = findArgument(name);
    if (!index) {
        return false;
    }
    
    return provided_[*index];
}

char const* const* ArgParser::getPositional(size_t* count) const {
    if (count) {
        *count = positionalArgs_.size();
    }
    
    return positionalArgs_.data();
}

bool ArgParser::helpRequested() const {
    return helpRequested_;
}

bool ArgParser::validate() const {
    bool valid = true;
    
    // Check that all required arguments are provided
    for (size_t i = 0; i < args_.size(); i++) {
        if (args_[i].required && !provided_[i]) {
            if (errorMgr_) {
                StreamPosition pos;
                pos.fileName = "args";
                
                if (args_[i].type == ArgType::Positional) {
                    errorMgr_->addError(ErrorCode::Argument, pos, 
                                      "Missing required positional argument: " + 
                                      args_[i].metavar);
                } else {
                    std::string argName;
                    if (!args_[i].longName.empty()) {
                        argName = "--" + args_[i].longName;
                    } else if (args_[i].shortName) {
                        argName = std::string("-") + args_[i].shortName;
                    }
                    
                    errorMgr_->addError(ErrorCode::Argument, pos, 
                                      "Missing required argument: " + argName);
                }
            }
            
            valid = false;
        }
    }
    
    return valid;
}

void ArgParser::addStandardArgs() {
    // Add help argument
    addFlag('h', "help", "Show this help message and exit", false);
    
    // Add verbose argument
    addFlag('v', "verbose", "Enable verbose output", false);
    
    // Add quiet argument
    addFlag('q', "quiet", "Suppress all output except errors", false);
}

ArgParser::~ArgParser() {
    // No need to free memory manually as we used shared_ptr and vectors
}

} // namespace coil