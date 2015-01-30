/*
    This file is part of DxTer.
    DxTer is a prototype using the Design by Transformation (DxT)
    approach to program generation.

    Copyright (C) 2015, The University of Texas and Bryan Marker

    DxTer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DxTer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DxTer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "localInput.h"

#if DOLLDLA

void LocalInput::Prop() {
  if (!IsValidCost(m_cost)) {
    InputNode::Prop();

    if (!m_dataTypeInfo.IsContiguous()) {
      cout << "ERROR: Local inputs must be contiguous in memory" << endl;
      throw;
    }
    m_cost = 0;
  }
}

void LocalInput::PrintCode(IndStream& out) {
  Type dataType = m_dataTypeInfo.m_type;
  out.Indent();
  if (dataType == REAL_SINGLE) {
    *out << "float ";
  } else {
    *out << "double ";
  }
  string size = m_dataTypeInfo.m_numRowsVar + " * " + m_dataTypeInfo.m_numColsVar;
  *out << m_varName.m_name << "[" << size << "] " << " = {0};" << endl;
}

NodeType LocalInput::GetType() const {
  return "LocalInput " + LayerNumToStr(GetLayer());
}

ClassType LocalInput::GetClass() {
  return "LocalInput";
}

ClassType LocalInput::GetNodeClass() const {
  return GetClass();
}

#endif // DOLLDLA