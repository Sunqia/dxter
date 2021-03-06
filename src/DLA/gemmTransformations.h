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
#if (DOBLIS||DOELEM||DOLLDLA)

#include "gemm.h"
#include "DLAOp.h"
#include "transform.h"
#include "elemRedist.h"
#include "lowerLayer.h"
#include "realLoop.h"

RealLoop* GemmVar1Loop(Node *Ain, ConnNum Anum, 
		Node *Bin, ConnNum Bnum, 
		Node *Cin, ConnNum Cnum,
		   BSSize bs,
		Trans transA, Trans transB,
		Coef alpha, Coef beta, 
		Layer layer, Type type);

RealLoop* GemmVar3Loop(Node *Ain, ConnNum Anum, 
		    Node *Bin, ConnNum Bnum, 
		      Node *Cin, ConnNum Cnum,
		   BSSize bs,
		      Trans transA, Trans transB,
		   bool reverse,
		      Coef alpha, Coef beta, 
		   Layer layer, Type type);

RealLoop* GemmVar2Loop(Node *Ain, ConnNum Anum, 
		    Node *Bin, ConnNum Bnum, 
		      Node *Cin, ConnNum Cnum,
		   BSSize bs,
		      Trans transA, Trans transB,
		      Coef alpha, Coef beta, 
		Layer layer, Type type);

class GemmLoopExp : public SingleTrans
{
 public:
  Layer m_fromLayer, m_toLayer;
  int m_dim;
  BSSize m_bsSize;
  GemmLoopExp(Layer fromLayer, Layer toLayer, int dim);
  GemmLoopExp(Layer fromLayer, Layer toLayer, int dim, BSSize bsSize);

  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
};

#if DOELEM
class DistGemmToLocalGemmStatC : public SingleTrans
{
 public:
  virtual string GetType() const {return "Distributed Gemm to Local Stat C";}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
  virtual Cost RHSCostEstimate(const Node *node) const;
};

class DistGemmToContribLocalGemmStatANonTrans : public SingleTrans
{
 public:
  virtual string GetType() const {return "Distributed Gemm to Local Stat A NonTrans";}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
  virtual Cost RHSCostEstimate(const Node *node) const;
};

class DistGemmToContribLocalGemmStatBNonTrans : public SingleTrans
{
 public:
  bool m_trans;
 DistGemmToContribLocalGemmStatBNonTrans(bool trans) :m_trans(trans) {}
  virtual string GetType() const {return (string)"Distributed Gemm to Local Stat B NonTrans "+(m_trans?"trans":"nonTrans");}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
  virtual Cost RHSCostEstimate(const Node *node) const;
};


class DistGemmToContribLocalGemmStatBTrans : public SingleTrans
{
 public:
  virtual string GetType() const {return "Distributed Gemm to Local Stat B Trans";}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
  virtual Cost RHSCostEstimate(const Node *node) const;
};

class DistGemmToContribLocalGemmStatATrans : public SingleTrans
{
 public:
  virtual string GetType() const {return "Distributed Gemm to Local Stat A Trans";}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
  virtual Cost RHSCostEstimate(const Node *node) const;
};


class GemmTrans : public TransTransformation
{
 public:
  GemmTrans(ConnNum argNum, Trans trans) : TransTransformation(argNum,trans) {}
  virtual string GetTransType() const;
  virtual bool CanApply(const Node *node) const;
};


class GemmInputReordering : public SingleTrans
{
 public:
  int m_type;
  const SingleTrans *m_inverse;
  GemmInputReordering(int type) :m_type(type) {}
  virtual string GetType() const {return ((string)"Gemm Reordering ") + (char)(m_type+48);}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
};
#endif

#if DOBLIS
class BLISGemmLoopExp : public SingleTrans
{
 public:
  Layer m_fromLayer, m_toLayer;
 BLISGemmLoopExp(Layer fromLayer, Layer toLayer) 
   : m_fromLayer(fromLayer), m_toLayer(toLayer) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
};
#endif

class GemmLowerLayer : public LowerLayer
{
 public:
 GemmLowerLayer(Layer fromLayer, Layer toLayer, DimName dim, Size bs)
   : LowerLayer(fromLayer, toLayer, dim, bs) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
};

#if DOELEM||DOBLIS
class SplitGemm : public SingleTrans
{
 public:
  Layer m_layer;
 SplitGemm(Layer layer) : m_layer(layer) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
};
#endif

#endif
