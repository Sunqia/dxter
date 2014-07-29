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



#include "loopSupport.h"
#include "elemRedist.h"
#include <cmath>
#include <climits>
#include "pack.h"
#include "critSect.h"
#include "blis.h"

Size BSSizeToSize(BSSize size)
{
  switch(size)
  {
#if DOELEM
    case (USEELEMBS):
      return ELEM_BS;
#elif DOBLIS
    case (USEBLISMC):
      return BLIS_MC_BS;
    case (USEBLISKC):
      return BLIS_KC_BS;
    case (USEBLISNC):
      return BLIS_NC_BS;
    case (USEBLISOUTERBS):
      return BLIS_OUTER_BS;
#elif DOTENSORS
  case (USETENSORBS):
    return TENSOR_BS;
#elif DOLLDLA
  case (USELLDLAMU):
    return LLDLA_MU;
  case (USELLDLA2MU):
    return 2*LLDLA_MU;
  case (USELLDLA3MU):
    return 3*LLDLA_MU;
#endif
  case (USEUNITBS):
    return ONE;
    default:
      throw;
  }
}

string BSSizeToVarName(BSSize size)
{
  switch(size)
    {
#if DOLLDLA
    case (USELLDLAMU):
      return MU_VAR_NAME;
    case (USELLDLA2MU):
      return "(2*" + (string)(MU_VAR_NAME) + ")";
    case (USELLDLA3MU):
      return "(3*" + (string)(MU_VAR_NAME) + ")";
#endif
    default:
      throw;
    }
}



string BSSizeToStr(BSSize size)
{
  switch(size)
  {
#if DOELEM
    case (USEELEMBS):
      throw;
#elif DOBLIS
    case (USEBLISMC):
      return "gemm_mc";
    case (USEBLISKC):
      return "gemm_kc";
    case (USEBLISNC):
      return "gemm_nc";
    case (USEBLISOUTERBS):
      return "bs_obj";
#endif
    default:
      throw;
  }
}

string BSSizeToSubSizeStr(BSSize size)
{
  switch(size)
  {
#if DOELEM
    case (USEELEMBS):
      throw;
#elif DOBLIS
    case (USEBLISMC):
      return "gemm_mr";
    case (USEBLISKC):
      return "gemm_kr";
    case (USEBLISNC):
      return "gemm_nr";
    case (USEBLISOUTERBS):
      throw;
#endif
    default:
      throw;
  }
}

unsigned int GetNumElems(PartDir dir)
{
  switch(dir)
  {
    case(PARTRIGHT):
    case(PARTDOWN):
    case(PARTLEFT):
    case(PARTUPWARD):
      return 3;
    case (PARTDIAG):
    case (PARTDIAGBACK):
      return 9;
    default:
      cout << "Bad PartDir\n";
      return -1;
  }
}

string PartDirToStr(PartDir dir)
{
  switch(dir)
  {
    case(PARTRIGHT):
      return "right";
    case(PARTDOWN):
      return "down";
    case (PARTDIAG):
      return "diag";
    case(PARTLEFT):
      return "rightBack";
    case(PARTUPWARD):
      return "downBack";
    case (PARTDIAGBACK):
      return "diagBack";
    default:
      cout << "Bad PartDir\n";
      return "bad";
  }
}

template <class PSetType>
void IntLoop<PSetType>::Prop()
{
  PSetType::Prop();
  
  bool foundControl = false;
  NodeVecIter iter = PSetType::m_inTuns.begin();
  for(; iter != PSetType::m_inTuns.end(); ++iter) {
    Node *in = *iter;
    if (!in->IsLoopTunnel()) {
      cout << "non loop tunnel on loop!\n";
      throw;
    }
    if (((LoopTunnel*)in)->IsSplit()) {
      SplitBase *split = (SplitBase*)in;
      if (split->m_isControlTun) {
        if (foundControl)
          throw;
        else
          foundControl = true;
      }
    }
  }
  if (!foundControl)
    throw;
  iter = PSetType::m_outTuns.begin();
  for(; iter != PSetType::m_outTuns.end(); ++iter)
    if (!(*iter)->IsLoopTunnel()) {
      cout << "non loop tunnel on loop!\n";
      throw;
    }
}

template <class PSetType>
bool IntLoop<PSetType>::CanMerge(BasePSet *pset) const
{
  if (!pset->IsLoop())
    return false;
  LoopInterface *loop = dynamic_cast<LoopInterface*>(pset);
  if (GetBS() != loop->GetBS())
    return false;
  //  Loop *loop = (Loop*)pset;
#if DOBLIS
  if (loop->m_comm != CORECOMM || m_comm != CORECOMM)
    return false;
#endif
  if (GetType() != loop->GetType())
    return false;
  const SplitBase *splitBase1 = GetControl();
  const SplitBase *splitBase2 = loop->GetControl();
  if (splitBase1->GetNodeClass() != SplitSingleIter::GetClass() ||
      splitBase2->GetNodeClass() != SplitSingleIter::GetClass())
    return false;
  const SplitSingleIter *split1 = (SplitSingleIter*)splitBase1;
  const SplitSingleIter *split2 = (SplitSingleIter*)splitBase2;
  if (split1->NumberOfLoopExecs() != split2->NumberOfLoopExecs())
    return false;

  for(unsigned int i = 0; i < split1->NumberOfLoopExecs(); ++i) {
    if (split1->NumIters(i) != split2->NumIters(i))
      return false;
  }
  
#if DOSQOPHASE
  //If there's a PACKABLOCK in both sets, then we don't
  // want to fuse those loops.  We don't merge the packing
  // yet and they're likely using other packed B panels for the
  // computation.  That will make it impossible to separate the
  // different uses of the single packed B panel buffer
  bool found1 = false;
  bool found2 = false;
  for(unsigned int i = 0; !found1 && i < m_posses[0]->m_possNodes.size(); ++i) {
    Node *node = m_posses[0]->m_possNodes[i];
    if (node->GetNodeClass() == PackBuff::GetClass()) {
      PackBuff *buff = (PackBuff*)node;
      if (buff->m_packMat == PACKABLOCK)
        found1 = true;
    }
  }
  if (found1) {
    for(unsigned int i = 0; !found2 && i < loop->m_posses[0]->m_possNodes.size(); ++i) {
      Node *node = loop->m_posses[0]->m_possNodes[i];
      if (node->GetNodeClass() == PackBuff::GetClass()) {
        PackBuff *buff = (PackBuff*)node;
        if (buff->m_packMat == PACKABLOCK)
          found2 = true;
      }
    }
    if (found1 && found2)
      return false;
  }
#endif //DOSQOPHASE
  
  if (!PSetType::CanMerge(pset))
    return false;
  
  const BasePSet *left=NULL, *right=NULL;
  //this is true if the left loop is actually on the left
  //otherwise, the order doesn't matter
  NodeVecConstIter iter = pset->m_inTuns.begin();
  for(; iter != pset->m_inTuns.end() && !left; ++iter) {
    const Node *inTun = *iter;
    for (ConnNum i = 0; i < inTun->m_inputs.size() && !left; ++i) {
      if (inTun->Input(i)->IsTunnel(SETTUNOUT) &&
          ((Tunnel*)inTun->Input(i))->m_pset == this) {
        right = pset;
        left = this;
      }
    }
  }
  if (!left) {
    iter = PSetType::m_inTuns.begin();
    for(; iter != PSetType::m_inTuns.end() && !left; ++iter) {
      const Node *inTun = *iter;
      for (ConnNum i = 0; i < inTun->m_inputs.size() && !left; ++i) {
        if (inTun->Input(i)->IsTunnel(SETTUNOUT) &&
            ((Tunnel*)inTun->Input(i))->m_pset == pset) {
          left = pset;
          right = this;
        }
      }
    }
  }
  
  if (left) {
    //make sure the output of the left loop is either only used as input to the
    // right loop or the right loop doesn't change it at all
    iter = left->m_outTuns.begin();
    for(; iter != left->m_outTuns.end(); ++iter) {
      LoopTunnel *leftOutTun = (LoopTunnel*)*iter;
      NodeConnVecConstIter connIter = leftOutTun->m_children.begin();
      bool foundConnToRightNotUpdated = false;
      
      bool foundConnNotToRight = false;
      for(; connIter != leftOutTun->m_children.end(); ++connIter) {
        const NodeConn *conn = *connIter;
        if (conn->m_n->IsLoopTunnel() && ((LoopTunnel*)conn->m_n)->m_pset == right) {
          LoopTunnel *rightInTun = (LoopTunnel*)conn->m_n;
          foundConnToRightNotUpdated |= !rightInTun->AllFullyUpdated();
          if (foundConnNotToRight) {
            if(foundConnToRightNotUpdated)
              return false;
          }
          //Check if the way the inputs/outputs are split are ok
          // for fusion
          if (leftOutTun->GetNodeClass() == CombineSingleIter::GetClass()) {
            if (rightInTun->GetNodeClass() == SplitSingleIter::GetClass()) {
#if TWOD
              if (((CombineSingleIter*)leftOutTun)->m_dir != ((SplitSingleIter*)rightInTun)->m_dir) {
#else
              if (((CombineSingleIter*)leftOutTun)->m_partDim != ((SplitSingleIter*)rightInTun)->m_partDim) {
#endif
                if (!leftOutTun->IsConst() || !rightInTun->IsConst())
                  return false;
                return false;
                cout << "not yet supporting different directions";
                throw;
              }
            }
            else {
              if (!leftOutTun->IsConst() || !rightInTun->IsConst()) {
                return false;
              }
            }
          }
          else {
            if (rightInTun->GetNodeClass() == SplitSingleIter::GetClass()) {
              if (!leftOutTun->IsConst() || !rightInTun->IsConst()) {
                return false;
              }
            }
          }
        }
        else {
          foundConnNotToRight = true;
          if (foundConnToRightNotUpdated)
            return false;
        }
      }
    }
    
    //check we wouldn't have a cycle
    iter = right->m_outTuns.begin();
    for(; iter != right->m_outTuns.end(); ++iter) {
      const Node *outTun = *iter;
      for (unsigned int i = 0; i < outTun->m_children.size(); ++i) {
        if (outTun->Child(0)->IsTunnel(SETTUNIN) &&
            ((Tunnel*)outTun->Child(0))->m_pset == left) {
          cout << "found cycle\n";
          throw;
        }
      }
    }
    
    
    //If the right loop uses a quadrant of a particular input
    // that is output by the left loop, make sure the left loop has
    // completely computed that quadrant
    iter = right->m_inTuns.begin();
    for(; iter != right->m_inTuns.end(); ++iter) {
      if (!(*iter)->IsLoopTunnel()) {
        throw;
      }
      const LoopTunnel *rightInTun = (LoopTunnel*)*iter;
      for (ConnNum i = 0; i < rightInTun->m_inputs.size() && left; ++i) {
        if (rightInTun->Input(0)->IsLoopTunnel() && rightInTun->Input(0)->IsTunnel(SETTUNOUT)) {
          const LoopTunnel *leftOutTun = (LoopTunnel*)(rightInTun->Input(0));
          if (((Tunnel*)leftOutTun)->m_pset == left) {
            for(int quad = TL; quad < LASTQUAD; ++quad) {
              if (rightInTun->QuadInUse((Quad)quad, true)) {
                if (leftOutTun->GetUpStat((Quad)quad) != FULLUP) {
                  return false;
                }
              }
            }
          }
        }
      }
    }
    
    
    //If the left loop uses a quadrant of an output
    // used by the right loop, make sure the right loop
    // doesn't change it
    iter = left->m_inTuns.begin();
    for(; iter != left->m_inTuns.end(); ++iter) {
      if (!(*iter)->IsLoopTunnel()) {
        throw;
      }
      const LoopTunnel *leftInTun = (LoopTunnel*)*iter;
      const LoopTunnel *leftOutTun = leftInTun->GetMatchingOutTun();
      for(int quad = TL; quad < LASTQUAD; ++quad) {
        if (leftInTun->QuadInUse((Quad)quad,false)) {
          for (unsigned int i = 0; i < leftOutTun->m_children.size(); ++i) {
            if (leftOutTun->Child(i)->IsLoopTunnel()) {
              LoopTunnel *rightInTun = (LoopTunnel*)(leftOutTun->Child(i));
              if (rightInTun->m_pset == right) {
                if (!rightInTun->IsConst() && rightInTun->GetUpStat((Quad)quad) != NOTUP)
                  return false;
              }
            }
          }
        }
      }
    }
  }
  else {
    const BasePSet *leftSet = NULL;
    bool foundConnection = false;
    iter = pset->m_inTuns.begin();
    for (; iter != pset->m_inTuns.end(); ++iter) {
      LoopTunnel *tun1 = (LoopTunnel*)(*iter);
      Node *input = tun1->Input(0);
      ConnNum inNum = tun1->InputConnNum(0);
      NodeConnVecIter childIter = input->m_children.begin();
      for(; childIter != input->m_children.end(); ++childIter) {
        if ((*childIter)->m_num == inNum) {
          Node *child = (*childIter)->m_n;
          if (child->IsLoopTunnel() && ((LoopTunnel*)child)->m_pset == this) {
            LoopTunnel *tun2 = (LoopTunnel*)child;
            foundConnection = true;
            if (!tun1->IsConst() || !tun2->IsConst()) {
              if (tun1->IsConst()) {
                if (leftSet) {
                  if (leftSet != pset) {
                    cout << "!!!! Not supported\n";
                    return false;
                  }
                }
                else
                  leftSet = pset;
                LoopTunnel *outTun = tun1->GetMatchingOutTun();
                if (outTun) {
                  if (outTun->m_children.size()) {
                    cout << "out tun has children\n";
                    throw;
                  }
                }
              }
              else if (tun2->IsConst()){
                if (leftSet) {
                  if (leftSet != this) {
                    cout << "!!!! Not supported\n";
                    return false;
                  }
                }
                else
                  leftSet = this;
                LoopTunnel *outTun = tun2->GetMatchingOutTun();
                if (outTun) {
                  if (outTun->m_children.size()) {
                    cout << "out tun has children\n";
                    throw;
                  }
                }
              }
              else {
                cout << "not supported yet...\n";
                //Here, we might be able to fuse these two loops
                // that both use the same input...
                throw;
              }
              
              if (!leftSet)
                throw;
              
              LoopTunnel *leftTun;
              LoopTunnel *rightTun;
              
              if (leftSet == this) {
                leftTun = tun2;
                rightTun = tun1;
              }
              else {
                leftTun = tun1;
                rightTun = tun2;
              }
              
              for(int quad = TL; quad < LASTQUAD; ++quad) {
                if (leftTun->QuadInUse((Quad)quad,false)) {
                  if (!rightTun->IsConst() && rightTun->GetUpStat((Quad)quad) != NOTUP)
                    return false;
                }
              }
            }
          }
        }
      }
    }
    
    if (!foundConnection) {
      throw;
      return false;
    }
  }
  return true;
}
  
template<class PSetType>
  void IntLoop<PSetType>::PrePrint(IndStream &out, Poss *poss)
{
  PSetType::PrePrint(out, poss);
  NodeVecIter iter = poss->m_inTuns.begin();
  for(; iter != poss->m_inTuns.end(); ++iter) {
    if (((LoopTunnel*)(*iter))->IsSplit()) {
      ((SplitBase*)(*iter))->PrintVarDeclarations(out);
    }
  }

#if DOBLIS
  string loopLevel = out.LoopLevel(1);
  
  if (GetType() == BLISLOOP) {
    if (m_comm != CORECOMM) {
      bool barrier = false;
      iter = m_inTuns.begin();
      for(; iter != m_inTuns.end() && !barrier; ++iter) {
	if (!FoundBarrier(*iter, 0, m_comm))
	  barrier = true;
      }
      if (barrier) {
	out.Indent();
	*out << "th_barrier( " << CommToStr(m_comm) << " );"
	     << "\t //barrier for dependency\n";

      }
      out.Indent();
      *out << "//// ***Parallelized with communicator "
	   << CommToStr(m_comm) << endl;
    }
    string idx = "idx" + loopLevel;
    string dimLen = "dimLen" + loopLevel;
    string bs = "bs" + loopLevel;
    
    SplitBase *splitBase = GetControl();
    if (splitBase->GetNodeClass() != SplitSingleIter::GetClass())
      throw;
    SplitSingleIter *split = (SplitSingleIter*)splitBase;
    
    string inputName = split->Input(0)->GetName(split->InputConnNum(0)).str();
    
    if (loopLevel == "1") {
      out.Indent();
      *out << "dim_t " << idx << ", " << dimLen << ", " << bs << ";\n";
    }
    
    
    switch(split->m_dir) {
      case(PARTDIAGBACK):
      case(PARTDIAG):
        out.Indent();
        *out << dimLen << " = min( bli_obj_length_after_trans( " << inputName << " ), "
        << "bli_obj_width_after_trans( " << inputName << " ) );\n";
        break;
      case(PARTDOWN):
      case(PARTUPWARD):
        out.Indent();
        *out << dimLen << " = bli_obj_length_after_trans( " << inputName << " );\n";
        break;
      case (PARTRIGHT):
      case (PARTLEFT):
        out.Indent();
        *out << dimLen << " = bli_obj_width_after_trans( " << inputName << " );\n";
        break;
      default:
        throw;
    }
    out.Indent();
    if (m_comm != CORECOMM) {
      *out << idx << " = 0;\n";
      out.Indent();
      *out << "th_shift_start_end(&" << idx << ", &" << dimLen << ", "
      << CommToStr(GetSubComm(m_comm)) << ", "
      << "bli_blksz_for_obj( &" << split->GetInputNameStr(0)
      << ", " << BSSizeToSubSizeStr(GetBSSize) << "));\n";
      out.Indent();
      *out << "for ( ; " << idx << " < " << dimLen << "; "
      << idx << " += " << bs <<" ) {\n";
    }
    else {
#if DOSM
      Comm outerComm = split->WithinParallelism();
      Comm innerComm = ParallelismWithinCurrentPosses();
      if ((innerComm != ALLPROCCOMM && innerComm != ALLL2COMM) &&
	  (outerComm == CORECOMM || innerComm != GetSubComm(outerComm))) {
        if (innerComm == CORECOMM) {
          if (outerComm == CORECOMM)
            *out << "if (th_global_thread_id() != 0)\n";
          else
            *out << "if (th_thread_id( " << CommToStr(GetSubComm(outerComm)) << " ) != 0)\n";
        }
        else if (outerComm != CORECOMM && innerComm != GetSubComm(GetSubComm(outerComm))) {
          outerComm = split->WithinParallelism();
          innerComm = ParallelismWithinCurrentPosses();
          throw;
        }
        else {  
          *out << "if (th_group_id( " << CommToStr(innerComm) << " ) != 0)\n";
        }
        out.Indent(1);
        *out << dimLen << " = 0;\n";
        out.Indent();
      }
#endif // DOSM
      *out << "for ( " << idx << " = 0; " << idx << " < " << dimLen << "; "
      << idx << " += " << bs <<" ) {\n";
    }
    out.Indent(1);
    *out << bs;
    switch(split->m_dir) {
      case(PARTDOWN):
      case(PARTDIAG):
      case (PARTRIGHT):
        *out << " = bli_determine_blocksize_f( " ;
        break;
      case (PARTLEFT):
      case(PARTUPWARD):
      case(PARTDIAGBACK):
        *out << " = bli_determine_blocksize_b( " ;
        break;
      default:
        throw;
    }
    
    *out << idx << ", " << dimLen
    << ", &" << inputName << ", " << BSSizeToStr(GetBSSize) << " );\n";
    
    loopLevel = out.LoopLevel(2);
    idx = "idx" + loopLevel;
    dimLen = "dimLen" + loopLevel;
    bs = "bs" + loopLevel;
    out.Indent(1);
    *out << "dim_t " << idx << ", " << dimLen << ", " << bs << ";\n";
  }
#elif DOLLDLA
  if (GetType() != LLDLALOOP)
    throw;
  if (GetBSSize() != USELLDLAMU &&
      GetBSSize() != USELLDLA2MU &&
      GetBSSize() != USELLDLA3MU)
    throw;
  SplitBase *split = GetControl();
  switch(GetDimName()) 
    {
    case (DIMM):
      {
	out.Indent();
	*out << "//Dim-m loop\n";;
	break;
      }
    case (DIMN):
      {
	out.Indent();
	*out << "//Dim-n loop\n";
	break;
      }
    case (DIMK):
      {
	out.Indent();
	*out << "//Dim-k loop\n";
	break;
    }
    default:
      break;
    }

  string loopLevel = split->GetLoopLevel();
  string lcv = "lcv" + loopLevel;
  out.Indent();
  *out << "for( " << lcv << " = ";
  
  if (split->GetNodeClass() != SplitSingleIter::GetClass())
    throw;

  bool needMin = false;
  if (split->m_dir == PARTDOWN) {
    *out << split->InputDataType(0).m_numRowsVar;
    if (!split->GetInputM(0)->EvenlyDivisibleBy(GetBS()))
      needMin = true;
  }
  else if (split->m_dir == PARTRIGHT) {
    *out << split->InputDataType(0).m_numColsVar;
    if (!split->GetInputN(0)->EvenlyDivisibleBy(GetBS()))
      needMin = true;
  }
  else
    throw;
  
  *out << "; " << lcv << " > 0; " << lcv << " -= ";

  
  *out << BSSizeToVarName(GetBSSize()) << " ) {\n";

  out.Indent(1);
  *out << "const unsigned int num";
  if (split->m_dir == PARTDOWN) {
    *out << "Rows";
  }
  else if (split->m_dir == PARTRIGHT) {
    *out << "Cols";
  }
  else
    throw;

  if (needMin)
    *out << loopLevel << " = min( " << lcv << ", ";
  else
    *out << loopLevel << " = ( ";
  *out << BSSizeToVarName(GetBSSize()) << " );\n";
#endif
}


template<class PSetType>
  void IntLoop<PSetType>::PostPrint(IndStream &out, Poss *poss)
{
  PSetType::PostPrint(out, poss);

  NodeVecIter iter = poss->m_inTuns.begin();
  for(; iter != poss->m_inTuns.end(); ++iter) {
    LoopTunnel *tun = (LoopTunnel*)(*iter);
    if (tun->IsSplit()) {
      SplitBase *split = (SplitBase*)tun;
      split->PrintIncrementAtEndOfLoop(GetBSSize(), out);
    }
  }

#if DOBLIS||DOLLDLA
  if (GetType() == BLISLOOP || GetType() == LLDLALOOP) {
    out.Indent();
    *out << "}\n";
  }
#endif //DOBLIS||DOLLDLA
}

template <class PSetType>
unsigned int IntLoop<PSetType>::LoopLevel() const
{
  unsigned int level = 0;
  Poss *poss = BasePSet::m_ownerPoss;
  while (poss) {
    if (!poss->m_pset)
      throw;
    if (poss->m_pset->IsLoop())
      ++level;
    poss = poss->m_pset->m_ownerPoss;
  }
  return level;
}
 
template<class PSetType>
bool IntLoop<PSetType>::WorthFusing(BasePSet *pset)
{
  //If we fuse two inner-most BLIS loops, there
  // could be two packed BPANEL buffers input into the same
  // loop, which means the buffers cannot be named the same.
  //Therefore, prohibit such fusion to allow for the same
  // buffer to be reused.
#if DOBLIS
  if (FindOtherPackBuffs((*(GetPosses().begin())).second, PACKABLOCK, NULL)) {
    return false;
  }
#endif
  
  NodeVecIter iter = PSetType::m_outTuns.begin();
  for(; iter != PSetType::m_outTuns.end(); ++iter) {
    Node *out = *iter;
    NodeConnVecIter iter2 = out->m_children.begin();
    for(; iter2 != out->m_children.end(); ++iter2) {
      Node *child = (*iter2)->m_n;
      if (child->IsTunnel(SETTUNIN)) {
        if (((Tunnel*)child)->m_pset == pset)
          return true;
      }
    }
  }
  iter = PSetType::m_inTuns.begin();
  for(; iter != PSetType::m_inTuns.end(); ++iter) {
    Node *in = *iter;
    NodeConnVecIter iter2 = in->m_inputs.begin();
    for(; iter2 != in->m_inputs.end(); ++iter2) {
      Node *input = (*iter2)->m_n;
      if (input->IsTunnel(SETTUNOUT)) {
        if (((Tunnel*)input)->m_pset == pset)
          return true;
      }
      ConnNum num = (*iter2)->m_num;
      NodeConnVecIter iter3 = input->m_children.begin();
      for(; iter3 != input->m_children.end(); ++iter3) {
        if ((*iter3)->m_num == num) {
          Node *otherChild = (*iter3)->m_n;
          if (otherChild->IsTunnel(SETTUNIN)) {
            if (((Tunnel*)otherChild)->m_pset == pset)
              return true;
          }
        }
      }
    }
  }
  return false;
}

 template<class PSetType>
 SplitBase* IntLoop<PSetType>::GetControl() const
{
  SplitBase *control = NULL;
  NodeVecConstIter iter = PSetType::m_inTuns.begin();
  for(; iter != PSetType::m_inTuns.end(); ++iter) {
    LoopTunnel *node = (LoopTunnel*)(*iter);
    if (node->IsSplit()) {
      SplitBase *split = (SplitBase*)node;
      if (split->m_isControlTun) {
        control = split;
      }
    }
  }
  if (!control)
    throw;
  return control;
}


template class IntLoop<RealPSet>;
template class IntLoop<ShadowPSet>;