#include "coil/variable_system.h"
#include "coil/instruction_set.h"
#include <stdexcept>
#include <algorithm>

namespace coil {

// Variable implementation
Variable::Variable(uint16_t id, uint16_t type, const std::vector<uint8_t>& initialValue)
  : id_(id), type_(type), initialValue_(initialValue) {
}

std::vector<uint8_t> Variable::createDeclaration() const {
  // Create a VAR instruction
  std::vector<Operand> operands;
  
  // Variable ID
  operands.push_back(Operand::createVariable(id_));
  
  // Type
  operands.push_back(Operand::createImmediate<uint16_t>(type_));
  
  // Initial value (if present)
  if (!initialValue_.empty()) {
      // This is simplified - in a real implementation, we'd need to handle
      // different types of initial values based on the variable type
      Operand initialValueOperand(type_, initialValue_);
      operands.push_back(initialValueOperand);
  }
  
  Instruction varInstruction(Opcode::VAR, operands);
  return varInstruction.encode();
}

std::vector<uint8_t> Variable::encode() const {
  std::vector<uint8_t> result;
  
  // ID (2 bytes)
  result.push_back(id_ & 0xFF);
  result.push_back((id_ >> 8) & 0xFF);
  
  // Type (2 bytes)
  result.push_back(type_ & 0xFF);
  result.push_back((type_ >> 8) & 0xFF);
  
  // Scope level (4 bytes)
  for (int i = 0; i < 4; i++) {
      result.push_back((scopeLevel_ >> (i * 8)) & 0xFF);
  }
  
  // Initial value size (4 bytes)
  uint32_t initialValueSize = static_cast<uint32_t>(initialValue_.size());
  for (int i = 0; i < 4; i++) {
      result.push_back((initialValueSize >> (i * 8)) & 0xFF);
  }
  
  // Initial value (if any)
  result.insert(result.end(), initialValue_.begin(), initialValue_.end());
  
  return result;
}

Variable Variable::decode(const std::vector<uint8_t>& data, size_t& offset) {
  if (data.size() < offset + 2 + 2 + 4 + 4) {
      throw std::runtime_error("Insufficient data for Variable");
  }
  
  Variable var;
  
  // ID (2 bytes)
  var.id_ = data[offset] | (data[offset + 1] << 8);
  offset += 2;
  
  // Type (2 bytes)
  var.type_ = data[offset] | (data[offset + 1] << 8);
  offset += 2;
  
  // Scope level (4 bytes)
  var.scopeLevel_ = data[offset] | (data[offset + 1] << 8) | 
                    (data[offset + 2] << 16) | (data[offset + 3] << 24);
  offset += 4;
  
  // Initial value size (4 bytes)
  uint32_t initialValueSize = data[offset] | (data[offset + 1] << 8) | 
                              (data[offset + 2] << 16) | (data[offset + 3] << 24);
  offset += 4;
  
  // Initial value (if any)
  if (initialValueSize > 0) {
      if (data.size() < offset + initialValueSize) {
          throw std::runtime_error("Insufficient data for Variable initial value");
      }
      
      var.initialValue_.assign(data.begin() + offset, data.begin() + offset + initialValueSize);
      offset += initialValueSize;
  }
  
  return var;
}

// ScopeManager implementation
ScopeManager::ScopeManager() : currentScopeLevel_(0) {
  // Create global scope (level 0)
  scopes_.emplace_back();
}

void ScopeManager::enterScope() {
  currentScopeLevel_++;
  
  // Ensure we have enough scope levels
  if (currentScopeLevel_ >= scopes_.size()) {
      scopes_.emplace_back();
  }
}

void ScopeManager::leaveScope() {
  if (currentScopeLevel_ == 0) {
      throw std::runtime_error("Cannot leave global scope");
  }
  
  // Clear variables in the current scope
  scopes_[currentScopeLevel_].clear();
  
  currentScopeLevel_--;
}

void ScopeManager::addVariable(const Variable& var) {
  // Create a copy of the variable with the current scope level
  Variable scopedVar = var;
  scopedVar.setScopeLevel(currentScopeLevel_);
  
  // Add to current scope
  scopes_[currentScopeLevel_][var.getId()] = scopedVar;
}

const Variable* ScopeManager::findVariable(uint16_t id) const {
  // Search from current scope up to global scope
  for (int level = currentScopeLevel_; level >= 0; level--) {
      auto it = scopes_[level].find(id);
      if (it != scopes_[level].end()) {
          return &(it->second);
      }
  }
  
  return nullptr; // Not found
}

std::vector<Variable> ScopeManager::getCurrentScopeVariables() const {
  std::vector<Variable> result;
  
  const auto& currentScope = scopes_[currentScopeLevel_];
  for (const auto& [id, var] : currentScope) {
      result.push_back(var);
  }
  
  return result;
}

std::vector<Variable> ScopeManager::getAllVariables() const {
  std::vector<Variable> result;
  
  for (const auto& scope : scopes_) {
      for (const auto& [id, var] : scope) {
          result.push_back(var);
      }
  }
  
  return result;
}

void ScopeManager::clear() {
  scopes_.clear();
  scopes_.emplace_back(); // Global scope
  currentScopeLevel_ = 0;
}

// VariableManager implementation
VariableManager::VariableManager() : nextVariableId_(1) {
  // Start with variable ID 1 (0 is reserved)
}

uint16_t VariableManager::createVariable(uint16_t type, const std::vector<uint8_t>& initialValue) {
  uint16_t id = nextVariableId_++;
  Variable var(id, type, initialValue);
  
  scopeManager_.addVariable(var);
  return id;
}

const Variable* VariableManager::getVariable(uint16_t id) const {
  return scopeManager_.findVariable(id);
}

bool VariableManager::variableExists(uint16_t id) const {
  return getVariable(id) != nullptr;
}

void VariableManager::enterScope() {
  scopeManager_.enterScope();
}

void VariableManager::leaveScope() {
  scopeManager_.leaveScope();
}

uint32_t VariableManager::getCurrentScopeLevel() const {
  return scopeManager_.getCurrentScopeLevel();
}

void VariableManager::clear() {
  scopeManager_.clear();
  nextVariableId_ = 1;
}

std::vector<Variable> VariableManager::getAllVariables() const {
  return scopeManager_.getAllVariables();
}

} // namespace coil