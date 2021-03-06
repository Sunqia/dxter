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

#include "nodeLinElem.h"
#include "node.h"
#include "DLANode.h"
#include "helperNodes.h"
#include "rqoHelperNodes.h"
#include "contraction.h"
#include "tensorRedist.h"
#include "tensorSumScatter.h"

void NodeLinElem::Print(IndStream &out)
{
  m_node->Print(out);
}

StrVec NodeLinElem::PossiblyDyingVars() const
{
  StrVec list;

  if (m_node->GetNodeClass() == OutputNode::GetClass())
    return list;
  string out;
  switch (m_node->NumOutputs())
    {
    case (0):
      break;
    case (1):
      if (m_children.size() > 1)
	out = m_node->GetNameStr(0);
      break;
    default:
      throw;
    }

  for(auto conn : m_node->m_inputs) {
    string name = conn->m_n->GetNameStr(conn->m_num);
    if (name != out)
      list.push_back(name);
  }
  return list;
}

VarCostMap NodeLinElem::NewVarsAndCosts() const
{
  VarCostMap map;
  switch (m_node->NumOutputs())
    {
    case (0):
      return map;
    case (1): 
      {
	string out = m_node->GetNameStr(0);
	for(unsigned int i = 0; i < m_node->m_inputs.size(); ++i) {
	  if (out == m_node->GetInputNameStr(i)) {
	    return map;
	  }
	}
#if DODM
	map[m_node->GetNameStr(0)] = ((DLANode*)m_node)->MaxNumberOfLocalElements(0);
#elif TWOD
	map[m_node->GetNameStr(0)] = ((DLANode*)m_node)->MaxNumberOfElements(0);
#endif
	return map;
      }
    default:
      throw;
    }
}

StrSet NodeLinElem::NewVars() const
{
  StrSet set;
  switch (m_node->NumOutputs())
    {
    case (0):
      return set;
    case (1): 
      {
	string out = m_node->GetNameStr(0);
	for(unsigned int i = 0; i < m_node->m_inputs.size(); ++i) {
	  if (out == m_node->GetInputNameStr(i)) {
	    return set;
	  }
	}
	set.insert(m_node->GetNameStr(0));
	return set;
      }
    default:
      throw;
    }
}

bool NodeLinElem::UsesInputVar(const string &var) const
{
  for(auto conn : m_node->m_inputs) {
    string name = conn->m_n->GetNameStr(conn->m_num);
    if (name == var)
      return true;
  }
  return false;
}

bool NodeLinElem::CreatesNewVars() const
{
#if DOTENSORS
  return m_node->CreatesNewVars();
#else
  throw;
#endif  
}

Cost NodeLinElem::InternalCost() const
{
#if DOTENSORS
  if (CurrPhase < PACKOPTPHASE)
    return 0;
  if (m_node->GetNodeClass() == Contraction::GetClass())
    {
      const Contraction *cont = (Contraction*)m_node;
      if (cont->m_needsPacking) {
	Size ASize = ((DLANode*)(cont->Input(0)))->MaxNumberOfLocalElements(cont->InputConnNum(0));
	Size BSize = ((DLANode*)(cont->Input(1)))->MaxNumberOfLocalElements(cont->InputConnNum(1));
	Size CSize = ((DLANode*)(cont->Input(2)))->MaxNumberOfLocalElements(cont->InputConnNum(2));
	return ASize + BSize + CSize;
      }
      else
	return 0;
    }
  if (m_node->GetNodeClass() == RedistNode::GetClass())
    {
      const RedistNode *redist = (RedistNode*)m_node;
      Size inSize = ((DLANode*)(redist->Input(0)))->MaxNumberOfLocalElements(redist->InputConnNum(0));
      Size outSize = redist->MaxNumberOfLocalElements(0);
      return inSize + outSize;
    }
  if (m_node->GetNodeClass() == AllReduceNode::GetClass())
    {
      throw;      
    }
  if (m_node->GetNodeClass() == SumScatterUpdateNode::GetClass())
    {
      const SumScatterUpdateNode *redist = (SumScatterUpdateNode*)m_node;
      Size inSize = ((DLANode*)(redist->Input(0)))->MaxNumberOfLocalElements(redist->InputConnNum(0));
      Size outSize = redist->MaxNumberOfLocalElements(0);
      return inSize + outSize;
    }
#endif
  return 0;
}
