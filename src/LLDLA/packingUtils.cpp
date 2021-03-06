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

#include "packingUtils.h"

#if DOLLDLA

#include "horizontalPack.h"
#include "horizontalUnpack.h"
#include "localInput.h"
#include "madd.h"
#include "uniqueNameSource.h"
#include "vadd.h"
#include "verticalPack.h"
#include "vvdot.h"
#include "verticalUnpack.h"

Partition* PartitionIntoMainAndResidual(Layer layer, Node* outNode, ConnNum outNum, Node* inNode, ConnNum inNum, DimName dim, int multiple) {
  DLANode* dlaNode = static_cast<DLANode*>(inNode);
  int totalSize;
  Dir partDir;
  if (dim == DIMM) {
    totalSize = dlaNode->GetInputNumRows(inNum);
    partDir = VERTICAL;
  } else {
    totalSize = dlaNode->GetInputNumCols(inNum);
    partDir = HORIZONTAL;
  }
  int residualSize = totalSize % multiple;
  int mainSize = totalSize - residualSize;
  if (residualSize == 0) {
    cout << "Error: residualSize == 0" << endl;
    LOG_FAIL("replacement for throw call");
  }
  auto part = new Partition(layer, partDir, mainSize);
  part->AddInput(outNode, outNum);
  return part;
}

Node* CopySymmetricBinop(Node* binop) {
  Node* copy;
  if (binop->GetNodeClass() == MAdd::GetClass()) {
    copy = MAdd::BlankInst();
  } else if (binop->GetNodeClass() == VAdd::GetClass()) {
    VAdd* vadd = static_cast<VAdd*>(binop);
    copy = new VAdd(vadd->m_layer, vadd->GetVecType());
  } else {
    LOG_FAIL("Bad operand to CopySymmetricBinop");
    throw;
  }
  //  copy->Duplicate(binop, true, false);
  return copy;
}

int ComputePackedWidth(int length, int multiple) {
  if (length % multiple == 0) {
    cout << "Error: Computing packing width for operand that does not need to be packed" << endl;
    cout << "Length = " << length << endl;
    cout << "Multiple = " << multiple << endl;
    LOG_FAIL("replacement for throw call");
  }
  return length + (multiple - (length % multiple));
}

Pack* PackToMultipleOf(Layer layer, Node* outNode, ConnNum outNum, Node* inNode, ConnNum inNum, DimName dim, int multiple) {
  Pack* pack;
  LocalInput* locIn;
  DLANode* dlaInNode = static_cast<DLANode*>(inNode);
  int packDimLength;
  int packedOperandWidth;
  string locName = localInputNames->Next(dlaInNode->GetInputName(inNum).m_name);

  if (dim == DIMM) {
    packDimLength = dlaInNode->GetInputNumRows(inNum);
    packedOperandWidth = ComputePackedWidth(packDimLength, multiple);
    locIn = new LocalInput(locName,
			   packedOperandWidth, dlaInNode->GetInputNumCols(inNum),
			   1, packedOperandWidth,
			   dlaInNode->GetDataType());
    pack = new VerticalPack(layer);
  } else {
    packDimLength = dlaInNode->GetInputNumCols(inNum);
    packedOperandWidth = ComputePackedWidth(packDimLength, multiple);
    locIn = new LocalInput(locName,
			   dlaInNode->GetInputNumRows(inNum), packedOperandWidth,
			   1, dlaInNode->GetInputNumRows(inNum),
			   dlaInNode->GetDataType());
    pack = new HorizontalPack(layer);
  }

  pack->AddInputs(4,
		  outNode, outNum,
		  locIn, 0);
  return pack;
}

Recombine* SplitBinarySymmetricOperationIntoMainAndResidual(Layer layer, Node* binop, DimName dim, int multiple) {
  auto operand0Part = PartitionIntoMainAndResidual(layer, binop->Input(0), binop->InputConnNum(0), binop, 0, dim, multiple);
  auto operand1Part = PartitionIntoMainAndResidual(layer, binop->Input(1), binop->InputConnNum(1), binop, 1, dim, multiple);

  auto mainBinop = CopySymmetricBinop(binop);
  mainBinop->AddInputs(4,
		       operand0Part, 0,
		       operand1Part, 0);

  auto residualBinop = CopySymmetricBinop(binop);
  residualBinop->AddInputs(4,
			  operand0Part, 1,
			  operand1Part, 1);

  Dir partDir = dim == DIMM ? VERTICAL : HORIZONTAL;
  auto recombine = new Recombine(layer, partDir);
  recombine->AddInputs(6,
		       mainBinop, 0,
		       residualBinop, 0,
		       binop->Input(1), binop->InputConnNum(1));

  return recombine;
}

Unpack* PackBinarySymmetricOperation(Layer layer, Node* binop, DimName dim, int multiple) {
  auto operand0Pack = PackToMultipleOf(layer, binop->Input(0), binop->InputConnNum(0), binop, 0, dim, multiple);
  auto operand1Pack = PackToMultipleOf(layer, binop->Input(1), binop->InputConnNum(1), binop, 1, dim, multiple);

  auto newBinop = CopySymmetricBinop(binop);
  newBinop->AddInputs(4,
		      operand0Pack, 0,
		      operand1Pack, 0);

  Unpack* unpack;
  if (dim == DIMM) {
    unpack = new VerticalUnpack(layer);
  } else {
    unpack = new HorizontalUnpack(layer);
  }
  unpack->AddInputs(4,
		    newBinop, 0,
		    binop->Input(1), binop->InputConnNum(1));

  return unpack;
}

#endif // DOLLDLA
