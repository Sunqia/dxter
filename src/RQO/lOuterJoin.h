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

#include "layers.h"
#include "rqoJoin.h"
#include "rqoBasis.h"
#include "transform.h"

#if DORQO



class LeftOuterJoin : public Join
{
 protected:
  DataTypeInfo m_dataTypeInfo;
  

 public:
  LeftOuterJoin();
  LeftOuterJoin(string sortBy, vector<string> in0Fields, vector<string> in1Fields);
  static Node* BlankInst() { return  new LeftOuterJoin; }
  virtual Node* GetNewInst() { return BlankInst(); }
  virtual void Duplicate(const Node *orig, bool shallow, bool possMerging);
  virtual const DataTypeInfo& DataType(ConnNum num) const;
  virtual void Prop();
  virtual void PrintCode(IndStream &out);
  virtual Cost GetCost() {return Input(0)->Outputs() * Input(1)->Outputs();}
  virtual ClassType GetNodeClass() const {return GetClass();}
  static ClassType GetClass() {return "leftouterjoin";}
  virtual void ClearDataTypeCache();
  virtual void BuildDataTypeCache();
  virtual bool Overwrites(const Node *input, ConnNum num) const {return false;}
  virtual Join* CreateCopyOfJoin() const;
  virtual int Outputs() {return (Input(0)->Outputs() > Input(1)->Outputs()) ? Input(0)-> Outputs() : Input(1)->Outputs();}

};

#endif