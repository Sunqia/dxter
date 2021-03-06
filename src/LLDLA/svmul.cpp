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
#include "regArith.h"
#include "recombine.h"
#include "regLoadStore.h"
#include "scalarArith.h"
#include "svmul.h"

#if DOLLDLA

SVMul::SVMul(Layer layer)
{
  m_layer = layer;
}

VecType SVMul::GetVecType() const {
  if (IsInputRowVector(0) && IsInputRowVector(1)) {
    return ROWVECTOR;
  }
  if (IsInputColVector(0) && IsInputColVector(1)) {
    return COLVECTOR;
  }
  LOG_FAIL("replacement for throw call");
  throw;
}

void SVMul::PrintCode(IndStream &out)
{
  if (m_layer == ABSLAYER) {
    if (GetDataType() == REAL_DOUBLE) {
      *out << "simple_smul( ";
    } else if (GetDataType() == REAL_SINGLE) {
      *out << "simple_smul_float( ";
    }
    if (GetVecType() == COLVECTOR) {
      *out << InputDataType(1).m_numRowsVar << ", " <<
	" 1, " <<
	GetInputName(0).str() << ", " <<
	GetInputName(1).str() << ", " <<
	InputDataType(1).m_rowStrideVar << ", " <<
	InputDataType(1).m_colStrideVar << ");\n";
    } else {
      *out <<" 1, " <<
	InputDataType(1).m_numColsVar << ", " <<
	GetInputName(0).str() << ", " <<
	GetInputName(1).str() << ", " <<
	InputDataType(1).m_rowStrideVar << ", " <<
	InputDataType(1).m_colStrideVar << ");\n";
    }

    return;
  }
  if (m_layer != LLDLAPRIMITIVELAYER) {
    cout << "ERROR: Attempt to generate code from non-primitive scalar vector multiply\n";
    LOG_FAIL("replacement for throw call");
  }
  const DataTypeInfo &inInfo = InputDataType(1);
  const Stride rowStride = inInfo.m_rowStride;
  const Stride colStride = inInfo.m_colStride;
  
  out.Indent();
  if (rowStride == NONUNITSTRIDE && colStride == NONUNITSTRIDE) {
    PrintGeneralStride(out);
  } else if (rowStride == UNITSTRIDE && colStride == NONUNITSTRIDE) {
    PrintColStride(out);
  } else if (rowStride == NONUNITSTRIDE && colStride == UNITSTRIDE) {
    PrintRowStride(out);
  } else {
    *out << "ERROR: BAD STRIDE\n";
  }

}


void SVMul::PrintRowStride(IndStream &out)
{
  if (GetVecType() == COLVECTOR) {
    *out << "row_stride_smul_2x1( " <<
      GetInputName(0).str() << ", " <<
      GetInputName(1).str() << ", " <<
      InputDataType(1).m_rowStrideVar << " );\n";
  } else {
    *out << "row_stride_smul_1x2( " <<
      GetInputName(0).str() << ", " <<
      GetInputName(1).str() << ", " <<
      InputDataType(1).m_rowStrideVar << " );\n";
  }
  return;
}

void SVMul::PrintColStride(IndStream &out)
{
  if (GetVecType() == COLVECTOR) {
    *out << "col_stride_smul_2x1( " <<
      GetInputName(0).str() << ", " <<
      GetInputName(1).str() << ", " <<
      InputDataType(1).m_colStrideVar << " );\n";
  } else {
    *out << "col_stride_smul_1x2( " <<
      GetInputName(0).str() << ", " <<
      GetInputName(1).str() << ", " <<
      InputDataType(1).m_colStrideVar << " );\n";
  }
}

void SVMul::PrintGeneralStride(IndStream &out)
{
  if (GetVecType() == COLVECTOR) {
    *out << "gen_stride_smul_2x1( " <<
      GetInputName(0).str() << ", " <<
      GetInputName(1).str() << ", " <<
      InputDataType(1).m_rowStrideVar << ", " <<
      InputDataType(1).m_colStrideVar << " );\n";
  } else {
    *out << "gen_stride_smul_1x2( " <<
      GetInputName(0).str() << ", " <<
      GetInputName(1).str() << ", " <<
      InputDataType(1).m_rowStrideVar << ", " <<
      InputDataType(1).m_colStrideVar << " );\n";
  }
}

void SVMul::Prop()
{
  if (!IsValidCost(m_cost)) {
    DLAOp<2,1>::Prop();

    if (!((DLANode*) Input(0))->IsScalar(InputConnNum(0))) {
      cout << "ERROR: SVMul input 0 is not a scalar\n";
      LOG_FAIL("replacement for throw call");
    }

    VectorOpInputDimensionCheck(1);

    if (m_layer == ABSLAYER) {
      if (GetVecType() == ROWVECTOR) {
	m_cost = GetInputN(1)->Sum();
      } else {
	m_cost = GetInputM(1)->Sum();
      }
    } else {
      m_cost = ZERO;
    }
  }
}

void SVMul::VectorOpInputDimensionCheck(ConnNum inputNum)
{
  if (GetVecType() == ROWVECTOR && *GetInputM(inputNum) != 1) {
    cout << "ERROR: " << GetType() << " input # " << inputNum << " has more than 1 row\n";
    LOG_FAIL("replacement for throw call");
  } else if (GetVecType() == COLVECTOR && *GetInputN(inputNum) != 1) {
    cout << "ERROR: " << GetType() << " input # " << inputNum  << " has more than 1 column\n";
  }
  int regWidth = GetVecRegWidth();
  if (m_layer == LLDLAPRIMITIVELAYER) {
    if (GetVecType() == ROWVECTOR && *GetInputN(inputNum) != regWidth) {
      cout << "ERROR: " << GetType() << " input # " << inputNum << " does not have regWidth columns\n";
      LOG_FAIL("replacement for throw call");
    } else if(GetVecType() == COLVECTOR && *GetInputM(inputNum) != regWidth) {
      cout << "ERROR: " << GetType() << " input # " << inputNum << " does not have regWidth rows\n";
      LOG_FAIL("replacement for throw call");
    }
  }
}

Node* SVMul::BlankInst()
{
  return new SVMul(LLDLAPRIMITIVELAYER);
}

NodeType SVMul::GetType() const
{
  return "SVMul" +  LayerNumToStr(GetLayer());
}

void SVMul::Duplicate(const Node *orig, bool shallow, bool possMerging)
{
  DLAOp<2,1>::Duplicate(orig, shallow, possMerging);
}

Phase SVMul::MaxPhase() const 
{ 
  switch (m_layer)
    { 
    case(ABSLAYER):
      return LLDLALOOPPHASE;
    case(LLDLAMIDLAYER):
      return LLDLAPRIMPHASE;
    case (LLDLAPRIMITIVELAYER):
      return NUMPHASES; 
    default:
      LOG_FAIL("replacement for throw call");
      throw;
    }
}

SVMulLoopRef::SVMulLoopRef(Layer fromLayer, Layer toLayer, VecType vtype)
{
  m_fromLayer = fromLayer;
  m_toLayer = toLayer;
  m_vtype = vtype;
}

string SVMulLoopRef::GetType() const
{
  switch(m_vtype)
    {
    case(ROWVECTOR):
      return "SVMulLoopRef - row vector";
    case(COLVECTOR):
      return "SVMulLoopRef - column vector";
    default:
      LOG_FAIL("replacement for throw call");
    }
}

bool SVMulLoopRef::CanApply(const Node *node) const
{
  if (node->GetNodeClass() == SVMul::GetClass()) {
    const SVMul *svmul = static_cast<const SVMul*>(node);
    if (svmul->GetLayer() != m_fromLayer) {
      return false;
    }
    if (m_vtype == ROWVECTOR) {
      return *(svmul->GetInputN(1)) > svmul->GetVecRegWidth() &&
	svmul->GetInputN(1)->EvenlyDivisibleBy(svmul->GetVecRegWidth());
    } 
    else if (m_vtype == COLVECTOR) {
      return *(svmul->GetInputM(1)) > svmul->GetVecRegWidth() &&
	svmul->GetInputM(1)->EvenlyDivisibleBy(svmul->GetVecRegWidth());
    } 
    else {
      LOG_FAIL("replacement for throw call");
      throw;
    }
  }

  cout << "ERROR: Cannot apply SVMulLoopRef to a non SVMul node" << endl;
  LOG_FAIL("replacement for throw call");
  throw;
}

void SVMulLoopRef::Apply(Node *node) const
{
  SVMul *svmul = static_cast<SVMul*>(node);

  SplitSingleIter *split = new SplitSingleIter(m_vtype == COLVECTOR ? PARTDOWN : PARTRIGHT, POSSTUNIN, true);
  split->AddInput(svmul->Input(1), svmul->InputConnNum(1));

  if (m_vtype == COLVECTOR) {
    split->SetUpStats(FULLUP, FULLUP,
		      NOTUP, NOTUP);
  } else {
    split->SetUpStats(FULLUP, NOTUP,
		      FULLUP, NOTUP);
  }

  split->SetIndepIters();

  LoopTunnel *scalarTun = new LoopTunnel(POSSTUNIN);
  scalarTun->AddInput(svmul->Input(0),svmul->InputConnNum(0));
  scalarTun->SetAllStats(FULLUP);
  scalarTun->SetIndepIters();

  SVMul *newMul = new SVMul(svmul->m_layer);
  newMul->SetLayer(m_toLayer);

  newMul->AddInput(scalarTun, 0);
  newMul->AddInput(split, 1);

  LoopTunnel *scalarTunOut = new LoopTunnel(POSSTUNOUT);
  scalarTunOut->AddInput(scalarTun, 0);
  scalarTunOut->AddInput(scalarTun, 0);
  scalarTunOut->CopyTunnelInfo(scalarTun);

  CombineSingleIter *com = split->CreateMatchingCombine(1, 
					      1, newMul, 0);
  
  Poss *loopPoss = new Poss(2, scalarTunOut, com);

  RealLoop* loop;
  if (svmul->GetDataType() == REAL_SINGLE) {
    loop = new RealLoop(LLDLALOOP, loopPoss, LLDLAMuSingle);
  } else if (svmul->GetDataType() == REAL_DOUBLE) {
    loop = new RealLoop(LLDLALOOP, loopPoss, LLDLAMuDouble);
  } else {
    cout << "Error: Bad data type in vadd apply\n";
    LOG_FAIL("replacement for throw call");
    throw;
  }

  // Row vectors are partitioned in the N dimension, column vectors in the M dimension
  loop->SetDimName(m_vtype == COLVECTOR ? DIMM : DIMN);

  node->m_poss->AddPSet(loop);
  node->RedirectChildren(loop->OutTun(1), 0);
  node->m_poss->DeleteChildAndCleanUp(node);

}

SVMulLowerLayer::SVMulLowerLayer(Layer fromLayer, Layer toLayer, Size bs)
{
  m_fromLayer = fromLayer;
  m_toLayer = toLayer;
  m_bs = bs;
}

bool SVMulLowerLayer::CanApply(const Node *node) const
{
  if (node->GetNodeClass() == SVMul::GetClass()) {
    const SVMul *svmul = static_cast<const SVMul*>(node);
    if (svmul->GetLayer() != m_fromLayer) {
      return false;
    }
    if (*(svmul->GetInputM(1)) <= m_bs &&
	*(svmul->GetInputN(1)) <= m_bs) {
      return true;
    } else {
      return false;
    }
    return true;
  }
  else {
    LOG_FAIL("replacement for throw call");
    throw;
  }
}

void SVMulLowerLayer::Apply(Node *node) const
{
  SVMul *svmul = static_cast<SVMul*>(node);
  svmul->SetLayer(m_toLayer);
}

string SVMulLowerLayer::GetType() const
{
  return "SVMul lower layer " + LayerNumToStr(m_fromLayer)
    + " to " + LayerNumToStr(m_toLayer);
}

SVMulToRegArith::SVMulToRegArith(Layer fromLayer, Layer toLayer, VecType vecType)
{
  m_fromLayer = fromLayer;
  m_toLayer = toLayer;
  m_vecType = vecType;
}

bool SVMulToRegArith::CanApply(const Node* node) const
{
  if (node->GetNodeClass() == SVMul::GetClass()) {
    const SVMul* svmul = static_cast<const SVMul*>(node);
    if (svmul->GetLayer() != m_fromLayer) {
      return false;
    }
    if (svmul->GetVecType() != m_vecType) {
      return false;
    }
    //    if ((!(*(svmul->GetInputM(1)) <= svmul->GetVecRegWidth()) &&
    if ((m_vecType == COLVECTOR) &&
	svmul->GetInputM(1)->EvenlyDivisibleBy(svmul->GetVecRegWidth())) {
      return true;
    }
    //    if (!(*(svmul->GetInputN(1)) <= svmul->GetVecRegWidth()) &&
    if ((m_vecType == ROWVECTOR) &&
	svmul->GetInputN(1)->EvenlyDivisibleBy(svmul->GetVecRegWidth())) {
      return true;
    }
    return false;
  }
  cout << "ERROR: Trying to apply SVMulToRegArith to non SVMul node" << endl;
  LOG_FAIL("replacement for throw call");
  throw;
}

void SVMulToRegArith::Apply(Node* node) const
{
  SVMul* svmul = static_cast<SVMul*>(node);

  // Split up the input vector
  SplitSingleIter* splitVec;
  if (m_vecType == ROWVECTOR) {
    splitVec = new SplitSingleIter(PARTRIGHT, POSSTUNIN, true);
  } else {
    splitVec = new SplitSingleIter(PARTDOWN, POSSTUNIN, true);
  }

  splitVec->AddInput(svmul->Input(1), svmul->InputConnNum(1));

  if (m_vecType == ROWVECTOR) {
    splitVec->SetUpStats(FULLUP, NOTUP,
			  FULLUP, NOTUP);
  } else {
    splitVec->SetUpStats(FULLUP, FULLUP,
			  NOTUP, NOTUP);
  }

  // Duplicate the value of C into the temp register
  DuplicateRegLoad* dup = new DuplicateRegLoad();
  dup->AddInput(svmul->Input(0), svmul->InputConnNum(0));
  
  node->m_poss->AddNode(dup);

  // Create tunnel for duplicated scalar
  LoopTunnel* scalarTun = new LoopTunnel(POSSTUNIN);
  scalarTun->AddInput(dup, 0);
  scalarTun->SetAllStats(FULLUP);

  // Create register for vector elements
  LoadToRegs* loadA = new LoadToRegs();
  loadA->AddInput(splitVec, 1);

  // Create inner multiply operation
  Mul* mul = new Mul();
  mul->AddInput(scalarTun, 0);
  mul->AddInput(loadA, 0);

  // Create store node to save newly computed elements of x * A
  StoreFromRegs* storeVec = new StoreFromRegs();
  storeVec->AddInput(mul, 0);
  storeVec->AddInput(splitVec, 1);

  // Create output tunnel for scalar
  LoopTunnel* scalarOut = new LoopTunnel(POSSTUNOUT);
  scalarOut->AddInput(scalarTun, 0);
  scalarOut->AddInput(scalarTun, 1);
  scalarOut->CopyTunnelInfo(scalarTun);

  // Combine resulting vector
  CombineSingleIter* combineVec = splitVec->CreateMatchingCombine(1, 1, storeVec, 0);

  // Create poss
  Poss* loopPoss = new Poss(2, combineVec, scalarOut);
  RealLoop* loop;
  if (svmul->GetDataType() == REAL_SINGLE) {
    loop = new RealLoop(LLDLALOOP, loopPoss, LLDLAMuSingle);
  } else if (svmul->GetDataType() == REAL_DOUBLE) {
    loop = new RealLoop(LLDLALOOP, loopPoss, LLDLAMuDouble);
  } else {
    cout << "Error: Bad data type in vadd apply\n";
    LOG_FAIL("replacement for throw call");
    throw;
  }
  
  // Adding loop to poss and cleanup
  node->m_poss->AddPSet(loop);
  node->RedirectChildren(loop->OutTun(0), 0);
  node->m_poss->DeleteChildAndCleanUp(node);
  return;
}

string SVMulToRegArith::GetType() const
{
  if (m_vecType == ROWVECTOR) {
    return "SVMul register arith - Row vector " + LayerNumToStr(m_fromLayer)
      + " to " + LayerNumToStr(m_fromLayer);
  } else {
    return "SVMul register arith - Col vector " + LayerNumToStr(m_fromLayer)
      + " to " + LayerNumToStr(m_fromLayer);
  }
}

// Scalar arithmetic
SVMulToScalarArith::SVMulToScalarArith(Layer fromLayer, Layer toLayer, VecType vtype)
{
  m_fromLayer = fromLayer;
  m_toLayer = toLayer;
  m_vecType = vtype;
}

bool SVMulToScalarArith::CanApply(const Node* node) const
{
  if (node->GetNodeClass() == SVMul::GetClass()) {
    const SVMul* svmul = static_cast<const SVMul*>(node);
    if (svmul->GetLayer() == m_fromLayer) {
      if ((*(svmul->GetInputM(1)) == 1 && m_vecType == ROWVECTOR) ||
	  (*(svmul->GetInputN(1)) == 1 && m_vecType == COLVECTOR)) {
	return true;
      }
    }
    return false;
  }
  cout << "ERROR: SVMulToScalarArith applied to non SVMul node" << endl;
  LOG_FAIL("replacement for throw call");
  throw;
}

void SVMulToScalarArith::Apply(Node* node) const
{
  SVMul* svmul = static_cast<SVMul*>(node);

  // Split up the input vector
  SplitSingleIter* splitVec;
  if (m_vecType == ROWVECTOR) {
    splitVec = new SplitSingleIter(PARTRIGHT, POSSTUNIN, true);
  } else {
    splitVec = new SplitSingleIter(PARTDOWN, POSSTUNIN, true);
  }

  splitVec->AddInput(svmul->Input(1), svmul->InputConnNum(1));

  if (m_vecType == ROWVECTOR) {
    splitVec->SetUpStats(FULLUP, NOTUP,
			  FULLUP, NOTUP);
  } else {
    splitVec->SetUpStats(FULLUP, FULLUP,
			  NOTUP, NOTUP);
  }

  LoopTunnel* scalarTun = new LoopTunnel(POSSTUNIN);
  scalarTun->AddInput(svmul->Input(0), svmul->InputConnNum(0));
  scalarTun->SetAllStats(FULLUP);

  MulScalars* sMul = new MulScalars();
  sMul->AddInput(scalarTun, 0);
  sMul->AddInput(splitVec, 1);

  // Create output tunnel for scalar
  LoopTunnel* scalarOut = new LoopTunnel(POSSTUNOUT);
  scalarOut->AddInput(scalarTun, 0);
  scalarOut->AddInput(scalarTun, 1);
  scalarOut->CopyTunnelInfo(scalarTun);

  // Combine resulting vector
  CombineSingleIter* combineVec = splitVec->CreateMatchingCombine(1, 1, sMul, 0);

  // Create poss
  Poss* loopPoss = new Poss(2, combineVec, scalarOut);
  RealLoop* loop = new RealLoop(LLDLALOOP, loopPoss, UnitBS);
  
  // Adding loop to poss and cleanup
  node->m_poss->AddPSet(loop);
  node->RedirectChildren(loop->OutTun(0), 0);
  node->m_poss->DeleteChildAndCleanUp(node);
  return;
}

string SVMulToScalarArith::GetType() const
{
  if (m_vecType == ROWVECTOR) {
    return "SVMul scalar arith - Row vector " + LayerNumToStr(m_fromLayer)
      + " to " + LayerNumToStr(m_fromLayer);
  } else {
    return "SVMul scalar arith - Col vector " + LayerNumToStr(m_fromLayer)
      + " to " + LayerNumToStr(m_fromLayer);
  }
}

ResidualPartitionSVMul::ResidualPartitionSVMul(Layer fromLayer, Layer toLayer, VecType vType, Size blockSize) {
  m_fromLayer = fromLayer;
  m_toLayer = toLayer;
  m_vecType = vType;
  m_blockSize = blockSize;
}

string ResidualPartitionSVMul::GetType() const
{
  if (m_vecType == ROWVECTOR) {
    return "Row vector ResidualPartitionSVMul " + LayerNumToStr(m_fromLayer) + " to " + LayerNumToStr(m_toLayer) + " with dim = " + std::to_string((long long int) m_blockSize);
  } else {
    return "Col vector ResidualPartitionSVMul " + LayerNumToStr(m_fromLayer) + " to " + LayerNumToStr(m_toLayer) + " with dim = " + std::to_string((long long int) m_blockSize);
  }
}

bool ResidualPartitionSVMul::CanApply(const Node* node) const
{
  if (node->GetNodeClass() == SVMul::GetClass()) {
    const SVMul* svmul = static_cast<const SVMul*>(node);
    if (m_vecType == ROWVECTOR && svmul->GetVecType() == ROWVECTOR) {
      return !(svmul->GetInputN(1)->EvenlyDivisibleBy(m_blockSize));
    } else if (m_vecType == COLVECTOR && svmul->GetVecType() == COLVECTOR) {
      return !(svmul->GetInputM(1)->EvenlyDivisibleBy(m_blockSize));
    } else {
      return false;
    }
  }
  cout << "ERROR: Cannot apply ResidualPartitionSVMul to non SVMul node" << endl;
  LOG_FAIL("replacement for throw call");
  throw;
}

void ResidualPartitionSVMul::Apply(Node* node) const
{
  Dir partType = m_vecType == ROWVECTOR ? HORIZONTAL : VERTICAL;
  SVMul* svmul = static_cast<SVMul*>(node);

  Size splitPoint = ResidualSplitPoint(svmul);

  Partition* part = new Partition(m_toLayer, partType, splitPoint);
  part->AddInput(svmul->Input(1), 0);

  SVMul* startSVMul = new SVMul(m_toLayer);
  startSVMul->AddInput(svmul->Input(0), 0);
  startSVMul->AddInput(part, 0);

  SVMul* endSVMul = new SVMul(m_toLayer);
  endSVMul->AddInput(svmul->Input(0), 0);
  endSVMul->AddInput(part, 1);

  Recombine* rec = new Recombine(m_toLayer, partType);
  rec->AddInput(startSVMul, 0);
  rec->AddInput(endSVMul, 0);
  rec->AddInput(svmul->Input(1), 0);

  Poss* poss = node->m_poss;
  poss->AddNode(part);
  poss->AddNode(startSVMul);
  poss->AddNode(endSVMul);
  poss->AddNode(rec);

  node->RedirectChildren(rec, 0);
  poss->DeleteChildAndCleanUp(node);

  return;
}

Size ResidualPartitionSVMul::ResidualSplitPoint(const SVMul* svmul) const
{
  const SizeList* splittingDimSizes;
  if (m_vecType == ROWVECTOR) {
    splittingDimSizes = svmul->GetInputN(1);
  } else {
    splittingDimSizes = svmul->GetInputM(1);
  }

  assert(splittingDimSizes->m_entries.size() == 1);

  SizeEntry* sizeEnt = splittingDimSizes->m_entries[0];

  assert(sizeEnt->m_type == REPEATEDSIZES);

  Size totalSize = sizeEnt->m_valA;
  int ts = (int) totalSize;
  int bs = (int) m_blockSize;
  int splitPoint = ts - (ts % bs);
  return (Size) splitPoint;
}

#endif // DOLLDLA
