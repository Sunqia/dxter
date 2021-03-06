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

#include "bool.h"

#if DOBOOL

DataTypeInfo Or::m_info;

string Or::GetUniqueName()
{
  static int num = 0;
  return (string)"or" + std::to_string(num++);
}

Or::Or()
:m_name(GetUniqueName())
{

}

void Or::Duplicate(const Node *orig, bool shallow, bool possMerging)
{
  Node::Duplicate(orig, shallow, possMerging);
  const Or *orOrig = (Or*)orig;
  m_name = orOrig->m_name;
}

Name Or::GetName(ConnNum num) const
{
  return m_name;
}

void Or::Prop()
{
  if (m_inputs.size() != 2)
    throw;
}

void Or::PrintCode(IndStream &out)
{
  out.Indent();
  *out << m_name.str() << " = Or( " << GetInputNameStr(0)
       << " , " << GetInputNameStr(1) << " );\n";
}



bool OrTrue::CanApply(const Node *node) const
{
  if (node->GetNodeClass() != Or::GetClass())
    throw;
  const Or *orNode = (Or*)node;
  if (orNode->Input(0)->GetNodeClass() == True::GetClass())
    return true;
  else if (orNode->Input(1)->GetNodeClass() == True::GetClass())
    return true;
  else
    return false;
}

void OrTrue::Apply(Node *node) const
{
  Or *orNode = (Or*)node;
  if (orNode->Input(0)->GetNodeClass() == True::GetClass())
    orNode->RedirectChildren(orNode->Input(0),orNode->InputConnNum(0));
  else
    orNode->RedirectChildren(orNode->Input(1),orNode->InputConnNum(1));
  
  orNode->m_poss->DeleteChildAndCleanUp(orNode);
}


bool OrFalse::CanApply(const Node *node) const
{
  if (node->GetNodeClass() != Or::GetClass())
    throw;
  const Or *orNode = (Or*)node;
  if (orNode->Input(0)->GetNodeClass() == False::GetClass())
    return true;
  else if (orNode->Input(1)->GetNodeClass() == False::GetClass())
    return true;
  else
    return false;
}

void OrFalse::Apply(Node *node) const
{
  Or *orNode = (Or*)node;
  if (orNode->Input(0)->GetNodeClass() == False::GetClass())
    orNode->RedirectChildren(orNode->Input(1),orNode->InputConnNum(1));
  else
    orNode->RedirectChildren(orNode->Input(0),orNode->InputConnNum(0));
  
  orNode->m_poss->DeleteChildAndCleanUp(orNode);
}

#endif // DOBOOL

