#include "coil/obj.hpp"

namespace coil {

  CoilObject::CoilObject(StreamReader &reader) 
    : header(reader)
  {
    this->symbolTable.
    this->symbolTable.decode(reader);
  
  }

  void CoilObject::encode(StreamWriter& writer) {

  }

  void CoilObject::decode(StreamReader &reader) {

  }
};
