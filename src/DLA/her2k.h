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

#if DOELEM||DOBLIS

#include "transform.h"
#include "elemRedist.h"
#include "DLAOp.h"
#include "lowerLayer.h"

RealLoop* Her2kLoopVar1(Node *Ain, ConnNum Anum, 
		    Node *Bin, ConnNum Bnum, 
		    Node *Cin, ConnNum Cnum,
		    Tri tri,
		    Trans trans,
		    Coef alpha, Coef beta, Type type,
		    Layer layer);


RealLoop* Her2kLoopVar2(Node *Ain, ConnNum Anum, 
		    Node *Bin, ConnNum Bnum, 
		    Node *Cin, ConnNum Cnum,
		    Tri tri,
		    Trans trans,
		    Coef alpha, Coef beta, Type type,
		    Layer layer);


RealLoop* Her2kLoopVar3(Node *Ain, ConnNum Anum, 
		    Node *Bin, ConnNum Bnum, 
		    Node *Cin, ConnNum Cnum,
		    Tri tri,
		    Trans trans,
		    Coef alpha, Coef beta, Type type,
		    Layer layer);

RealLoop* Her2kLoopVar4(Node *Ain, ConnNum Anum, 
		    Node *Bin, ConnNum Bnum, 
		    Node *Cin, ConnNum Cnum,
		    Tri tri,
		    Trans trans,
		    Coef alpha, Coef beta, Type type,
		    Layer layer);


RealLoop* Her2kLoopVar9(Node *Ain, ConnNum Anum, 
		    Node *Bin, ConnNum Bnum, 
		    Node *Cin, ConnNum Cnum,
		    Tri tri,
		    Trans trans,
		    Coef alpha, Coef beta, Type type,
		    Layer layer);

RealLoop* Tri2kLoopVar9(Node *Ain, ConnNum Anum,
                     Node *Bin, ConnNum Bnum,
                     Node *Cin, ConnNum Cnum,
                     Node *Din, ConnNum Dnum,
                     Node *Ein, ConnNum Enum,
                     Tri tri,
                     Coef alpha, Coef beta, Type type,
		    Layer layer);

RealLoop* Tri2kLoopVar10(Node *Ain, ConnNum Anum, 
		     Node *Bin, ConnNum Bnum, 
		     Node *Cin, ConnNum Cnum,
		     Node *Din, ConnNum Dnum,
		     Node *Ein, ConnNum Enum,
		     Tri tri,
		     Coef alpha, Coef beta, Type type,
		     Layer layer);


class Her2kProps
{
 public:
  Tri m_tri;
  Trans m_trans;
  Coef m_alpha, m_beta;
  Type m_type;
  Her2kProps(Tri tri, Trans trans, Coef alpha, Coef beta, Type type);
  virtual ~Her2kProps() {}
  virtual void DuplicateProps(const Her2kProps *orig);
  virtual void FlattenCore(ofstream &out) const;
  virtual void UnflattenCore(ifstream &in, SaveInfo &info);
};

class Her2k : public DLAOp<3,1>, public Her2kProps
{
 public:
  Her2k(Layer layer, Tri tri, Trans trans, Coef alpha, Coef beta, Type type);
  virtual NodeType GetType() const;
  static Node* BlankInst() { return  new Her2k(ABSLAYER, LOWER, NORMAL, COEFONE, COEFONE, REAL); }
  virtual Node* GetNewInst() { return BlankInst(); }
#if DOELEM
  virtual bool CanTransposeInputs() const {return false;}
  virtual bool ShouldCullDP() const;
#endif
  virtual void Prop();
  virtual void Duplicate(const Node *orig, bool shallow, bool possMerging);
  virtual Phase MaxPhase() const;
  virtual void PrintCode(IndStream &out);
  virtual ClassType GetNodeClass() const {return GetClass();}
  static ClassType GetClass() {return "Her2kLoop";}
  virtual void FlattenCore(ofstream &out) const;
  virtual void UnflattenCore(ifstream &in, SaveInfo &info);
  static Cost GetCost(Layer layer, const Sizes *m, const Sizes *k);

};

class Her2kLoopExp : public SingleTrans
{
 public:
  Layer m_fromLayer, m_toLayer;
  unsigned int m_var;
 Her2kLoopExp(Layer fromLayer, Layer toLayer, unsigned int variant) 
   : m_fromLayer(fromLayer), m_toLayer(toLayer), m_var(variant) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
};

//This is a triangular rank-2k update used in Elemental
class Tri2k : public DLAOp<5,1>, public Her2kProps
{
 public:
  Tri2k(Layer layer, Tri tri, Trans trans, Coef alpha, Coef beta, Type type);
  virtual Phase MaxPhase() const;
  virtual NodeType GetType() const;
  static Node* BlankInst() { return  new Tri2k(ABSLAYER, LOWER, NORMAL, COEFONE, COEFONE, REAL); }
  virtual Node* GetNewInst() { return BlankInst(); }
  virtual void Prop();
  virtual void PrintCode(IndStream &out);
  virtual ClassType GetNodeClass() const {return GetClass();}
  static ClassType GetClass() {return "localTri2k";}
  virtual void Duplicate(const Node *orig, bool shallow, bool possMerging);
  virtual bool CanTransposeInputs() const {return true;}
  virtual void FlattenCore(ofstream &out) const;
  virtual void UnflattenCore(ifstream &in, SaveInfo &info);
  static Cost GetCost(Layer layer, const Sizes *localDim1, const Sizes *localDim2, const Sizes *localDim3);
};

#if DOELEM
class DistHer2kToLocalTri2k : public SingleTrans
{
 public:
  virtual string GetType() const {return "Distributed Her2k to Local Tri2k";}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
  virtual Cost RHSCostEstimate(const Node *node) const;
};

class DistHer2kToLocalHer2kContrib : public SingleTrans
{
 public:
  DistType m_AType, m_BType;
  DistHer2kToLocalHer2kContrib(DistType aType, DistType bType)
    : m_AType(aType), m_BType(bType) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
  virtual Cost RHSCostEstimate(const Node *node) const;
};

class Tri2kTrans : public TransTransformation
{
 public:
  Tri2kTrans(ConnNum argNum, Trans trans) : TransTransformation(argNum,trans) {}
  virtual string GetTransType() const;
  virtual bool CanApply(const Node *node) const;
};
#endif //DOELEM

class Tri2kLowerLayer : public LowerLayer
{
 public:
 Tri2kLowerLayer(Layer fromLayer, Layer toLayer, DimName dim, Size bs)
   : LowerLayer(fromLayer, toLayer, dim, bs) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
};

#if DOBLIS
class Her2kToTri2K : public SingleTrans
{
 public:
  Layer m_layer;
 Her2kToTri2K(Layer layer) : m_layer(layer) {}
  virtual string GetType() const {return "Her2k to Tri2K";}
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
};
#endif

class Tri2kToTriRK : public SingleTrans
{
 public:
  Layer m_fromLayer, m_toLayer;
 Tri2kToTriRK(Layer fromLayer, Layer toLayer) 
   : m_fromLayer(fromLayer), m_toLayer(toLayer) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
};

class Tri2kLoopExp : public SingleTrans
{
 public:  
  unsigned int m_var;
  Layer m_fromLayer, m_toLayer;
 Tri2kLoopExp(Layer fromLayer, Layer toLayer, unsigned int variant)
   : m_var(variant), m_fromLayer(fromLayer), m_toLayer(toLayer) {}
  virtual string GetType() const;
  virtual bool CanApply(const Node *node) const;
  virtual void Apply(Node *node) const;
  virtual bool IsRef() const {return true;}
};
#endif
