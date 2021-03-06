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

#pragma once

#include "base.h"

#if DOTENSORS
string ModeArrayVarName(const DimVec &vec);
string IndexPairVarName(Dim dim1, Dim dim2);
string ModeArrayPairVarName(const DimVec &arr1, const DimVec &arr2);
string TensorDistVarName(const DistType &type);
string IndexArrayVarName(const string &indices);
string PermutationVarName(const DimVec &perm);
string DistEntryVecVarName(const DistEntryVec &vec);
#elif DOLLDLA
string LLDLAPartVarName(const string &var, unsigned int part);
string LLDLATransVarName(const string &var, Trans trans);
#endif



enum VarType {
#if DOTENSORS
  TensorVarType,
  ModeArrayVarType,
  IndexPairType,
  ModeArrayPairVarType,
  TensorDistVarType,
  IndexArrayType,
  PermutationVarType,
  DistEntryVecVarType,
#elif DOLLDLA
  VarPartType,
  VarTransType,
#endif
  DirectVarDeclType,
  InvalidType
};

class Var
{
 protected:
  string m_part;
  string m_varDecl;
  Type m_dataType;
  string m_compStr;

 public:
  VarType m_type;
  union {
    Name *m_name;

#if DOTENSORS
    DimVec *m_vec;
    std::pair<DimVec, DimVec> *m_arrPair;
    std::pair<Dim,Dim> *m_pair;
    string *m_indices;
    DistEntryVec *m_entryVec;
#endif
#if DODM
    DistType *m_distType;
#endif
#if DOLLDLA

    string *m_transVar;
#endif
  };

 public:
 Var() : m_type(InvalidType) {}
#if DOTENSORS
  Var(const Name &name);
  Var(VarType type, const DimVec &vec);
  Var(const DimVec &vec1, const DimVec &vec2);
  Var(Dim dim1, Dim dim2);
  Var(const DistEntryVec &vec);
#endif

#if !DOLLDLA
  Var(const Var &var);
  Var(VarType type, const string &str);
#endif // !DOLLDLA

#if DODM
  Var(const DistType &type);
#endif
#if DOLLDLA
  Var(const string &varName, unsigned int partNum, Type dataType);
  Var(const string &varName, Trans trans, Type dataType);
  Var(const Var &var, Type dataType);
  Var(VarType type, const string &str, Type dataType);
#endif
  ~Var();
  Var& operator=(const Var &rhs);
  string CompStr() const {return m_compStr;}
  void PrintDecl(IndStream &out) const;
  string GetVarName() const;
};

struct VarCompare {
  bool operator() (const Var& lhs, const Var& rhs) const{
    return lhs.CompStr() < rhs.CompStr();
  }
};

typedef set<Var,VarCompare> VarSet; // inefficient compare, so only do with small sizes
typedef VarSet::iterator VarSetIter;
typedef VarSet::const_iterator VarSetConstIter;

