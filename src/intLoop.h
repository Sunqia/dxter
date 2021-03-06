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
#include "transform.h"
#include "basePSet.h"
#include "comm.h"
#include "LLDLA.h"
#include "costs.h"

#if DOLOOPS

class LoopFusionStubTrans;
extern LoopFusionStubTrans *LoopFusionStub;

enum PartDir { PARTDOWN,
	       PARTRIGHT,
	       PARTDIAG,
	       PARTUPWARD,
	       PARTLEFT,
	       PARTDIAGBACK,
	       LASTPARTDIR };

enum MatPart { PART0, PART1, PART2,
	       PART00, PART01, PART02, PART10, PART11, PART12, PART20, PART21, PART22,
	       LASTPART };

enum LoopType { ELEMLOOP,
		BLISLOOP,
		LLDLALOOP,
		TENSORLOOP,
		UNKNOWNLOOP };

enum UpStat { FULLUP,
	      PARTUP,
	      NOTUP,
	      BADUP };

enum Quad { TL, TR,
	     BL, BR,
	     LASTQUAD };

enum BSSizeEnum { 
#if DOELEM
  USEELEMBS,
#elif DOBLIS
  USEBLISMC,
  USEBLISKC,
  USEBLISNC,
  USEBLISOUTERBS,
#elif DOTENSORS
  USETENSORBS,
#elif DOLLDLA
  USELLDLAMUSINGLE,
  USELLDLA2MUSINGLE,
  USELLDLA3MUSINGLE,
  USELLDLAMUDOUBLE,
  USELLDLA2MUDOUBLE,
  USELLDLA3MUDOUBLE,
#endif
  USEUNITBS,
  BADBSSIZE 
};

class BSSize
{
 public:
  BSSizeEnum m_val;
#if DOLLDLA
  unsigned int m_multiple;
#endif
 
  
 BSSize() : m_val(BADBSSIZE)
#if DOLLDLA
    , m_multiple(0) 
#endif
{}
  
  explicit BSSize(BSSizeEnum val)
    : m_val(val)
#if DOLLDLA
    , m_multiple(1) 
#endif
    {}

  explicit BSSize(BSSizeEnum val, unsigned int multiple)
    : m_val(val)
#if DOLLDLA
, m_multiple(multiple) 
#endif
  {}

  bool operator==(const BSSize &rhs) const 
#if DOLLDLA
  {return m_val == rhs.m_val && m_multiple == rhs.m_multiple;}
#else
  {return m_val == rhs.m_val;}
#endif

  bool operator!=(const BSSize &rhs) const 
#if DOLLDLA
  {return m_val != rhs.m_val || m_multiple != rhs.m_multiple;}
#else
  {return m_val != rhs.m_val;}
#endif

  string VarName() const;
  string SubSizeStr() const;
  string Str() const;

  inline Size GetSize() const {
    //Just a sanity check; upperbound can be changed
#if DOLLDLA
    if (m_multiple == 0 || m_multiple > 32) {
      LOG_FAIL("replacement for throw call");
      throw;
    }
#endif
    switch(m_val)
      {
#if DOELEM
      case (USEELEMBS):
	return ELEM_BS;
#elif DOBLIS
      case (USEBLISMC):
	return BLIS_MC_BS;
      case (USEBLISKC):
	return BLIS_KC_BS;
      case (USEBLISNC):
	return BLIS_NC_BS;
      case (USEBLISOUTERBS):
	return BLIS_OUTER_BS;
#elif DOTENSORS
      case (USETENSORBS):
	return TENSOR_BS;
#elif DOLLDLA
      case (USELLDLAMUSINGLE):
	return m_multiple * arch->SVecRegWidth();
      case (USELLDLA2MUSINGLE):
	return m_multiple*2*arch->SVecRegWidth();
      case (USELLDLA3MUSINGLE):
	return m_multiple*3*arch->SVecRegWidth();
      case (USELLDLAMUDOUBLE):
	return m_multiple*arch->DVecRegWidth();
      case (USELLDLA2MUDOUBLE):
	return m_multiple*2*arch->DVecRegWidth();
      case (USELLDLA3MUDOUBLE):
	return m_multiple*3*arch->DVecRegWidth();
#endif
      case (USEUNITBS):
#if DOLLDLA
	return m_multiple*ONE;
#else
	return ONE;
#endif
      default:
	LOG_FAIL("replacement for throw call");
	throw;
      }
  }
};

#if DOELEM
extern BSSize ElemBS;
#elif DOBLIS
extern BSSize BlisMC;
extern BSSize BlisKC;
extern BSSize BlisNC;
extern BSSize BlisOuter;
#elif DOTENSORS
extern BSSize TensorBS;
#elif DOLLDLA
extern BSSize LLDLAMuSingle;
extern BSSize LLDLA2MuSingle;
extern BSSize LLDLA3MuSingle;
extern BSSize LLDLAMuDouble;
extern BSSize LLDLA2MuDouble;
extern BSSize LLDLA3MuDouble;
#endif
extern BSSize BadBS;
extern BSSize UnitBS;


unsigned int GetNumElems(PartDir dir);
string PartDirToStr(PartDir dir);

class SplitBase;
class LoopTunnel;

class LoopInterface
{
 public:
  virtual int GetBS() const = 0;
  virtual BSSize GetBSSize() const = 0;
  virtual LoopType GetType() const = 0;
#if TWOD
  virtual DimName GetDimName() const = 0;
#endif
  virtual const IntSet& GetLabel() const = 0;
  virtual SplitBase* GetControl() const = 0;
  virtual bool CanMerge(BasePSet *pset) const = 0;
  virtual bool WorthFusing(BasePSet *pset) = 0;
  virtual unsigned int LoopLevel() const = 0;
#if DOBLIS
  virtual bool HasIndepIters() const = 0;
#endif
};

template <class PSetType>
class IntLoop : public PSetType, public LoopInterface
{
 public:
  IntLoop() {}
 IntLoop(Poss *poss) : PSetType(poss) {}
  virtual bool CanMerge(BasePSet *pset) const;
  virtual bool WorthFusing(BasePSet *pset);
  SplitBase* GetControl() const;
  virtual Cost Prop();
  virtual unsigned int LoopLevel() const;
  virtual void PrePrint(IndStream &out, Poss *poss);
  virtual void PostPrint(IndStream &out, Poss *poss);
  virtual bool IsLoop() const {return true;}
  virtual bool IsTransparent() const {return false;}
};


class LoopFusionStubTrans : public SingleTrans
{
 public:
  virtual string GetType() const {return "Loop Fusion";}
  virtual bool IsSingle() const {return true;}
  virtual bool CanApply(const Node *node) const {LOG_FAIL("replacement for throw call"); throw;}
  virtual void Apply(Node *node) const {LOG_FAIL("replacement for throw call"); throw;}
};
#endif //DOLOOPS
