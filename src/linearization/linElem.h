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

class LinElem;

typedef std::vector<LinElem*> LinElemVec;
typedef LinElemVec::iterator LinElemVecIter;
typedef LinElemVec::const_iterator LinElemVecConstIter;

class LinElem
{
 public:
  LinElemVec m_children;
  LinElemVec m_inputs;
  bool m_addedToLinOrder;

 LinElem() : m_addedToLinOrder(false) {}
  virtual ~LinElem() {}

  void AddInputIfUnique(LinElem *elem);
  void AddChildIfUnique(LinElem *elem);

  inline bool HasAdded() const {return m_addedToLinOrder;}
  inline void SetAdded() {m_addedToLinOrder = true;} 
  inline void ClearAdded() {m_addedToLinOrder = false;} 

  virtual void Print() = 0;
  bool CanAddToLinearOrder() const;

  virtual bool IsClear() const {return false;}
};
