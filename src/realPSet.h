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

#include "base.h"
#include "poss.h"
#include "possTunnel.h"
#include "basePSet.h"

class RealPSet : public BasePSet
{
 public:
  PossMMap m_posses;
  NodeVec m_inTuns;
  NodeVec m_outTuns;
  string m_functionality;
  PSet();
  PSet(Poss *poss);
  virtual ~PSet();
  void AddPoss(Poss *poss);
  void AddPossesOrDispose(PossMMap &mmap, PossMMap *added = NULL);
  GraphNum NumPosses() {return m_posses.size();}
  bool operator==(const PSet &rhs) const;
  virtual void Prop();
  virtual bool TakeIter(const TransMap &trans, const TransMap &simplifiers);
  virtual void ClearBeforeProp();
  bool GlobalSimplification(const TransMap &globalSimplifiers, const TransMap &simplifiers);
  virtual void Duplicate(const BasePSet *orig, NodeMap &map, bool possMerging);
  virtual PSet* GetNewInst() {return new RealPSet;}
  void PatchAfterDuplicate(NodeMap &map);
  void CombineAndRemoveTunnels();
  void RemoveAndDeletePoss(Poss *poss, bool removeFromMyList);
  void Simplify(const TransMap &simplifiers, bool recursive = false);
  //  void RemoveDups();
  void ClearFullyExpanded();
  virtual bool IsTransparent() const {return true;}
  void Cull(Phase phase);
  bool MergePosses(const TransMap &simplifiers, CullFunction cullFunc);
  void FormSets(unsigned int phase);
  virtual GraphNum TotalCount() const;
  virtual void InlinePoss(Poss *inliningPoss, PossMMap &newPosses) = 0;
  virtual void FormSetAround();
  virtual Poss* GetCurrPoss() const;

  void Flatten(ofstream &out) const;
  virtual void FlattenCore(ofstream &out) const {}
  void Unflatten(ifstream &in, SaveInfo &info);
  virtual void UnflattenCore(ifstream &in, SaveInfo &info) {}

  virtual void BuildDataTypeCache();
  virtual void ClearDataTypeCache();

#if DOBLIS
  virtual bool IsCritSect() const {return false;}
  Comm ParallelismWithinCurrentPosses() const;
  bool RemoveParallelization(Comm comm);
  void ReplaceAllComms(Comm comm1, Comm comm2);
#endif //DOBLIS

  const string& GetFunctionalityString() const;
};

