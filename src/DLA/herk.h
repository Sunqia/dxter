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
#include "elemRedist.h"
#include "lowerLayer.h"

RealLoop* HerkLoopVar1(Node *Ain, ConnNum Anum, 
		   Node *Cin, ConnNum Cnum,
		   Tri tri,
		   Trans trans,
		   Coef alpha, Coef beta, Type type,
		   Layer layer);

RealLoop* HerkLoopVar2(Node *Ain, ConnNum Anum, 
		   Node *Cin, ConnNum Cnum,
		   Tri tri,
		   Trans trans,
		   Coef alpha, Coef beta, Type type,
		   Layer layer);

RealLoop* HerkLoopVar5(Node *Ain, ConnNum Anum, 
		   Node *Cin, ConnNum Cnum,
		   Tri tri,
		   Trans trans,
		   Coef alpha, Coef beta, Type type,
		   Layer layer);

RealLoop* TriRKLoopVar5(Node *Ain, ConnNum Anum, 
		    Node *Bin, ConnNum Bnum, 
		    Node *Cin, ConnNum Cnum,
		    Tri tri,
		    Coef alpha, Coef beta, Type type,
		    Layer layer);

RealLoop* TriRKLoopVar7(Node *Ain, ConnNum Anum, 
		    Node *Bin, ConnNum Bnum, 
		    Node *Cin, ConnNum Cnum,
		    Tri tri,
		    Coef alpha, Coef beta, Type type,
		    Layer layer);

#if DOBLIS
RealLoop* BLISHerkLoop(Node *Ain, ConnNum Anum, 
		   Node *Bin, ConnNum Bnum,
		   Node *Cin, ConnNum Cnum,
		   Tri tri,
		   Coef alpha, Type type,
		   Layer layer);
#endif

class HerkProps
{
 public:
  Trans m_transA, m_transB;
  Tri m_tri;
  Coef m_alpha;
  Coef m_beta;
  Type m_type;
  HerkProps(Tri tri, Trans transA, Trans transB, Coef alpha, Coef beta, Type type);
  virtual ~HerkProps() {}
  void Duplicate(const HerkProps *orig);
  virtual void FlattenCore(ofstream &out) const;
  virtual void UnflattenCore(ifstream &in, SaveInfo &info);
};

class Herk : public DLAOp<2,1>, public HerkProps
{
 public:
  Herk(Layer layer, Tri tri, Trans trans, 
       Coef alpha, Coef beta, Type type);
  virtual NodeType GetType() const;
  static Node* BlankInst() { return new Herk(ABSLAYER, LOWER, NORMAL, COEFONE, COEFONE, REAL); }
  virtual Node* GetNewInst() { return BlankInst(); }
  virtual void Duplicate(const Node *orig, bool shallow, bool possMerging);
  virtual void PrintCode(IndStream &out);
  virtual ClassType GetNodeClass() const {return GetClass();}
  static ClassType GetClass() {return "Herk";}
  virtual void Prop();
  virtual void FlattenCore(ofstream &out) const;
  virtual void UnflattenCore(ifstream &in, SaveInfo &info);
  virtual Phase MaxPhase() const;
  virtual bool ShouldCullDP() const;
};

class HerkLoopExp : public SingleTrans
{
 public:  
  unsigned int m_var;
  Layer m_fromLayer, m_toLayer;
 HerkLoopExp(Layer fromLayer, Layer toLayer, unsigned int variant)
   : m_var(variant), m_fromLayer(fromLayer), m_toLayer(toLayer) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
};

class TriRKLoopExp : public SingleTrans
{
 public:  
  unsigned int m_var;
  Layer m_fromLayer, m_toLayer;
 TriRKLoopExp(Layer fromLayer, Layer toLayer, unsigned int variant)
   : m_var(variant), m_fromLayer(fromLayer), m_toLayer(toLayer) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
};



//This is a triangular rank-k update used in Elemental
class TriRK : public DLAOp<3,1>, public HerkProps
{
 public:
#if DOBLIS
  Comm m_comm;
#endif
  TriRK(Layer layer, Tri tri, Trans transA, Trans transB, Coef alpha, Coef beta, Type type);
  virtual Phase MaxPhase() const;
  virtual NodeType GetType() const;
  static Node* BlankInst() { return new TriRK(ABSLAYER, LOWER, NORMAL, NORMAL, COEFONE, COEFONE, REAL); }
  virtual Node* GetNewInst() { return BlankInst(); }
#if DOELEM
  virtual bool CanTransposeInputs() const;
#endif
  virtual void Prop();
  virtual void PrintCode(IndStream &out);
  virtual ClassType GetNodeClass() const {return GetClass();}
  static ClassType GetClass() {return "TriRK";}
  virtual void Duplicate(const Node *orig, bool shallow, bool possMerging);
  virtual void FlattenCore(ofstream &out) const;
  virtual void UnflattenCore(ifstream &in, SaveInfo &info);
#if DOBLIS
  virtual bool IsBLISParallelizable() const;
  virtual void Parallelize(Comm comm);
  virtual bool IsParallel() const;
  virtual bool RemoveParallelization();
  virtual Comm ParallelComm() const {return m_comm;}
#endif
};

#if DOELEM
class TriRKTrans : public TransTransformation
{
 public:
  TriRKTrans(ConnNum argNum, Trans trans) : TransTransformation(argNum, trans) {}
  virtual string GetTransType() const;
  virtual bool CanApply(const Node *node) const;
};
#endif

class DistHerkToLocalTriRK : public SingleTrans
{
 public:
  virtual string GetType() const {return "Distributed Herk to Local TriRK";}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
};

#if DOBLIS
class BLISTriRKLoopExp : public SingleTrans
{
 public:
  Layer m_fromLayer, m_toLayer;
  BLISTriRKLoopExp(Layer from, Layer to)
    : m_fromLayer(from), m_toLayer(to) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
  virtual Cost RHSCostEstimate(const Node *node) const {throw;}
};
#endif

class TriRKLowerLayer : public LowerLayer
{
 public:
 TriRKLowerLayer(Layer fromLayer, Layer toLayer, DimName dim, Size bs)
   : LowerLayer(fromLayer, toLayer, dim, bs) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
};

#if DOBLIS
class HerkToTriRK : public SingleTrans
{
 public:
  Layer m_layer;
 HerkToTriRK(Layer layer) :m_layer(layer) {}
  virtual string GetType() const {return "Herk to TriRK";}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
};
#endif
#endif
