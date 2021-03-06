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



//These are basic classes from which to inherit standard DLA
// features and code

#pragma once

#include "transform.h"
#include "DLANode.h"

#if DODLA

template<ConnNum numIn, ConnNum numOut>
class DLAOp : public DLANode
{
 public:

#if TWOD
  virtual const SizeList* GetM(ConnNum num) const;
  virtual const SizeList* GetN(ConnNum num) const;
#if DODM
  virtual const SizeList* LocalM(ConnNum num) const;
  virtual const SizeList* LocalN(ConnNum num) const;
#endif
#elif DOTENSORS
  virtual const Dim NumDims(ConnNum num) const;
  virtual const SizeList* Len(ConnNum num, Dim dim) const;
  virtual const SizeList* LocalLen(ConnNum num, Dim dim) const;
#endif
  virtual const DataTypeInfo& DataType(ConnNum num) const;
  virtual Name GetName(ConnNum num) const;
  virtual void Prop();
  virtual ConnNum NumOutputs() const;
  virtual bool Overwrites(const Node *input, ConnNum num) const;
#if DOTENSORS
  virtual bool CreatesNewVars() const {return false;}
#endif
};

#endif //DODLA
