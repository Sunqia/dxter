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



#include "base.h"
#include "transform.h"
#include "basePSet.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#include "loopSupport.h"

extern unsigned int M_phase;

BasePSet::BasePSet()
  : m_ownerPoss(NULL), m_hasProped(false), m_isTopLevel(false)
{
}

Node* BasePSet::InTun(unsigned int num) const
{
  if (num >= m_inTuns.size()) {
    cout << num << " >= " << m_inTuns.size() << endl;
    throw;
  }
  return m_inTuns[num];
}

Node* BasePSet::OutTun(unsigned int num) const
{
  if (num >= m_outTuns.size())
    throw;
  return m_outTuns[num];
}

void BasePSet::ClearBeforeProp()
{
  m_hasProped = false;
  for (unsigned int i = 0; i < m_inTuns.size(); ++i)
    InTun(i)->ClearBeforeProp();
  for (unsigned int i = 0; i < m_outTuns.size(); ++i)
    OutTun(i)->ClearBeforeProp();
}


void BasePSet::Duplicate(const BasePSet *orig, NodeMap &map, bool possMerging, bool useShadows)
{
  m_isTopLevel = orig->m_isTopLevel;
  NodeVecConstIter iter  = orig->m_inTuns.begin();
  for (; iter != orig->m_inTuns.end(); ++iter) {
    Tunnel *tun = (Tunnel*)(map[*iter]);
    //expect set tunnel for this set to be duplicated
    // as part of the owning poss's duplication
    if (!tun)
      throw;
    m_inTuns.push_back(tun);
    tun->m_pset = this;
    if (useShadows) {
      while (!tun->m_children.empty()) {
	delete tun->m_children[0];
	tun->m_children.erase(tun->m_children.begin());
      }
    }
  }
  iter  = orig->m_outTuns.begin();
  for (; iter != orig->m_outTuns.end(); ++iter) {
    Tunnel *tun = (Tunnel*)(map[*iter]);
    if (!tun)
      throw;
    m_outTuns.push_back(tun);
    tun->m_pset = this;
    if (useShadows) {
      while (!tun->m_inputs.empty()) {
	delete tun->m_inputs[0];
	tun->m_inputs.erase(tun->m_inputs.begin());
      }
    }

  }
}

bool FoundPossUp(Node *node, const BasePSet *set, NodeVec &queue)
{
  NodeVecIter checkIter = queue.begin();
  for(; checkIter != queue.end(); ++checkIter) {
    if (*checkIter == node) {
      cout << "recursion on node " << node << " " << node->GetNodeClass() << endl;
      throw;
    }
  }
  queue.push_back(node);
  if (node->IsTunnel(POSSTUNOUT) || node->IsTunnel(POSSTUNIN)) {
    queue.pop_back();
    return false;
  }
  else if (node->IsTunnel(SETTUNOUT)) {
    const Tunnel *tunOut = (Tunnel*)node;
    if (tunOut->m_pset == set) {
      queue.pop_back();
      return true;
    }
    else {
      const BasePSet *foundSet = tunOut->m_pset;
      NodeVecConstIter iter = foundSet->m_inTuns.begin();
      for(; iter != foundSet->m_inTuns.end(); ++iter) {
        if (FoundPossUp(*iter, set, queue)) {
          queue.pop_back();
          return true;
        }
      }
    }
  }
  else {
    NodeConnVecConstIter iter = node->m_inputs.begin();
    for (; iter != node->m_inputs.end(); ++iter) {
      if (FoundPossUp((*iter)->m_n, set, queue)) {
        queue.pop_back();
        return true;
      }
    }
  }
  queue.pop_back();
  return false;
}

bool NothingBetween(const BasePSet *left, const BasePSet *right)
{
  NodeVecConstIter iter = right->m_inTuns.begin();
  for(; iter != right->m_inTuns.end(); ++iter) {
    Node *input = *iter;
    NodeConnVecConstIter iter2 = input->m_inputs.begin();
    for (; iter2 != input->m_inputs.end(); ++iter2) {
      if ((*iter2)->m_n->IsTunnel(SETTUNOUT)) {
        Tunnel *tunOut = (Tunnel*)((*iter2)->m_n);
        if (tunOut->m_pset != left) {
          NodeVec queue;
          if (FoundPossUp(tunOut,left,queue))
            return false;
          if (!queue.empty()) {
            cout << "queue not empty!\n";
            throw;
          }
        }
      }
      else {
        NodeVec queue;
        if (FoundPossUp((*iter2)->m_n,left, queue))
          return false;
        if (!queue.empty()) {
          cout << "queue not empty!\n";
          throw;
        }
      }
    }
  }
  return true;
}

bool ShouldMerge(const BasePSet *set1, const BasePSet *set2)
{
  unsigned int i, j, k;
  for(i = 0; i < set1->m_inTuns.size(); ++i) {
    const Node *in = set1->m_inTuns[i];
    for(j = 0; j < in->m_inputs.size(); ++j) {
      const Node *inInput = in->Input(j);
      if (inInput->IsTunnel()) {
        if (((Tunnel*)inInput)->m_pset == set2)
          return true;
      }
      for(k = 0; k < inInput->m_children.size(); ++k) {
        const Node *child = inInput->Child(k);
        if (child->IsTunnel()) {
          if (((Tunnel*)child)->m_pset == set2)
            return true;
        }
      }
    }
  }
  for(i = 0; i < set1->m_outTuns.size(); ++i) {
    const Node *out = set1->m_outTuns[i];
    for(j = 0; j < out->m_children.size(); ++j) {
      const Node *child = out->Child(j);
      if (child->IsTunnel()) {
        if (((Tunnel*)child)->m_pset == set2)
          return true;
      }
    }
  }
  return false;
}

bool BasePSet::CanMerge(BasePSet *pset) const
{
  bool nothingBetween = NothingBetween(this, pset) && NothingBetween(pset, this);
  if (!nothingBetween)
    return false;
  return ShouldMerge(this, pset);
}

void BasePSet::RemoveInTun(Node *tun)
{
  NodeVecIter iter = m_inTuns.begin();
  for(; iter != m_inTuns.end(); ++iter) {
    if (*iter == tun) {
      m_inTuns.erase(iter);
      return;
    }
  }
  throw;
}

void BasePSet::RemoveOutTun(Node *tun)
{
  NodeVecIter iter = m_outTuns.begin();
  for(; iter != m_outTuns.end(); ++iter) {
    if (*iter == tun) {
      m_outTuns.erase(iter);
      return;
    }
  }
  throw;
}


void BasePSet::FormSetAround()
{
  Poss *owner = m_ownerPoss;
  RealPSet *newSet = new RealPSet;
  Poss *newPoss = new Poss;
  
  newPoss->m_pset = newSet;
  newSet->m_posses.insert(PossMMapPair(newPoss->GetHash(),newPoss));
  
  newSet->m_ownerPoss = owner;
  owner->m_sets.push_back(newSet);
  
  
  
  NodeVecIter iter = m_inTuns.begin();
  for(; iter != m_inTuns.end(); ++iter) {
    Tunnel *tun = (Tunnel*)(*iter);
    Tunnel *newSetTun = new Tunnel(SETTUNIN);
    Tunnel *newTun = new Tunnel (POSSTUNIN);
    newPoss->AddNode(newTun);
    newPoss->m_inTuns.push_back(newTun);
    newTun->AddInput(newSetTun,0);
    if (tun->m_inputs.size() != 1) {
      throw;
    }
    NodeConn *in = tun->m_inputs[0];
    newSetTun->AddInput(in->m_n, in->m_num);
    tun->ChangeInput2Way(in->m_n, in->m_num, newTun, 0);
    newSet->m_inTuns.push_back(newSetTun);
    newSetTun->m_pset = newSet;
    owner->AddNode(newSetTun);
    owner->RemoveFromGraphNodes(tun);
    tun->m_poss=NULL;
    newPoss->AddNode(tun);
  }
  
  
  iter = m_outTuns.begin();
  for(; iter != m_outTuns.end(); ++iter) {
    Tunnel *tun = (Tunnel*)(*iter);
    Tunnel *newSetTun = new Tunnel(SETTUNOUT);
    Tunnel *newTun = new Tunnel (POSSTUNOUT);
    newPoss->m_outTuns.push_back(newTun);
    newPoss->AddNode(newTun);
    newSetTun->AddInput(newTun,0);
    tun->RedirectAllChildren(newSetTun);
    newTun->AddInput(tun,0);
    
    newSet->m_outTuns.push_back(newSetTun);
    newSetTun->m_pset = newSet;
    owner->AddNode(newSetTun);
    owner->RemoveFromGraphNodes(tun);
    tun->m_poss=NULL;
    newPoss->AddNode(tun);
  }
  
  bool found = false;
  PSetVecIter iter2 = owner->m_sets.begin();
  for(; !found && iter2 != owner->m_sets.end(); ++iter2) {
    if (*iter2 == this) {
      owner->m_sets.erase(iter2);
      found = true;
    }
  }
  if (!found)
    throw;
  newPoss->m_sets.push_back(this);
  m_ownerPoss = newPoss;
  newSet->m_functionality = newPoss->GetFunctionalityString();
  if (newSet->IsLoop()) {
    LoopInterface *loop = dynamic_cast<LoopInterface*>(newSet);
    newSet->m_functionality += (char)(loop->GetBSSize());
  }
}

void BasePSet::Flatten(ofstream &out) const
{
  WRITE(START);
  WRITE(m_isTopLevel);
  FlattenCore(out);
  GraphNum size;
  if (m_isTopLevel) {
    FullyFlatten(m_inTuns, out);
    FullyFlatten(m_outTuns, out);
  }
  else {
    size = m_inTuns.size();
    WRITE(size);
    NodeVecConstIter iter = m_inTuns.begin();
    for(; iter != m_inTuns.end(); ++iter)
      WRITE(*iter);
    WRITE(END);
    size = m_outTuns.size();
    WRITE(size);
    iter = m_outTuns.begin();
    for(; iter != m_outTuns.end(); ++iter) {
      WRITE(*iter);
    }
  }
  WRITE(m_ownerPoss);

  WRITE(END);
  WRITE(END);

}

void BasePSet::Unflatten(ifstream &in, SaveInfo &info)
{
  char tmp;
  READ(tmp);
  if (tmp != START)
    throw;
  READ(m_isTopLevel);
  UnflattenCore(in,info);
  GraphNum size;
  if (m_isTopLevel) {
    FullyUnflatten(m_inTuns, in, info);
    FullyUnflatten(m_outTuns, in, info);
  }
  else {
    READ(size);
    for(GraphNum i = 0; i < size; ++i) {
      Node *tun;
      READ(tun);
      Swap(&tun,info.nodeMap);
      m_inTuns.push_back(tun);
    }
    READ(tmp);
    if (tmp != END)
      throw;
    READ(size);
    for(GraphNum i = 0; i < size; ++i) {
      Node *tun;
      READ(tun);
      Swap(&tun,info.nodeMap);
      m_outTuns.push_back(tun);
    }
  }
  READ(m_ownerPoss);

  READ(tmp);
  if (tmp != END)
    throw;
  READ(tmp);
  if (tmp != END)
    throw;
  if (!m_isTopLevel)
    Swap(&m_ownerPoss, info.possMap);
  if (m_isTopLevel) {
    NodeVecIter iter2 = m_inTuns.begin();
    for(; iter2 != m_inTuns.end(); ++iter2)
      (*iter2)->PatchAfterDuplicate(*(info.nodeMap));
    iter2 = m_outTuns.begin();
    for(; iter2 != m_outTuns.end(); ++iter2)
      (*iter2)->PatchAfterDuplicate(*(info.nodeMap));
  }
}

bool BasePSet::CanPrint() const
{
  NodeVecConstIter iter = m_inTuns.begin();
  for(; iter != m_inTuns.end(); ++iter) {
    const Node *in = *iter;
    if (!in->CanPrintCode())
      return false;
  }
  return true;
}


#if DOBLIS
Comm PSet::ParallelismWithinCurrentPosses() const
{
  Comm comm = CORECOMM;
  if (IsLoop()) {
    const Loop *loop = (Loop*)this;
    if (loop->m_comm != CORECOMM) {
      return loop->m_comm;
    }
  }
  const Poss *currPoss = GetCurrPoss();
  PSetVecConstIter iter = currPoss->m_sets.begin();
  for(; iter != currPoss->m_sets.end(); ++iter) {
    const PSet *set = *iter;
    comm = MaxComm(comm,set->ParallelismWithinCurrentPosses());
  }
  NodeVecConstIter iter2 = currPoss->m_possNodes.begin();
  for(; iter2 != currPoss->m_possNodes.end(); ++iter2) {
    const Node *node = *iter2;
    if (node->IsParallel())
      comm = MaxComm(comm, node->ParallelComm());
  }
  return comm;
}
#endif //DOBLIS

