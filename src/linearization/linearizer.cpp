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

#include "linearizer.h"
#include "poss.h"
#include "nodeLinElem.h"
#include "setLinElem.h"
#include "basePSet.h"
#include "node.h"
#include "helperNodes.h"

bool FoundInVec(const LinElem *elem, const LinElemVec &vec)
{
  for(auto t : vec)
    if (t == elem)
      return true;
  return false;
}

Linearizer::Linearizer(const Poss *poss)
{
  PtrToLinElemMap map;
  for(auto node : poss->m_possNodes) {
    FindOrAdd(node, map);
  }
  for(auto set : poss->m_sets) {
    FindOrAdd(set, map);
  }
}

Linearizer::~Linearizer()
{
  for(auto elem : m_elems)
    delete elem;
}

LinElem* Linearizer::FindOrAdd(Node *node, PtrToLinElemMap &map)
{
  PtrToLinElemMapIter find = map.find(node);
  if (find != map.end())
    return find->second;
  
  if (node->IsTunnel()) {
    const Tunnel *tun = (Tunnel*)node;
    switch (tun->m_tunType)
    {
      case (POSSTUNIN):
      case (POSSTUNOUT):
        return NULL;
      case (SETTUNIN):
      case (SETTUNOUT):
        return FindOrAdd(tun->m_pset, map);
      default:
        throw;
    }
  }
  
  
  NodeLinElem *elem = new NodeLinElem(node);
  map[node] = elem;
  m_elems.push_back(elem);
  
  if (node->GetNodeClass() == OutputNode::GetClass()) {
    m_alwaysLive.insert(node->GetInputNameStr(0));
  }
  
  for(auto inputConn : node->m_inputs) {
    LinElem *inElem = FindOrAdd(inputConn->m_n, map);
    if (inElem)
      elem->AddInputIfUnique(inElem);
  }
  
  if (node->NumOutputs() > 1)
    throw;
  
  LinElem *overwriter = NULL;
  LinElemSet others;
  for(auto childConn : node->m_children) {
    LinElem *outElem = FindOrAdd(childConn->m_n, map);
    if (outElem) {
      elem->AddChildIfUnique(outElem);
      if (childConn->m_n->Overwrites(node, 0)) {
        if (overwriter) {
          if (overwriter != outElem) {
            cout << "two children of set overwrite output\n";
            throw;
          }
        }
        else {
          overwriter = outElem;
        }
      }
      else if (childConn->m_n->IsDataDependencyOfInput()) {
        others.insert(outElem);
      }
    }
  }
  
  if (overwriter) {
    LinElemSetIter iter = others.find(overwriter);
    if (iter != others.end()) {
      others.erase(iter);
    }
    overwriter->m_preds.insert(overwriter->m_preds.begin(),
                               others.begin(),
                               others.end());
    for(auto other : others) {
      if (other->m_succ) {
        cout << "already has succesor\n";
        throw;
      }
      else
        other->m_succ = overwriter;
    }
  }
  
  return elem;
}

LinElem* Linearizer::FindOrAdd(BasePSet *set, PtrToLinElemMap &map)
{
  PtrToLinElemMapIter find = map.find(set);
  if (find != map.end())
    return find->second;
  
  SetLinElem *elem = new SetLinElem(set);
  map[set] = elem;
  m_elems.push_back(elem);
  
  for(auto inTun : set->m_inTuns) {
    for(auto inputConn : inTun->m_inputs) {
      LinElem *inElem = FindOrAdd(inputConn->m_n, map);
      if (inElem)
        elem->AddInputIfUnique(inElem);
    }
  }
  
  for(auto outTun : set->m_outTuns) {
    LinElem *overwriter = NULL;
    LinElemSet others;
    for(auto childConn : outTun->m_children) {
      LinElem *outElem = FindOrAdd(childConn->m_n, map);
      if (outElem) {
        elem->AddChildIfUnique(outElem);
        if (childConn->m_n->Overwrites(outTun, childConn->m_num)) {
          if (overwriter) {
            if (overwriter != outElem) {
              cout << "two children of set overwrite output\n";
              throw;
            }
          }
          else {
            overwriter = outElem;
          }
        }
        else {
          others.insert(outElem);
        }
      }
    }
    if (overwriter) {
      LinElemSetIter iter = others.find(overwriter);
      if (iter != others.end()) {
        others.erase(iter);
      }
      overwriter->m_preds.insert(overwriter->m_preds.begin(),
                                 others.begin(),
                                 others.end());
      for(auto other : others) {
        if (other->m_succ) {
          cout << "has succs\n";
          throw;
        }
        else
          other->m_succ = overwriter;
      }
    }
  }
  
  return elem;
}

void Linearizer::FindOptimalLinearization(const StrSet &stillLive)
{
  ClearCurrLinearization();
  
  m_lin.m_cost = -1;
  
  LinElemVec readyToAdd;
  
  Linearization curr;
  
  for(auto elem : m_elems) {
    if (elem->IsNode()) {
      NodeLinElem *nodeElem = (NodeLinElem*)elem;
      if (nodeElem->m_node->GetNodeClass() == InputNode::GetClass()) {
        curr.m_order.push_back(elem);
        elem->SetAdded();
      }
    }
  }
  
  //output nodes don't print any code and for tensors are often just connected to inputs, so
  // put them in the order now to reduce computation complexity of finding all topological sorts
  for(auto elem : m_elems) {
    if (elem->IsNode()) {
      NodeLinElem *nodeElem = (NodeLinElem*)elem;
      if (nodeElem->m_node->GetNodeClass() == OutputNode::GetClass()) {
        if (nodeElem->CanAddToLinearOrder())
        {
          curr.m_order.push_back(elem);
          elem->SetAdded();
        }
      }
    }
  }
  
#if DOTENSORS
  cout << "\\\\********Scheduling the following\n";
  for(auto elem : m_elems) {
    if (!elem->HasAdded()) {
      if (elem->IsNode()) {
        cout << "\\\\" << ((NodeLinElem*)elem)->m_node->GetNodeClass() << endl;
      }
      
      else if (elem->IsSet())
        cout << "\\\\set " << ((SetLinElem*)elem)->m_set->GetFunctionalityString() << endl;
    }
  }
#endif // DOTENSORS
  
  for(auto elem : m_elems) {
    if (elem->CanAddToLinearOrder()) {
      bool skip = false;
      if (elem->m_children.size() == 1
          && elem->IsNode()
          && ((NodeLinElem*)elem)->m_node->GetNodeClass() == TempVarNode::GetClass())
      {
        LinElem *child = elem->m_children[0];
        for(auto in : child->m_inputs) {
          if (in != elem) {
            if (in->IsNode()
                && in->m_children.size() == 1
                && ((NodeLinElem*)in)->m_node->GetNodeClass() == TempVarNode::GetClass())
            {
              if (FoundInVec(in, readyToAdd)) {
                skip = true;
                break;
              }
            }
          }
        }
      }
      if (!skip)
        readyToAdd.push_back(elem);
    }
  }
  
  if (readyToAdd.empty())
    throw;
  
  //  cout << "\n\n\nfinding optimal with " << m_elems.size() << endl;
  //  cout.flush();
  
  RecursivelyFindOpt(curr, readyToAdd, m_lin, stillLive);
  
  for(auto elem : curr.m_order) {
    if (elem->IsNode()) {
      NodeLinElem *nodeElem = (NodeLinElem*)elem;
      if (nodeElem->m_node->GetNodeClass() != InputNode::GetClass() &&
          nodeElem->m_node->GetNodeClass() != OutputNode::GetClass())
        throw;
    }
    else
      throw;
  }
  
  if (m_lin.m_order.size() != m_elems.size()) {
    cout << m_lin.m_order.size() << endl;
    cout << m_elems.size() << endl;
    throw;
  }
  
  if (m_lin.GetCostNoRecursion(stillLive, m_alwaysLive) < 0) {
    m_lin.m_cost = -1;
    cout << m_lin.GetCostNoRecursion(stillLive, m_alwaysLive) << endl;
    throw;
  }
}

void Linearizer::RecursivelyFindOpt(Linearization &curr, const LinElemVec &readyToAdd, Linearization &opt, const StrSet &stillLive) const
{
  if (readyToAdd.empty()) {
    if (opt.m_cost < 0) {
      opt = curr;
      opt.GetCostNoRecursion(stillLive, m_alwaysLive);
    }
    else {
      if (opt.m_order.size() != curr.m_order.size())
        throw;
      if (opt.GetCostNoRecursion(stillLive, m_alwaysLive) > curr.GetCostNoRecursion(stillLive, m_alwaysLive))
        opt = curr;
    }
  }
  else {
    for(unsigned int i = 0; i < readyToAdd.size(); ++i) {
      LinElemVec newReadyToAdd = readyToAdd;
      newReadyToAdd.erase(newReadyToAdd.begin()+i);
      LinElem *currAdd = readyToAdd[i];
      
      AddAndRecurse(curr, newReadyToAdd, currAdd, opt, stillLive);
    }
  }
}

void Linearizer::AddAndRecurse(Linearization &curr, LinElemVec &readyToAdd, LinElem *currAdd, Linearization &opt, const StrSet &stillLive) const
{
  if (!currAdd->CanAddToLinearOrder())
    throw;
  curr.m_order.push_back(currAdd);
  currAdd->SetAdded();
  if (currAdd->m_children.size() > 1
      || (currAdd->m_succ && currAdd->m_succ->CanAddToLinearOrder()))
  {
    //Go through children and add those that should be added immediately
    //Keep track of TempVarNodes that may finally be able to add
    LinElemVec tempVarNodes;
    int printedImmediately = 0;
    for(auto child : currAdd->m_children) {
      if (child->CanAddToLinearOrder()) {
        bool done = false;
        if (child->IsNode()) {
          NodeLinElem *childNode = (NodeLinElem*)child;
          Node *node = childNode->m_node;
          if (node->GetNodeClass() == OutputNode::GetClass()) {
            curr.m_order.push_back(child);
            child->SetAdded();
            ++printedImmediately;
            done = true;
            if (child->m_succ)
              throw;
          }
        }
        if (!done)
          readyToAdd.push_back(child);
      }
      else {
        //If that was the last input to (e.g.) a set
        // other than temps, then now the temps can
        // add, followed by the set
        for (auto input : child->m_inputs) {
          if (!input->HasAdded() &&
              input != currAdd &&
              input->CanAddToLinearOrder())
          {
            if (input->IsNode() && ((NodeLinElem*)input)->m_node->GetNodeClass() == TempVarNode::GetClass()) {
              if (!FoundInVec(input,readyToAdd)) {
                readyToAdd.push_back(input);
                break;
              }
            }
          }
        }
      }
    }
    if (currAdd->m_succ && currAdd->m_succ->CanAddToLinearOrder() && !FoundInVec(currAdd->m_succ,readyToAdd)) {
      LinElem *succ = currAdd->m_succ;
      bool skip = false;
      if (succ->IsNode() && ((NodeLinElem*)succ)->m_node->GetNodeClass() == TempVarNode::GetClass()) {
	if (succ->m_children.size() == 1) {
	  for (auto input : succ->m_children[0]->m_inputs) {
	    if (input->IsNode() && ((NodeLinElem*)input)->m_node->GetNodeClass() == TempVarNode::GetClass()
		&& input->m_children.size() == 1 && FoundInVec(input, readyToAdd)) 
	      {
		skip = true;
		break;
	      }
	  }
	}
      }
      if (!skip)
	readyToAdd.push_back(currAdd->m_succ);
    }
    RecursivelyFindOpt(curr, readyToAdd, opt, stillLive);
    while (printedImmediately > 0) {
      LinElem *elem = curr.m_order.back();
      elem->ClearAdded();
      curr.m_order.pop_back();
      --printedImmediately;
    }
  }
  else if (currAdd->m_children.size() == 1) {
    LinElem *child = currAdd->m_children[0];
    if (currAdd->IsNode() && ((NodeLinElem*)currAdd)->m_node->GetNodeClass() == TempVarNode::GetClass()) {
      int countOfAdded = 0;
      //this is a tempvar node
      // if the child is a set, print all of its other input temp var nodes and it
      // if it's not a set, try to print it
      for (auto input : child->m_inputs) {
        if (!input->HasAdded()
            && input->IsNode()
            && ((NodeLinElem*)input)->m_node->GetNodeClass() == TempVarNode::GetClass())
        {
          if (!input->CanAddToLinearOrder())
            throw;
          for(unsigned int i = 0; i < readyToAdd.size(); ++i) {
            if(readyToAdd[i] == input) {
              readyToAdd.erase(readyToAdd.begin()+i);
              break;
            }
          }
          ++countOfAdded;
          curr.m_order.push_back(input);
          input->SetAdded();
        }
      }
      if (!child->CanAddToLinearOrder()) {
        cout << child->CanAddToLinearOrder() << endl;
        throw;
      }
      AddAndRecurse(curr, readyToAdd, child, opt, stillLive);
      for(; countOfAdded > 0; --countOfAdded) {
        LinElem *elem = curr.m_order.back();
        if (!elem->IsNode() || ((NodeLinElem*)elem)->m_node->GetNodeClass() != TempVarNode::GetClass())
          throw;
        curr.m_order.pop_back();
        elem->ClearAdded();
      }
    }
    else {
      if (child->CanAddToLinearOrder())
        AddAndRecurse(curr, readyToAdd, child, opt, stillLive);
      else {
        //If that was the last input to (e.g.) a set
        // other than temps, then now the temps can
        // add, followed by the set
        for (auto input : child->m_inputs) {
          if (!input->HasAdded() &&
              input->CanAddToLinearOrder() &&
              input->IsNode() &&
              input != currAdd
              && ((NodeLinElem*)input)->m_node->GetNodeClass() == TempVarNode::GetClass())
          {
            if (!FoundInVec(input, readyToAdd)) {
              readyToAdd.push_back(input);
            }
	    break;
          }
        }
        RecursivelyFindOpt(curr, readyToAdd, opt, stillLive);
      }
    }
  }
  else {
    //don't have children, so continue working with readyToAdd list
    RecursivelyFindOpt(curr, readyToAdd, opt, stillLive);
  }
  currAdd->ClearAdded();
  curr.m_order.pop_back();
}


void Linearizer::FindAnyLinearization()
{
  ClearCurrLinearization();
  
  LinElemVec readyToAdd;
  
  for(auto elem : m_elems) {
    if (elem->CanAddToLinearOrder()) {
      readyToAdd.push_back(elem);
    }
  }
  
  while (!readyToAdd.empty()) {
    LinElem *elem = readyToAdd.back();
    readyToAdd.pop_back();
    m_lin.m_order.push_back(elem);
    elem->SetAdded();
    if (elem->m_succ && elem->m_succ->CanAddToLinearOrder())
      readyToAdd.push_back(elem->m_succ);
    for(auto child : elem->m_children) {
      if (child->CanAddToLinearOrder())
        readyToAdd.push_back(child);
    }
  }
  
  if (m_lin.m_order.size() != m_elems.size()) {
    PrintConnections();
    throw;
  }
}

void Linearizer::ClearCurrLinearization()
{
  m_lin.Clear();
  for(auto elem : m_elems) {
    elem->ClearCache();
    elem->ClearAdded();
  }
}

bool Linearizer::HasCurrLinearization() const
{
  if (m_lin.m_cost < 0) {
    return false;
  }
  else {
    if (m_lin.m_order.size() != m_elems.size())
      throw;
    return true;
  }
}

void Linearizer::InsertVecClearing(const StrSet &stillLive)
{
  if (!HasCurrLinearization())
    throw;
  m_lin.InsertVecClearing(stillLive, m_alwaysLive);
}

void Linearizer::PrintConnections() const
{
  for(auto elem : m_elems) {
    cout << elem << ":\n";
    for(auto in : elem->m_inputs) {
      cout << "\tin:\t" << in << endl;
    }
    for(auto child : elem->m_children) {
      cout << "\tout:\t" << child << endl;
    }
  }
}
