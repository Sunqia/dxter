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
#if DOBLIS||DOELEM

#include "transform.h"
#include "DLAOp.h"

RealLoop* TriInvAlgVar1Lower(Node *in, ConnNum num);
RealLoop* TriInvAlgVar1Upper(Node *in, ConnNum num);

RealLoop* TriInvAlgVar2Lower(Node *in, ConnNum num);
RealLoop* TriInvAlgVar2Upper(Node *in, ConnNum num);

RealLoop* TriInvAlgVar3Lower(Node *in, ConnNum num);
RealLoop* TriInvAlgVar3Upper(Node *in, ConnNum num);

RealLoop* TriInvAlgVar8Lower(Node *in, ConnNum num);

class TriInv : public DLAOp<1,1>
{
 public:
  Tri m_tri;
 TriInv(Layer layer, Tri tri) : m_tri(tri) {SetLayer(layer);}
  virtual void Duplicate(const Node *orig, bool shallow, bool possMerging);
  virtual void Prop();
  virtual void PrintCode(IndStream &out);
  virtual ClassType GetNodeClass() const {return GetClass();}
  static ClassType GetClass() {return "TriInv";}
  virtual void FlattenCore(ofstream &out) const;
  virtual void UnflattenCore(ifstream &in, SaveInfo &info);
  virtual NodeType GetType() const {return "TriInv " + LayerNumToStr(m_layer);}
  static Node* BlankInst() { return  new TriInv(ABSLAYER, LOWER); }
  virtual Node* GetNewInst() { return BlankInst(); }
  virtual Phase MaxPhase() const;
#if DOELEM
  virtual bool ShouldCullDP() const {return m_layer==ABSLAYER;}
  virtual bool DoNotCullDP() const {return m_layer==DMLAYER;}
  virtual bool CanTransposeInputs() const {return m_layer==SMLAYER;}
#endif
};

class TriInvLoopExp : public SingleTrans
{
 public:
  unsigned int m_var;
 TriInvLoopExp(unsigned int var) : m_var(var) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
};

class DistTriInvToLocalTriInv : public SingleTrans
{
 public:
  virtual string GetType() const {return "Distributed TriInv to Local TriInv";}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
};
#endif
