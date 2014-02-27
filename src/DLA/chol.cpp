/*
    This file is part of DxTer.
    DxTer is a prototype using the Design by Transformation (DxT)
    approach to program generation.

    Copyright (C) 2013, The University of Texas and Bryan Marker

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


#include "layers.h"
#if DOELEM||DOBLIS

#include "chol.h"
#include "distributions.h"
#include "loopSupport.h"
#include "blas.h"
#include <stdio.h>
#include <string>
#include <iomanip>
#include <sstream>

void Chol::Duplicate(const Node *orig, bool shallow, bool possMerging)
{
  DLANode::Duplicate(orig, shallow, possMerging);
  m_tri = ((Chol*)orig)->m_tri;
}

void Chol::Prop()
{
  if (!IsValidCost(m_cost)) {
    DLAOp<1,1>::Prop();
    const Sizes *ms = GetM(0);
    const Sizes *ns = GetN(0);
    if (! (*ms == *ns)) {
      cout << "chol m_m != m_n :" << GetM(0) << " " << GetN(0) << " on " << GetInputName(0).str() << endl; 
    }
    if (m_layer == SMLAYER) 
      m_cost = GAMMA * 1.0/3 * InputLocalM(0)->SumCubes();
    else if (m_layer == ABSLAYER || m_layer == DMLAYER)
      m_cost = 0;
  }
}

void Chol::PrintCode(IndStream &out)
{
  if (m_layer == ABSLAYER) {
    *out << "AbsChol( " << TriToStr(m_tri) << ", " << GetInputName(0).str() << " );\n";
  }
  else if (m_layer == DMLAYER) {
    *out << "DMChol( " << TriToStr(m_tri) << ", " << GetInputName(0).str() << " );\n";
  }
  else if (m_layer == SMLAYER) {
    out.Indent();
    *out << "internal::LocalCholesky( " << TriToStr(m_tri) << ", " << GetInputName(0).str() << " );\n";
  }
  else
    throw;
}

#if DOELEM
DistType Chol::GetDistType(unsigned int num) const
{
  if (m_layer == ABSLAYER || m_layer == DMLAYER)
    return D_MC_MR;
  else if (m_layer == SMLAYER)
    return D_STAR_STAR;
  else
    throw;
}
#endif


void Chol::FlattenCore(ofstream &out) const
{
  DLAOp<1,1>::FlattenCore(out);
  WRITE(m_tri);
}

void Chol::UnflattenCore(ifstream &in, SaveInfo &info)
{
  DLAOp<1,1>::UnflattenCore(in, info);
  READ(m_tri);
}

bool Chol::ShouldCullDP() const 
{
  if (m_layer == ABSLAYER || m_layer == DMLAYER)
    return true;
  else 
    return false;
}


Phase Chol::MaxPhase() const
{
#if DODPPHASE
  if (m_layer == ABSLAYER || m_layer == DMLAYER)
    return DPPHASE;
  else 
#if DOLAPHASE
  return LAPHASE;
#else
  return NUMPHASES;
#endif
#else
  throw;
#endif
}

void Chol::SanityCheck()
{
  DLAOp<1,1>::SanityCheck();
  if (m_inputs.size() != 1)
    cout << "m_inputs.size() != 1\n";
#if DOELEM
  if (m_layer == DMLAYER) {
    if (InputDistType(0) != D_MC_MR)
      cout << "input not D_MC_MR";
  }
  else if (m_layer == SMLAYER) {
    if (InputDistType(0) != D_STAR_STAR) {
      cout << "input not D_STAR_STAR";
      throw;
    }
    if (!(*InputLocalM(0) == *InputLocalN(0)))
      throw;
  }
#endif
}

string CholLoopExp::GetType() const 
{
  std::stringstream str;
  str << "CholLoop expansion variant "
      << m_varNum;
  return str.str();

}

bool CholLoopExp::CanApply(const Poss *poss, const Node *node) const
{
  if (node->GetNodeClass() != Chol::GetClass()) 
    return false;
  const Chol *chol = (Chol*)node;
  return chol->GetLayer() == ABSLAYER;
}

void CholLoopExp::Apply(Poss *poss, Node *node) const
{
  Chol *chol = (Chol*)node;
  Loop *loop;
  if (chol->m_tri == LOWER) {
    if (m_varNum == 3)
      loop = Chol3LowerAlg(chol->Input(0), chol->InputConnNum(0), true);
    else if (m_varNum == 2)
      loop = Chol2LowerAlg(chol->Input(0), chol->InputConnNum(0), true);
    else if (m_varNum == 1)
      loop = Chol1LowerAlg(chol->Input(0), chol->InputConnNum(0), true);
    else
      throw;
  }
  else {
    if (m_varNum == 3)
      loop = Chol3UpperAlg(chol->Input(0), chol->InputConnNum(0), true);
    else if (m_varNum == 2)
      loop = Chol2UpperAlg(chol->Input(0), chol->InputConnNum(0), true);
    else
      throw;
  }

  poss->AddLoop(loop);
  
  node->RedirectChildren(loop->OutTun(0),0);
  node->m_poss->DeleteChildAndCleanUp(node);
} 

#if DOELEM

bool DistCholToLocalChol::CanApply(const Poss *poss, const Node *node) const
{
  if (node->GetNodeClass() != Chol::GetClass())
    return false;
  const Chol *chol = (Chol*)node;
  return chol->GetLayer() == DMLAYER;
}

void DistCholToLocalChol::Apply(Poss *poss, Node *node) const
{
  Chol *chol = (Chol*)node;
  RedistNode *node1 = new RedistNode(D_STAR_STAR);
  Chol *node2 = new Chol(SMLAYER, chol->m_tri);
  RedistNode *node3 = new RedistNode(D_MC_MR);
  node1->AddInput(node->Input(0),node->InputConnNum(0));
  node2->AddInput(node1,0);
  node3->AddInput(node2,0);
  poss->AddNodes(3, node1, node2, node3);
  node->RedirectChildren(node3,0);
  node->m_poss->DeleteChildAndCleanUp(node);
}
#endif


Loop* Chol1LowerAlg(Node *in, unsigned int num, bool dist)
{
  Split *splitA = new Split(PARTDIAG, POSSTUNIN, true);
  splitA->AddInput(in, num);
  splitA->SetUpStats(FULLUP, FULLUP,
		     NOTUP, NOTUP);
  
  Trxm *trsm;
  if (dist)
    trsm = new Trxm(true, DMLAYER, RIGHT, LOWER, NONUNIT, CONJTRANS, COEFONE, COMPLEX);
  else
    trsm = new Trxm(true, SMLAYER, RIGHT, LOWER, NONUNIT, CONJTRANS, COEFONE, COMPLEX);
  trsm->AddInputs(4,
		  splitA, 0,
		  splitA, 1);
  Poss *poss4 = new Poss(trsm,false);
  PSet *set4 = new PSet(poss4);
  
  Node *tri;
  if (dist) {
    tri = new Herk(DMLAYER, LOWER, NORMAL, COEFNEGONE, COEFONE, COMPLEX);
    tri->AddInputs(4,set4->OutTun(0),0,splitA,4);
  }
  else {
    throw;
  }
  Poss *poss1 = new Poss(tri,false);
  PSet *set1 = new PSet(poss1);
  
  Chol *chol;
  if (dist)
    chol = new Chol(DMLAYER, LOWER);
  else
    throw;
  chol->AddInput(set1->OutTun(0),0);
  Poss *poss2 = new Poss(chol,false);
  PSet *set2 = new PSet(poss2);
  
  Combine *comA = splitA->CreateMatchingCombine(2, 
						1, set4->OutTun(0), 0,
						4, set2->OutTun(0), 0);
  
  Poss *loopPoss = new Poss(1, comA);
  Loop *loop;
  if (dist)
    loop = new Loop(ELEMLOOP, loopPoss, USEELEMBS);
  else
    throw;
    //loop = new Loop(BLISLOOP, loopPoss, FLAME_BS);

  return loop;
}

Loop* Chol2LowerAlg(Node *in, unsigned int num, bool dist)
{
  Split *splitA = new Split(PARTDIAG, POSSTUNIN, true);
  splitA->AddInput(in, num);
  splitA->SetUpStats(FULLUP, FULLUP,
		     FULLUP, NOTUP);
  
  Node *tri;
  if (dist) {
    tri = new Gemm(DMLAYER, NORMAL, CONJTRANS, COEFONE, COEFNEGONE, COMPLEX);
    tri->AddInputs(6,splitA,1,splitA,1,splitA,4);

  }
  else {
    throw;
  }
  Poss *poss1 = new Poss(tri,false);
  PSet *set1 = new PSet(poss1);
  
  Chol *chol;
  if (dist)
    chol = new Chol(DMLAYER, LOWER);
  else
    throw;
  chol->AddInput(set1->OutTun(0),0);
  Poss *poss2 = new Poss(chol,false);
  PSet *set2 = new PSet(poss2);
  
  Gemm *gemm2;
  if (dist)
    gemm2 = new Gemm(DMLAYER, NORMAL, CONJTRANS, COEFONE, COEFNEGONE, COMPLEX);
  else
    gemm2 = new Gemm(SMLAYER, NORMAL, CONJTRANS, COEFONE, COEFNEGONE, COMPLEX);
  gemm2->AddInputs(6,splitA,2,splitA,1,splitA,5);
  Poss *poss3 = new Poss(gemm2,false);
  PSet *set3 = new PSet(poss3);
  
  Trxm *trsm;
  if (dist)
    trsm = new Trxm(true, DMLAYER, RIGHT, LOWER, NONUNIT, CONJTRANS, COEFONE,COMPLEX);
  else
    trsm = new Trxm(true, SMLAYER, RIGHT, LOWER, NONUNIT, CONJTRANS, COEFONE,COMPLEX);
  trsm->AddInputs(4,
		  set2->OutTun(0), 0,
		  set3->OutTun(0), 0);
  Poss *poss4 = new Poss(trsm,false);
  PSet *set4 = new PSet(poss4);
  
  Combine *comA = splitA->CreateMatchingCombine(2, 
						4, set2->OutTun(0), 0,
						5, set4->OutTun(0), 0);
  
  Poss *loopPoss = new Poss(1, comA);
  Loop *loop;
  if (dist)
    loop = new Loop(ELEMLOOP, loopPoss, USEELEMBS);
  else
    throw;
  //    loop = new Loop(BLISLOOP, loopPoss, FLAME_BS);

  return loop;
}

Loop* Chol2UpperAlg(Node *in, unsigned int num, bool dist)
{
  Split *splitA = new Split(PARTDIAG, POSSTUNIN, true);
  splitA->AddInput(in, num);
  splitA->SetUpStats(FULLUP, FULLUP,
		     FULLUP, NOTUP);
  
  Node *tri;
  if (dist) {
    tri = new Herk(DMLAYER, UPPER, CONJTRANS, COEFNEGONE, COEFONE, COMPLEX);
  }
  else {
    throw;
    //tri = new LocalSyrk(UPPER, CONJTRANS, COEFNEGONE, COEFONE);
    //    tri->AddInputs(4,splitA,3,splitA,4);
  }
  tri->AddInputs(4, splitA, 3, splitA, 4);
  Poss *poss1 = new Poss(tri,false);
  PSet *set1 = new PSet(poss1);
  
  Chol *chol;
  if (dist)
    chol = new Chol(DMLAYER, UPPER);
  else
    throw;
  chol->AddInput(set1->OutTun(0),0);
  Poss *poss3 = new Poss(chol,false);
  PSet *set3 = new PSet(poss3);

  Gemm *gemm;
  if (dist)
    gemm = new Gemm(DMLAYER, CONJTRANS, NORMAL, COEFNEGONE, COEFONE,COMPLEX);
  else
    gemm = new Gemm(SMLAYER, CONJTRANS, NORMAL, COEFNEGONE, COEFONE, COMPLEX);
  gemm->AddInputs(6,splitA,3,splitA,6,splitA,7);
  Poss *poss2 = new Poss(gemm,false);
  PSet *set2 = new PSet(poss2);
  
  Trxm *trsm;
  if (dist)
    trsm = new Trxm(true, DMLAYER, LEFT, UPPER, NONUNIT, CONJTRANS,COEFONE,COMPLEX);
  else
    trsm = new Trxm(true, SMLAYER, LEFT, UPPER, NONUNIT, CONJTRANS,COEFONE,COMPLEX);
  trsm->AddInputs(4,
		  set3->OutTun(0), 0,
		  set2->OutTun(0), 0);
  Poss *poss4 = new Poss(trsm,false);
  PSet *set4 = new PSet(poss4);
  
  Combine *comA = splitA->CreateMatchingCombine(2, 
						4, set3->OutTun(0), 0,
						7, set4->OutTun(0), 0);
  
  Poss *loopPoss = new Poss(1, comA);
  Loop *loop;
  if (dist)
    loop = new Loop(ELEMLOOP, loopPoss, USEELEMBS);
  else
    throw;
    //loop = new Loop(BLISLOOP, loopPoss, FLAME_BS);

  return loop;
}

Loop* Chol3LowerAlg(Node *in, unsigned int num, bool dist)
{
  Split *splitA = new Split(PARTDIAG, POSSTUNIN, true);
  splitA->AddInput(in, num);
  splitA->SetUpStats(FULLUP, FULLUP,
		    FULLUP, PARTUP);

  Chol *chol;
  if (dist)
    chol = new Chol(DMLAYER, LOWER);
  else
    throw;
  chol->AddInput(splitA, 4);
  Poss *poss1 = new Poss(chol,false);
  PSet *set1 = new PSet(poss1);

  Trxm *trsm;
  if (dist)
    trsm = new Trxm(true, DMLAYER, RIGHT, LOWER, NONUNIT, CONJTRANS, COEFONE,COMPLEX);
  else
    trsm = new Trxm(true, SMLAYER, RIGHT, LOWER, NONUNIT, CONJTRANS, COEFONE,COMPLEX);
  trsm->AddInputs(4, set1->OutTun(0), 0, splitA, 5);
  Poss *poss2 = new Poss(trsm,false);
  PSet *set2 = new PSet(poss2);


  Node *tri;
  if (dist) {
    tri = new Herk(DMLAYER, LOWER, NORMAL, COEFNEGONE, COEFONE, COMPLEX);
    tri->AddInputs(4,
		   set2->OutTun(0), 0,
		   splitA, 8);
  }
  else {
    throw;
    //tri = new LocalSyrk(LOWER, NORMAL,  COEFNEGONE, 1);
    //    tri->AddInputs(4,
    //		   set2->OutTun(0), 0,
    //		   splitA, 8);
  }
  Poss *poss3 = new Poss(tri,false);
  PSet *set3 = new PSet(poss3);

  Combine *comA = splitA->CreateMatchingCombine(3,
						4, set1->OutTun(0), 0,
						5, set2->OutTun(0), 0,
						8, set3->OutTun(0), 0);

  Poss *loopPoss = new Poss(1, comA);
  Loop *loop;
  if (dist)
    loop = new Loop(ELEMLOOP, loopPoss, USEELEMBS);
  else
    throw;
  //    loop = new Loop(BLISLOOP, loopPoss, FLAME_BS);

  return loop;
}


Loop* Chol3UpperAlg(Node *in, unsigned int num, bool dist)
{
  Split *splitA = new Split(PARTDIAG, POSSTUNIN, true);
  splitA->AddInput(in,num);
  splitA->SetUpStats(FULLUP, FULLUP,
		     FULLUP, PARTUP);
    
    Chol *chol;
    if (dist)
      chol = new Chol(DMLAYER, UPPER);
    else
      throw;
    chol->AddInput(splitA, 4);
    Poss *poss1 = new Poss(chol,false);
    PSet *set1 = new PSet(poss1);
    
    Trxm *trsm;
    if (dist)
      trsm = new Trxm(true, DMLAYER, LEFT, UPPER, NONUNIT, CONJTRANS, COEFONE,COMPLEX);
    else
      trsm = new Trxm(true, SMLAYER, LEFT, UPPER, NONUNIT, CONJTRANS, COEFONE, COMPLEX);
    trsm->AddInputs(4, set1->OutTun(0), 0, splitA, 7);
    Poss *poss2 = new Poss(trsm,false);
    PSet *set2 = new PSet(poss2);
    
    Node *tri;
    if (dist) {
      tri = new Herk(DMLAYER, UPPER, CONJTRANS, COEFNEGONE, COEFONE, COMPLEX);
      tri->AddInputs(4,
		     set2->OutTun(0), 0,
		     splitA, 8);
    }
    else {
      throw;
      /*tri = new LocalSyrk(UPPER, CONJTRANS, COEFNEGONE, 1);
      tri->AddInputs(4,
		     set2->OutTun(0), 0,
		     splitA, 8);*/
    }
    Poss *poss3 = new Poss(tri,false);
    PSet *set3 = new PSet(poss3);
    
    Combine *comA = new Combine(PARTDIAG, POSSTUNOUT);
    comA->AddInput(splitA,0);
    comA->AddInput(splitA,1);
    comA->AddInput(splitA,2);
    comA->AddInput(splitA,3);
    comA->AddInput(set1->OutTun(0),0);
    comA->AddInput(splitA,5);
    comA->AddInput(splitA,6);
    comA->AddInput(set2->OutTun(0),0);
    comA->AddInput(set3->OutTun(0),0);
    comA->AddInput(splitA,9);
	
    comA->CopyTunnelInfo(splitA);
    
    Poss *loopPoss = new Poss(1, comA);
    Loop *loop;
    if (dist)
      loop = new Loop(ELEMLOOP, loopPoss, USEELEMBS);
    else
      throw;
      //loop = new Loop(BLISLOOP, loopPoss, FLAME_BS);
    
    return loop;
}

#endif
