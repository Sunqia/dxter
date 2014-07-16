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



#pragma once

#include "loop.h"
#include "loopTunnel.h"
#include "combineBase.h"

class CombineSingleIter : public CombineBase
{
 public:
#if TWOD
  CombineSingleIter();
  CombineSingleIter(PartDir dir, PossTunType type);
#else
  CombineSingleIter();
  CombineSingleIter(Dim partDim, PossTunType type);
#endif
  virtual void PrintCode(IndStream &out);
  static Node* BlankInst() { return new CombineSingleIter(LASTPARTDIR,LASTTUNNEL);}
  virtual Node* GetNewInst() {return BlankInst(); }
  virtual NodeType GetType() const;
  virtual void Prop();
  virtual PossTunnel* GetSetTunnel();
  virtual unsigned int NumOutputs() const {return 1;}
  virtual ClassType GetNodeClass() const {return GetClass();}
  static ClassType GetClass() {return "combine";}
  virtual const DataTypeInfo& DataType(unsigned int num) const;
#if TWOD
  virtual const Sizes* GetM(unsigned int num) const;
  virtual const Sizes* GetN(unsigned int num) const;
#if DODM
  virtual const Sizes* LocalM(unsigned int num) const;
  virtual const Sizes* LocalN(unsigned int num) const;
#endif
#else
  virtual const Dim NumDims(unsigned int num) const;
  virtual const Sizes* Len(unsigned int num, Dim dim) const;
  virtual const Sizes* LocalLen(unsigned int num, Dim dim) const;
#endif
  virtual Name GetName(unsigned int num) const;
  virtual bool IsCombine() const {return true;}
};