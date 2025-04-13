

int main() {

  // parse CASM into COIL
  Lexer lex;
  lex.tokens("example.casm", "r");

  // Ensure tokens are correct and ordered
  Parser parser(lex);

  // Generate COIL
  Codegen gen(parser);


  Instr instr(
    
    
  )









  "
  ; CASM Program

  PPSECT data
  SYM hello_world_str
    PPDATA .asciiz \"Hello World!\"
  SYM hello_world_str_end

  PPSECT text

  PPTARG cpu

  ; include platform specific ABIs (i.e. linux-64 bit)
  PPINCL std(\"ABI\")

  SYM _start
    SCOPE

    VAR #1, TOP_I32(10)
    VAR #2, TOP_V128(TOP_I32, [10, 10, 10, 10])

    ; no third operand so #2 is destination and left operand
    ; performs #2 at element i + #1 into element i
    ADD #2, #1

    POP #2
    POP #1

    SCOPE
      PPABI linux-64bit
        PPABIP param0 #1, TOP_I64(1)
        PPABIP param1 #2, TOP_I64(stdout)
        PPABIP param2 #3, TOP_PTR(hello_world_str)
        PPABIP param3 #4, TOP_U64(hello_world_str-hello_world_str_end)
        PPABIR ret0 #5, TOP_U64
      PPABIEND
      SYSC
    SCOPL

    VAR #1, TOP_I64(60)
    VAR #2, TOP_I64(0)

    ; utilizes x86-64 linux syscall structure for x86 architectures
    ; utilizes arm-64 linux syscall structure for arm architectures
    PPABI linux-64bit
      PPABIP param0 #1
      PPABIP param1 #2
    PPABIEND
    
    ; syscall registers are set above in abi statement
    SYSC

    SCOPL

  "


  "
  int main() {
    volatile int x = 10;
    x += 12;
    return 0;  
  }

  PPSECT .text
  SYM main
    SCOPE
      VAR #0, TYPE_I32(10) ; int x
      ADD #0, TYPE_I32(12) ; x += 12
      
      PPABI cabi
        PPABIR ret0 TYPE_I32(0)
      PPABIEND
      
      RET
    SCOPL

  "






}