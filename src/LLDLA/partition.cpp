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

#include <assert.h>

#include "LLDLA.h"
#include "partition.h"

#if DOLLDLA

Partition::Partition(Layer layer, Dir partType, Size splitSize) {
  m_layer = layer;
  m_partType = partType;
  m_splitSize = splitSize;
  return;
}

void Partition::PrintCode(IndStream &out) {
  out.Indent();
  *out << m_startName.m_name << " = " << GetInputName(0).m_name << ";\n";
  string splitPoint;
  const SizeList *sizes = NULL;
  if (m_partType == HORIZONTAL) {
    sizes = GetInputN(0);
  }
  else {
    sizes = GetInputM(0);
  }
  if (sizes->IsConstant()) {
    Size size = (*sizes)[0];
    splitPoint = std::to_string((long long int)(floor(size / (double)m_splitSize) * m_splitSize));
  }
  else
    LOG_FAIL("not handling non-constant partition print code yet");
  out.Indent();

  if (m_partType == HORIZONTAL) {
    *out << m_endName.m_name << " = " << GetInputName(0).m_name << " + " << InputDataType(0).m_colStrideVar << " * " << splitPoint + ";\n";

    out.Indent();
    *out << m_startDim.m_name << " = " << splitPoint + ";\n";
    out.Indent();
    *out << m_endDim.m_name << " = " << InputDataType(0).m_numColsVar << " - " << splitPoint+ ";\n";
  } else {
    *out << m_endName.m_name << " = " << GetInputName(0).m_name << " + " << InputDataType(0).m_rowStrideVar << " * " << splitPoint + ";\n";

    out.Indent();
    *out << m_startDim.m_name << " = " << splitPoint + ";\n";
    out.Indent();
    *out << m_endDim.m_name << " = " << InputDataType(0).m_numRowsVar << " - " << splitPoint + ";\n";
  }
  return;
}

void Partition::Prop() {
  if (!IsValidCost(m_cost)) {
    DLANode::Prop();
    m_cost = 0;
  }
  return;
}

Name Partition::GetName(ConnNum num) const {
  if (num > 1) {
    cout << "num > 1 in Partition::GetName" << endl;
    LOG_FAIL("replacement for throw call");
  }

  if (num == 0) {
    return m_startName;
  } else {
    return m_endName;
  }
}

void Partition::AddVariables(VarSet &set) const {
  //  cout << "ADDING PARTITION VARIABLES" << endl;
  string startVarDecl;
  string endVarDecl;
  string typeName;
  if (GetDataType() == REAL_SINGLE) {
    typeName = "float* ";
  } else if (GetDataType() == REAL_DOUBLE) {
    typeName = "double* ";
  } else {
    cout << "Unsupported datatype: " << GetDataType() << endl;
  }
  if (m_partType == HORIZONTAL) {
    startVarDecl = typeName + " " +  m_startName.m_name + ";";
    endVarDecl = typeName + " " +  m_endName.m_name + ";";
  } else {
    startVarDecl = typeName + " " +  m_startName.m_name + ";";
    endVarDecl = typeName + " " +  m_endName.m_name + ";";
  }

  Var startVar(DirectVarDeclType, startVarDecl, GetDataType());
  Var endVar(DirectVarDeclType, endVarDecl, GetDataType());

  set.insert(startVar);
  set.insert(endVar);

  DataTypeInfo inData = InputDataType(0);

  string startDimDecl = "unsigned int " + m_startDim.m_name + ";";
  string endDimDecl = "unsigned int " + m_endDim.m_name + ";";
  Var startDimVar(DirectVarDeclType, startDimDecl, GetDataType());
  Var endDimVar(DirectVarDeclType, endDimDecl, GetDataType());
  set.insert(startDimVar);
  set.insert(endDimVar);
}

void Partition::BuildDataTypeCache() {
  if (m_partType == HORIZONTAL) {
    SetHorizontalNames();
    BuildHorizontalDataTypeCache();
  } else {
    SetVerticalNames();
    BuildVerticalDataTypeCache();
  }
}

void Partition::SetHorizontalNames() {
  m_startName = GetInputName(0);
  m_startName.m_name += "_LEFT";

  m_endName = GetInputName(0);
  m_endName.m_name += "_RIGHT";

  DataTypeInfo inData = InputDataType(0);
  m_startDim.m_name = inData.m_numColsVar + "_LEFT";
  m_endDim.m_name = inData.m_numColsVar + "_RIGHT";
}

void Partition::SetVerticalNames() {
  m_startName = GetInputName(0);
  m_startName.m_name += "_TOP";

  m_endName = GetInputName(0);
  m_endName.m_name += "_BOTTOM";

  DataTypeInfo inData = InputDataType(0);
  m_startDim.m_name = inData.m_numRowsVar + "_TOP";
  m_endDim.m_name = inData.m_numRowsVar + "_BOTTOM";
}

void Partition::BuildHorizontalDataTypeCache() {
  BuildHorizontalDataTypeInfo();
  BuildHorizontalSizes();
}

void Partition::BuildVerticalDataTypeCache() {
  BuildVerticalDataTypeInfo();
  BuildVerticalSizes();
}

void Partition::BuildHorizontalSizes() {
  const SizeList* colSizes = GetInputN(0);
  BuildStartAndEndSizes(colSizes);
}

void Partition::BuildVerticalSizes() {
  const SizeList* rowSizes = GetInputM(0);
  BuildStartAndEndSizes(rowSizes);
}

void Partition::BuildStartAndEndSizes(const SizeList* toSplit) {
  m_startSizes = SizeList::M_cache.GetCachedSplitSize(true,
						      toSplit,
						      m_splitSize);
  m_endSizes = SizeList::M_cache.GetCachedSplitSize(false,
						    toSplit,
						    m_splitSize);
}

void Partition::BuildHorizontalDataTypeInfo() {
  DataTypeInfo inData = InputDataType(0);

  string startNumColsVar = inData.m_numColsVar;
  startNumColsVar = startNumColsVar + "_LEFT";
  m_startInfo = new DataTypeInfo(GetInputRowStride(0), GetInputColStride(0),
				 inData.m_numRowsVar, startNumColsVar,
				 inData.m_rowStrideVar, inData.m_colStrideVar,
				 inData.m_type);

  string endNumColsVar = inData.m_numColsVar;
  endNumColsVar = endNumColsVar + "_RIGHT";
  m_endInfo = new DataTypeInfo(GetInputRowStride(0), GetInputColStride(0),
			       inData.m_numRowsVar, endNumColsVar,
			       inData.m_rowStrideVar, inData.m_colStrideVar,
			       inData.m_type);
}

void Partition::BuildVerticalDataTypeInfo() {
  DataTypeInfo inData = InputDataType(0);

  string startNumRowsVar = inData.m_numRowsVar;
  startNumRowsVar = startNumRowsVar + "_TOP";
  m_startInfo = new DataTypeInfo(GetInputRowStride(0), GetInputColStride(0),
				 startNumRowsVar, inData.m_numColsVar,
				 inData.m_rowStrideVar, inData.m_colStrideVar,
				 inData.m_type);

  string endNumRowsVar = inData.m_numRowsVar;
  endNumRowsVar = endNumRowsVar + "_BOTTOM";
  m_endInfo = new DataTypeInfo(GetInputRowStride(0), GetInputColStride(0),
			       endNumRowsVar, inData.m_numColsVar,
			       inData.m_rowStrideVar, inData.m_colStrideVar,
			       inData.m_type);
}

void Partition::ClearDataTypeCache() {
  m_startInfo = NULL;
  m_endInfo = NULL;

  m_startSizes = NULL;
  m_endSizes = NULL;
}

ConnNum Partition::NumOutputs() const {
  return 2;
}

const DataTypeInfo& Partition::DataType(ConnNum num) const {
  if (num > 1) {
    cout << "Error: argument to DataType is too large\n";
    LOG_FAIL("replacement for throw call");
  }
  if (num == 0) {
    return *m_startInfo;
  } else {
    return *m_endInfo;
  }
}

void Partition::Duplicate(const Node* orig, bool shallow, bool possMerging) {
  DLANode::Duplicate(orig, shallow, possMerging);
  const Partition* part = static_cast<const Partition*>(orig);

  m_layer = part->m_layer;

  m_partType = part->m_partType;
  m_splitSize = part->m_splitSize;

  m_startSizes = part->m_startSizes;
  m_endSizes = part->m_endSizes;

  m_startInfo = part->m_startInfo;
  m_endInfo = part->m_endInfo;

  m_startName = m_startName;
  m_endName = m_endName;
}

const SizeList* Partition::GetM(ConnNum num) const {
  if (num > 1) {
    cout << "Bad connection number in Partition::GetM" << endl;
    LOG_FAIL("replacement for throw call");
  }

  if (m_partType == HORIZONTAL) {
    return GetInputM(0);
  } else {
    if (num == 0) {
      return m_startSizes;
    } else {
      return m_endSizes;
    }
  }
  LOG_FAIL("replacement for throw call");
}

const SizeList* Partition::GetN(ConnNum num) const {
  if (num > 1) {
    cout << "Bad connection number in partition::GetN" << endl;
    LOG_FAIL("replacement for throw call");
  }

  if (m_partType == VERTICAL) {
    return GetInputN(0);
  } else {
    if (num == 0) {
      return m_startSizes;
    } else {
      return m_endSizes;
    }
  }
}

PartitionLowerLayer::PartitionLowerLayer(Layer fromLayer, Layer toLayer) {
  m_fromLayer = fromLayer;
  m_toLayer = toLayer;
}

bool PartitionLowerLayer::CanApply(const Node *node) const {
  if (node->GetNodeClass() == Partition::GetClass()) {
    const Partition *part = static_cast<const Partition*>(node);
    if (part->GetLayer() != m_fromLayer) {
      return false;
    }
    return true;
  }
  cout << "Error: Applying PartitionLowerLayer to non-Partition node\n";
  LOG_FAIL("replacement for throw call");
  throw;
}

void PartitionLowerLayer::Apply(Node *node) const {
  Partition *part = static_cast<Partition*>(node);
  part->SetLayer(m_toLayer);
}

string PartitionLowerLayer::GetType() const {
  return "Partition lower layer " + LayerNumToStr(m_fromLayer)
    + " to " + LayerNumToStr(m_toLayer);
}

#endif // DOLLDLA
