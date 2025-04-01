#ifndef COIL_VARIABLE_SYSTEM_H
#define COIL_VARIABLE_SYSTEM_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <optional>

namespace coil {

/**
* Variable class represents a variable in COIL
*/
class Variable {
public:
  // Default constructor
  Variable() = default;
  
  // Constructor with ID, type, and optional initial value
  Variable(uint16_t id, uint16_t type, const std::vector<uint8_t>& initialValue = {});
  
  // Getters
  uint16_t getId() const { return id_; }
  uint16_t getType() const { return type_; }
  uint32_t getScopeLevel() const { return scopeLevel_; }
  const std::vector<uint8_t>& getInitialValue() const { return initialValue_; }
  
  // Setters
  void setScopeLevel(uint32_t level) { scopeLevel_ = level; }
  
  // Check if variable is initialized
  bool isInitialized() const { return !initialValue_.empty(); }
  
  // Create a variable declaration binary encoding
  std::vector<uint8_t> createDeclaration() const;
  
  // Encode variable to binary
  std::vector<uint8_t> encode() const;
  
  // Decode variable from binary
  static Variable decode(const std::vector<uint8_t>& data, size_t& offset);
  
private:
  uint16_t id_ = 0;                    // Variable ID
  uint16_t type_ = 0;                  // Variable type
  uint32_t scopeLevel_ = 0;            // Scope level where this variable is defined
  std::vector<uint8_t> initialValue_;  // Initial value (if any)
};

/**
* Scope manager to handle variable scopes
*/
class ScopeManager {
public:
  // Constructor
  ScopeManager();
  
  // Enter a new scope
  void enterScope();
  
  // Leave current scope
  void leaveScope();
  
  // Add variable to current scope
  void addVariable(const Variable& var);
  
  // Find variable by ID
  const Variable* findVariable(uint16_t id) const;
  
  // Get current scope level
  uint32_t getCurrentScopeLevel() const { return currentScopeLevel_; }
  
  // Get all variables in the current scope
  std::vector<Variable> getCurrentScopeVariables() const;
  
  // Get all variables in all scopes
  std::vector<Variable> getAllVariables() const;
  
  // Clear all scopes
  void clear();
  
private:
  uint32_t currentScopeLevel_;
  std::vector<std::unordered_map<uint16_t, Variable>> scopes_;
};

/**
* Variable manager to track variables across a COIL module
*/
class VariableManager {
public:
  // Constructor
  VariableManager();
  
  // Create a new variable
  uint16_t createVariable(uint16_t type, const std::vector<uint8_t>& initialValue = {});
  
  // Get variable by ID
  const Variable* getVariable(uint16_t id) const;
  
  // Check if variable exists
  bool variableExists(uint16_t id) const;
  
  // Enter a new scope
  void enterScope();
  
  // Leave current scope
  void leaveScope();
  
  // Get current scope level
  uint32_t getCurrentScopeLevel() const;
  
  // Clear all variables and scopes
  void clear();
  
  // Get all variables
  std::vector<Variable> getAllVariables() const;
  
private:
  ScopeManager scopeManager_;
  uint16_t nextVariableId_;
};

} // namespace coil

#endif // COIL_VARIABLE_SYSTEM_H