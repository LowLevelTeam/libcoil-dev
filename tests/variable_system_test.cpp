#include "test_helper.h"

bool testVariableCreation() {
  // Create a variable
  coil::Variable var(1, coil::Type::INT32, {0x01, 0x02, 0x03, 0x04});
  
  // Check properties
  TEST_ASSERT(var.getId() == 1);
  TEST_ASSERT(var.getType() == coil::Type::INT32);
  TEST_ASSERT(var.getScopeLevel() == 0);
  TEST_ASSERT(var.isInitialized());
  TEST_ASSERT(var.getInitialValue().size() == 4);
  TEST_ASSERT(var.getInitialValue()[0] == 0x01);
  TEST_ASSERT(var.getInitialValue()[1] == 0x02);
  TEST_ASSERT(var.getInitialValue()[2] == 0x03);
  TEST_ASSERT(var.getInitialValue()[3] == 0x04);
  
  // Test scope level
  var.setScopeLevel(2);
  TEST_ASSERT(var.getScopeLevel() == 2);
  
  // Create a variable without initial value
  coil::Variable uninitVar(2, coil::Type::FP32);
  TEST_ASSERT(!uninitVar.isInitialized());
  TEST_ASSERT(uninitVar.getInitialValue().empty());
  
  return true;
}

bool testVariableEncoding() {
  // Create a variable
  coil::Variable original(1, coil::Type::INT32, {0x01, 0x02, 0x03, 0x04});
  original.setScopeLevel(3);
  
  // Encode to binary
  std::vector<uint8_t> encoded = original.encode();
  
  // Decode from binary
  size_t offset = 0;
  coil::Variable decoded = coil::Variable::decode(encoded, offset);
  
  // Check properties
  TEST_ASSERT(decoded.getId() == original.getId());
  TEST_ASSERT(decoded.getType() == original.getType());
  TEST_ASSERT(decoded.getScopeLevel() == original.getScopeLevel());
  TEST_ASSERT(decoded.isInitialized() == original.isInitialized());
  TEST_ASSERT(decoded.getInitialValue().size() == original.getInitialValue().size());
  TEST_ASSERT(std::equal(
      decoded.getInitialValue().begin(),
      decoded.getInitialValue().end(),
      original.getInitialValue().begin()
  ));
  
  // Check that offset was advanced correctly
  TEST_ASSERT(offset == encoded.size());
  
  return true;
}

bool testVariableDeclaration() {
  // Create a variable
  coil::Variable var(1, coil::Type::INT32, {0x01, 0x02, 0x03, 0x04});
  
  // Create a variable declaration instruction
  std::vector<uint8_t> declarationInstr = var.createDeclaration();
  
  // Should be a VAR instruction
  TEST_ASSERT(declarationInstr[0] == coil::Opcode::VAR);
  
  // Should have variable ID and type operands, plus initial value
  TEST_ASSERT(declarationInstr[1] == 3);  // 3 operands
  
  return true;
}

bool testScopeManager() {
  coil::ScopeManager scopeManager;
  
  // Initial scope should be global (level 0)
  TEST_ASSERT(scopeManager.getCurrentScopeLevel() == 0);
  
  // Create variables
  coil::Variable globalVar(1, coil::Type::INT32);
  scopeManager.addVariable(globalVar);
  
  // Enter a new scope
  scopeManager.enterScope();
  TEST_ASSERT(scopeManager.getCurrentScopeLevel() == 1);
  
  // Add a variable to the new scope
  coil::Variable localVar(2, coil::Type::FP32);
  scopeManager.addVariable(localVar);
  
  // Both variables should be findable
  TEST_ASSERT(scopeManager.findVariable(1) != nullptr);
  TEST_ASSERT(scopeManager.findVariable(2) != nullptr);
  
  // Scope level should be set correctly
  TEST_ASSERT(scopeManager.findVariable(1)->getScopeLevel() == 0);
  TEST_ASSERT(scopeManager.findVariable(2)->getScopeLevel() == 1);
  
  // Variables in current scope
  auto scopeVars = scopeManager.getCurrentScopeVariables();
  TEST_ASSERT(scopeVars.size() == 1);
  TEST_ASSERT(scopeVars[0].getId() == 2);
  
  // All variables
  auto allVars = scopeManager.getAllVariables();
  TEST_ASSERT(allVars.size() == 2);
  
  // Leave the scope
  scopeManager.leaveScope();
  TEST_ASSERT(scopeManager.getCurrentScopeLevel() == 0);
  
  // Local variable should no longer be findable
  TEST_ASSERT(scopeManager.findVariable(1) != nullptr);
  TEST_ASSERT(scopeManager.findVariable(2) == nullptr);
  
  // Clear all scopes
  scopeManager.clear();
  TEST_ASSERT(scopeManager.getCurrentScopeLevel() == 0);
  TEST_ASSERT(scopeManager.findVariable(1) == nullptr);
  
  return true;
}

bool testVariableManager() {
  coil::VariableManager manager;
  
  // Create variables
  uint16_t var1Id = manager.createVariable(coil::Type::INT32, {0x01, 0x02, 0x03, 0x04});
  uint16_t var2Id = manager.createVariable(coil::Type::FP64);
  
  // Check variables
  TEST_ASSERT(manager.variableExists(var1Id));
  TEST_ASSERT(manager.variableExists(var2Id));
  TEST_ASSERT(!manager.variableExists(100));
  
  const coil::Variable* var1 = manager.getVariable(var1Id);
  TEST_ASSERT(var1 != nullptr);
  TEST_ASSERT(var1->getType() == coil::Type::INT32);
  TEST_ASSERT(var1->isInitialized());
  
  const coil::Variable* var2 = manager.getVariable(var2Id);
  TEST_ASSERT(var2 != nullptr);
  TEST_ASSERT(var2->getType() == coil::Type::FP64);
  TEST_ASSERT(!var2->isInitialized());
  
  // Test scoping
  TEST_ASSERT(manager.getCurrentScopeLevel() == 0);
  
  manager.enterScope();
  TEST_ASSERT(manager.getCurrentScopeLevel() == 1);
  
  uint16_t var3Id = manager.createVariable(coil::Type::INT16);
  TEST_ASSERT(manager.getVariable(var3Id)->getScopeLevel() == 1);
  
  manager.leaveScope();
  TEST_ASSERT(manager.getCurrentScopeLevel() == 0);
  
  // var3 should no longer be accessible
  TEST_ASSERT(!manager.variableExists(var3Id));
  
  // Get all variables
  auto allVars = manager.getAllVariables();
  TEST_ASSERT(allVars.size() == 2);
  
  // Clear
  manager.clear();
  TEST_ASSERT(!manager.variableExists(var1Id));
  TEST_ASSERT(!manager.variableExists(var2Id));
  
  return true;
}

bool testNestedScopes() {
  coil::VariableManager manager;
  
  // Global scope (level 0)
  uint16_t globalVar = manager.createVariable(coil::Type::INT32);
  
  // Enter scope 1
  manager.enterScope();
  uint16_t scope1Var = manager.createVariable(coil::Type::INT32);
  
  // Enter scope 2
  manager.enterScope();
  uint16_t scope2Var = manager.createVariable(coil::Type::INT32);
  
  // All variables should be accessible
  TEST_ASSERT(manager.variableExists(globalVar));
  TEST_ASSERT(manager.variableExists(scope1Var));
  TEST_ASSERT(manager.variableExists(scope2Var));
  
  // Leave scope 2
  manager.leaveScope();
  
  // scope2Var should not be accessible, others should be
  TEST_ASSERT(manager.variableExists(globalVar));
  TEST_ASSERT(manager.variableExists(scope1Var));
  TEST_ASSERT(!manager.variableExists(scope2Var));
  
  // Leave scope 1
  manager.leaveScope();
  
  // Only globalVar should be accessible
  TEST_ASSERT(manager.variableExists(globalVar));
  TEST_ASSERT(!manager.variableExists(scope1Var));
  TEST_ASSERT(!manager.variableExists(scope2Var));
  
  return true;
}

int main() {
  std::vector<std::pair<std::string, std::function<bool()>>> tests = {
      {"Variable Creation", testVariableCreation},
      {"Variable Encoding", testVariableEncoding},
      {"Variable Declaration", testVariableDeclaration},
      {"Scope Manager", testScopeManager},
      {"Variable Manager", testVariableManager},
      {"Nested Scopes", testNestedScopes}
  };
  
  return runTests(tests);
}