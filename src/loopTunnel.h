/*
    This file is part of DxTer.
    DxTer is a prototype using the Design by Transformation (DxT)
    approach to program generation.

    Copyright (C) 2013, The University of Texas and Bryan Marker

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

class LoopTunnel : public PossTunnel
{
 public:
  UpStat 
    m_statTL, m_statTR,
    m_statBL, m_statBR;
#if TWOD
  Sizes *m_msizes, *m_nsizes;
  Sizes *m_mlsizes, *m_nlsizes;
#else
  SizesArray m_sizes, m_lsizes;
#endif
  bool m_indepIters;

  LoopTunnel(PossTunType type);
  virtual ~LoopTunnel();
  void SetUpStats( UpStat statTL, UpStat statTR,
		   UpStat statBL, UpStat statBR );
  void SetAllStats(UpStat stat);
  void CopyTunnelInfo(const LoopTunnel *tun);
  virtual bool QuadInUse(Quad quad, bool atEnd) const;
  bool IsConst() const;
  UpStat GetUpStat(Quad quad) const;
  bool AllFullyUpdated() const;
  virtual void SanityCheck();  
#if DODM
  virtual const DistType& GetDistType(unsigned int num) const;
#endif
  inline void SetIndepIters() {m_indepIters = true;}
  inline bool IndepIters() const {return m_indepIters;}
  bool InputIsTemp() const;

  virtual unsigned int NumOutputs() const;
  static Node* BlankInst() { return new LoopTunnel(LASTTUNNEL);}
  virtual Node* GetNewInst() {return BlankInst(); }
  virtual PossTunnel* GetSetTunnel();
  virtual void Prop();
  virtual void Duplicate(const Node *orig, bool shallow, bool possMerging);
  virtual NodeType GetType() const;
  virtual bool IsLoopTunnel() const {return true;}
  virtual LoopTunnel* GetMatchingOutTun() const;
  virtual LoopTunnel* GetMatchingInTun() const;
#if TWOD
  virtual const Sizes* GetM(unsigned int num) const;
  virtual const Sizes* GetN(unsigned int num) const;
  virtual const Sizes* LocalM(unsigned int num) const;
  virtual const Sizes* LocalN(unsigned int num) const;
#else
  virtual const unsigned int NumDims(unsigned int num) const;
  virtual const Sizes* Len(unsigned int num, unsigned int dim) const;
  virtual const Sizes* LocalLen(unsigned int num, unsigned int dim) const;
#endif
  virtual Name GetName(unsigned int num) const;
 Name GetOrigName() const;
  Loop* GetMyLoop() const;
  virtual ClassType GetNodeClass() const {return GetClass();}
  static ClassType GetClass() {return "LoopTunnel";}
  
  virtual void PrintVarDeclarations() const {throw;}
  LoopType GetLoopType() const;

  virtual void StartFillingSizes();
  virtual void AppendSizes(unsigned int execNum, unsigned int numIters, unsigned int parFactor);
  virtual void UpdateLocalSizes();
  virtual void ClearSizeCache();

  virtual void FlattenCore(ofstream &out) const;
  virtual void UnflattenCore(ifstream &in, SaveInfo &info);

  virtual bool Overwrites(const Node *input, unsigned int num) const;
  virtual bool KeepsInputVarLive(Node *input, unsigned int numIn, unsigned int &numOut) const;
};
