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

#include "DLANode.h"
#include "node.h"
#include "layers.h"

#if DOLLDLA

class Partition : public DLANode
{
  
 private:
  Dir m_partType;

  Size m_partSplitPoint;

  Sizes* m_startSizes;
  Sizes* m_endSizes;

  DataTypeInfo* m_startInfo;
  DataTypeInfo* m_endInfo;

  Name m_startName;
  Name m_endName;

  void SetHorizontalNames();
  void SetVerticalNames();

  void BuildHorizontalDataTypeCache();
  void BuildVerticalDataTypeCache();

  void BuildHorizontalDataTypeInfo();
  void BuildVerticalDataTypeInfo();

  void BuildHorizontalSizes();
  void BuildVerticalSizes();
  void BuildStartAndEndSizes(const Sizes* toSplit);

 public:
  Layer m_layer;
  
  Partition(Layer layer, Dir partType, Size partStart);

  virtual void PrintCode(IndStream &out);

  virtual ~Partition() {}
  inline void SetLayer(Layer layer) { m_layer = layer; }
  inline Layer GetLayer() const { return m_layer; }
  virtual bool IsReadOnly() const { return false; }
  virtual bool CanTrans() const { return false; }

  virtual NodeType GetType() const { return "Partition"; }
  static ClassType GetClass() {return "partitionNode";}
  virtual ClassType GetNodeClass() const { return GetClass(); }
  virtual Name GetName(ConnNum num) const;

  virtual void AddVariables(VarSet &set) const;
  virtual void BuildDataTypeCache();
  virtual void ClearDataTypeCache();

  virtual ConnNum NumOutputs() const;
  virtual const DataTypeInfo& DataType(ConnNum num) const;

  bool KeepsInputVarLive(Node* input, ConnNum numIn, ConnNum &numOut)
    const { return false; }
  virtual bool Overwrites(const Node* input, ConnNum num) const { return true; }

  virtual Node* GetNewInst() { return BlankInst(); }
  static Node* BlankInst() { return new Partition(ABSLAYER, VERTICAL, 0); }

  virtual void Duplicate(const Node* orig, bool shallow, bool possMerging);

  virtual void Prop();

  virtual const Sizes* GetM(ConnNum num) const;
  virtual const Sizes* GetN(ConnNum num) const;

};

#endif //DOLLDLA