/*
    This file is part of DxTer.
    DxTer is a prototype using the Design by Transformation (DxT)
    approach to program generation.

    Copyright (C) 2014, The University of Texas and Bryan Marker

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

Partition::Partition(Layer layer, Dir partType, Size partSplitPoint)
{
  m_layer = layer;
  m_partType = partType;
  m_partSplitPoint = partSplitPoint;
  return;
}

void Partition::PrintCode(IndStream &out)
{
  out.Indent();
  *out << m_startName.m_name << " = " << GetInputName(0).m_name << ";\n";
  out.Indent();
  if (m_partType == HORIZONTAL) {
    *out << m_endName.m_name << " = " << GetInputName(0).m_name << " + " << InputDataType(0).m_colStrideVar << " * " << std::to_string((int) m_partSplitPoint) + ";\n"; 
  } else {
    *out << m_endName.m_name << " = " << GetInputName(0).m_name << " + " << InputDataType(0).m_rowStrideVar << " * " << std::to_string((int) m_partSplitPoint) + ";\n";
  }
  return;
}

void Partition::Prop()
{
  if (!IsValidCost(m_cost)) {
    DLANode::Prop();
    m_cost = 0;
  }
  return;
}

Name Partition::GetName(ConnNum num) const
{
  if (num > 1) {
    cout << "num > 1 in Partition::GetName" << endl;
    throw;
  }

  if (num == 0) {
    return m_startName;
  } else {
    return m_endName;
  }
}

void Partition::AddVariables(VarSet &set) const
{
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
}

void Partition::BuildDataTypeCache()
{
  if (m_partType == HORIZONTAL) {
    SetHorizontalNames();
    BuildHorizontalDataTypeCache();
  } else {
    SetVerticalNames();
    BuildVerticalDataTypeCache();
  }
}

void Partition::SetHorizontalNames()
{
  m_startName = GetInputName(0);
  m_startName.m_name += "_LEFT";

  m_endName = GetInputName(0);
  m_endName.m_name += "_RIGHT";
}

void Partition::SetVerticalNames()
{
  m_startName = GetInputName(0);
  m_startName.m_name += "_TOP";

  m_endName = GetInputName(0);
  m_endName.m_name += "_BOTTOM";
}

void Partition::BuildHorizontalDataTypeCache()
{
  BuildHorizontalDataTypeInfo();
  BuildHorizontalSizes();
}

void Partition::BuildVerticalDataTypeCache()
{
  BuildVerticalDataTypeInfo();
  BuildVerticalSizes();
}

void Partition::BuildHorizontalSizes()
{
  const Sizes* colSizes = GetInputN(0);
  assert(colSizes->IsPartitionable(m_partSplitPoint));
  BuildStartAndEndSizes(colSizes);
}

void Partition::BuildVerticalSizes()
{
  const Sizes* rowSizes = GetInputM(0);
  assert(rowSizes->IsPartitionable(m_partSplitPoint));
  BuildStartAndEndSizes(rowSizes);
}

void Partition::BuildStartAndEndSizes(const Sizes* toSplit)
{
  SizeEntry* sizeEnt = toSplit->m_entries[0];
  int numIterations = sizeEnt->m_repeats;
  int parFactor = sizeEnt->m_parFactor;
  Size sizeOfEachIteration = sizeEnt->m_valA;
  Size sizeOfEndIterations = sizeOfEachIteration - m_partSplitPoint;
  
  m_startSizes = new Sizes();
  m_startSizes->AddRepeatedSizes(m_partSplitPoint, numIterations, parFactor);

  m_endSizes = new Sizes();
  m_endSizes->AddRepeatedSizes(sizeOfEndIterations, numIterations, parFactor);
  return;
}

void Partition::BuildHorizontalDataTypeInfo()
{
  DataTypeInfo inData = InputDataType(0);

  string startNumColsVar = inData.m_numColsVar;
  startNumColsVar = startNumColsVar + "_LEFT";
  m_startInfo = new DataTypeInfo(inData.m_rowStride, inData.m_colStride,
				 inData.m_numRowsVar, startNumColsVar,
				 inData.m_rowStrideVar, inData.m_colStrideVar,
				 inData.m_type);

  string endNumColsVar = inData.m_numColsVar;
  endNumColsVar = endNumColsVar + "_RIGHT";
  m_endInfo = new DataTypeInfo(inData.m_rowStride, inData.m_colStride,
				 inData.m_numRowsVar, endNumColsVar,
				 inData.m_rowStrideVar, inData.m_colStrideVar,
				 inData.m_type);
}

void Partition::BuildVerticalDataTypeInfo()
{
  DataTypeInfo inData = InputDataType(0);

  string startNumRowsVar = inData.m_numColsVar;
  startNumRowsVar = startNumRowsVar + "_TOP";
  m_startInfo = new DataTypeInfo(inData.m_rowStride, inData.m_colStride,
				 startNumRowsVar, inData.m_numColsVar,
				 inData.m_rowStrideVar, inData.m_colStrideVar,
				 inData.m_type);

  string endNumRowsVar = inData.m_numColsVar;
  endNumRowsVar = endNumRowsVar + "_BOTTOM";
  m_endInfo = new DataTypeInfo(inData.m_rowStride, inData.m_colStride,
				 endNumRowsVar, inData.m_numColsVar,
				 inData.m_rowStrideVar, inData.m_colStrideVar,
				 inData.m_type);
}

void Partition::ClearDataTypeCache()
{
  m_startInfo = NULL;
  m_endInfo = NULL;

  m_startSizes = NULL;
  m_endSizes = NULL;
}

ConnNum Partition::NumOutputs() const
{
  return 2;
}

const DataTypeInfo& Partition::DataType(ConnNum num) const
{
  cout << "GETTING DATATYPE FROM PARTITION\n";
  if (num > 1) {
    cout << "Error: argument to DataType is too large\n";
    throw;
  }
  if (num == 0) {
    return *m_startInfo;
  } else {
    return *m_endInfo;
  }
}

void Partition::Duplicate(const Node* orig, bool shallow, bool possMerging)
{
  DLANode::Duplicate(orig, shallow, possMerging);
  const Partition* part = (Partition*) orig;
  m_layer = part->m_layer;
  m_partType = part->m_partType;
  m_startSizes = part->m_startSizes;
  m_endSizes = part->m_endSizes;
  m_startInfo = part->m_startInfo;
  m_endInfo = part->m_endInfo;
  m_startName = m_startName;
  m_endName = m_endName;
}

const Sizes* Partition::GetM(ConnNum num) const {
  if (num > 1) {
    cout << "Bad connection number in Partition::GetM" << endl;
    throw;
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
  throw;
}

const Sizes* Partition::GetN(ConnNum num) const {
  if (num > 1) {
    cout << "Bad connection number in partition::GetN" << endl;
    throw;
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

#endif // DOLLDLA
