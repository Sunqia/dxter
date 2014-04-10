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



#include "base.h"
#include <cmath>
#include "transform.h"
#include "poss.h"
#include <cstring>
#include <math.h>
#include "costs.h"
#include <ostream>
#include <sstream>

using namespace std;

#if DOELEM
DistType STAR_STAR = D_STAR_STAR;
DistType MC_MR = D_MC_MR;
DistType MC_MR_T = D_MC_MR_T;
DistType MC_MR_H = D_MC_MR_H;
DistType MR_MC = D_MR_MC;
DistType MR_MC_T = D_MR_MC_T;
DistType MR_MC_H = D_MR_MC_H;
DistType MC_STAR = D_MC_STAR;
DistType MC_STAR_T = D_MC_STAR_T;
DistType MC_STAR_H = D_MC_STAR_H;
DistType STAR_MC = D_STAR_MC;
DistType STAR_MC_T = D_STAR_MC_T;
DistType STAR_MC_H = D_STAR_MC_H;
DistType MR_STAR = D_MR_STAR;
DistType MR_STAR_T = D_MR_STAR_T;
DistType MR_STAR_H = D_MR_STAR_H;
DistType STAR_MR = D_STAR_MR;
DistType STAR_MR_T = D_STAR_MR_T;
DistType STAR_MR_H = D_STAR_MR_H;
DistType VC_STAR = D_VC_STAR;
DistType VC_STAR_H = D_VC_STAR_H;
DistType VC_STAR_T = D_VC_STAR_T;
DistType STAR_VC = D_STAR_VC;
DistType VR_STAR = D_VR_STAR;
DistType STAR_VR = D_STAR_VR;
#endif

char END = '#';
char START = '+';


#if DOTENSORS
DistType::DistType(const DistType &rhs)
  :m_dists(NULL)
{
  m_numDims = rhs.m_numDims;
  if (m_numDims) {
    m_dists = new DistEntry[m_numDims];
    for (Dim dim = 0; dim < m_numDims; ++dim)
      m_dists[dim] = rhs.m_dists[dim];
  }
}

DistType& DistType::operator=(const DistType &rhs)
{
  m_numDims = rhs.m_numDims;
  if (m_dists)
    delete [] m_dists;
  if (m_numDims) {
    m_dists = new DistEntry[m_numDims];
    for (Dim dim = 0; dim < m_numDims; ++dim)
      m_dists[dim] = rhs.m_dists[dim];
  }
  else
    m_dists = NULL;
  return *this;
}

void DistType::PrepForNumDims(Dim numDims)
{
  if (m_dists)
    delete [] m_dists;
  m_numDims = numDims;
  if (numDims)
    m_dists = new DistEntry[numDims];
  else
    m_dists = NULL;
}

DistType::~DistType()
{
  if (m_dists) {
    delete [] m_dists;
    m_dists = NULL;
  }
  m_numDims = 0;
}

DimSet DistType::UsedGridDims() const
{
  //Reflect changes in IsSane
  DimSet set;
  for (Dim dim = 0; dim < m_numDims; ++dim) {
    DimVec vec = m_dists[dim].DistEntryDims();
    DimVecIter iter = vec.begin();
    for(; iter != vec.end(); ++iter) {
      if (!set.insert(*iter).second) {
	cout << DistTypeToStr(*this) << endl;
	throw;
      }
    }
  }
  return set;
}

bool DistType::IsSane() const
{
  //Reflect changes in UsedGridDims
  DimSet set;
  for (Dim dim = 0; dim < m_numDims; ++dim) {
    DimVec vec = m_dists[dim].DistEntryDims();
    DimVecIter iter = vec.begin();
    for(; iter != vec.end(); ++iter) {
      if (!set.insert(*iter).second) {
	return false;
      }
    }
  }
  return true;
}


void DistType::SetToDefault(Dim numDims)
{
  if (numDims > NUM_GRID_DIMS)
    throw;
  m_numDims = numDims;
  if (m_dists)
    delete [] m_dists;
  m_dists = new DistEntry[numDims];

  unsigned int numStartDists = ceil((double)NUM_GRID_DIMS / numDims);
  unsigned int numEndDists = floor((double)NUM_GRID_DIMS / numDims);
  unsigned int tipPoint = numDims;
  if (numEndDists != numStartDists) 
    tipPoint = (NUM_GRID_DIMS - numEndDists * numDims) / (numStartDists - numEndDists);

  /*    
  cout <<"numDims: " << numDims 
       << "\ngridDims: " << NUM_GRID_DIMS << endl;
  
  cout << "start " << numStartDists << endl;
  cout << "end " << numEndDists << endl;
  cout << "tip " << tipPoint << endl;
  */
  Dim currDistDim = 0;
  for (Dim i = 0; i < numDims; ++i) {
    DimVec vec;
    for (unsigned int j = 0; j < (i < tipPoint ? numStartDists : numEndDists); ++j)  {
      vec.push_back(currDistDim);
      currDistDim++;
    }
    m_dists[i].DimsToDistEntry(vec);
  }
}

void DistType::SetToStar(Dim numDims)
{
  if (numDims > NUM_GRID_DIMS)
    throw;
  m_numDims = numDims;
  if (m_dists)
    delete [] m_dists;
  m_dists = new DistEntry[numDims];

  for(Dim dim = 0; dim < numDims; ++dim)
    m_dists[dim].SetToStar();
}

string DistEntry::DistEntryToStr() const
{
#if (MAX_NUM_DIMS > 10)
  this code needs to be updated since dim "12"
    will be reversed to be 21
#endif 
    DimVec vec = DistEntryDims();
  if (vec.empty())
    return "*";
  std::stringstream ret;
  ret << "m";
  DimVecIter iter = vec.begin();
  for (; iter != vec.end(); ++iter) {
    ret << "_";
    ret << *iter;
  }
  return ret.str();
}

string DistType::QuickStr() const
{
  std::stringstream ret;
  if (!m_dists)
    throw;
  for(Dim dim = 0; dim < m_numDims; ++dim) {
    ret << m_dists[dim].m_val << " ";
  }
  return ret.str();
}

DimVec DistEntry::DistEntryDims() const
{
#if 0
  if (dist > 3000) {
    cout << "big dist to start: " << dist << endl;
    throw;
  }
  cout << "\n\nstarting with dist = " << dist << endl;
#endif
  DimVec vec;
  if (IsStar())
    return vec;
  unsigned int currStage = NUM_GRID_DIMS;
  unsigned int distVal = m_val-1;
  unsigned int numDists = 1;
  while (distVal > currStage) {
    if (!currStage) {
      cout << "starting dist " << m_val << endl;
      cout << "numDists " << numDists << endl;
      cout << "!currStage\n";
      throw;
    }
#if 0
    cout << "distVal bef " << distVal << endl;
    cout << "numDists bef " << numDists << endl;
    cout << "currStage bef " << currStage << endl;
    if ((distVal - currStage) >= distVal) {
      cout << "problem with current\n";
      throw;
    }
#endif
    distVal -= currStage;
    numDists++;
    currStage *= NUM_GRID_DIMS;
#if 0
    cout << "distVal aft " << distVal << endl;
    cout << "numDists aft " << numDists << endl;
    cout << "currStage aft " << currStage << endl;
#endif
  }
  string out;
  while (numDists) {
#if 0
    cout << "inserting " << distVal % MAX_NUM_DIMS << endl;
    cout << "overwriting " << distVal << " with " << distVal / MAX_NUM_DIMS << endl;
    cout << "numDists " << numDists << " being decremented" << endl;
#endif
    vec.insert(vec.begin(), distVal % NUM_GRID_DIMS);
    distVal = distVal / NUM_GRID_DIMS;
    --numDists;
  }
  if (distVal != 0) {
    cout << endl << distVal << " != 0\n";
    cout << "dist = " << m_val << endl;
    throw;
  }
  return vec;
}

void DistEntry::DimsToDistEntry(DimVec dims)
{
  unsigned int currStage = 1;
  unsigned int distVal = 0;
  DimVecConstRevIter iter = dims.rbegin();
  for(; iter != dims.rend(); ++iter) {
    if (*iter > NUM_GRID_DIMS)
      throw;
    //offset coming from the left
    distVal += currStage;
    //offset on the right
    distVal += currStage * *iter;
    
    currStage *= NUM_GRID_DIMS;
  }

  if (distVal > 1410273309) {
    cout << "big distVal: " << distVal << endl;
    throw;
  }
    
  m_val = distVal;
}

string DistType::str() const
{
  string out = "[";
  for (unsigned int i = 0; i < m_numDims; ++i) {
    out += m_dists[i].DistEntryToStr();
    if (i+1 < m_numDims)
      out += ",";
  }
  out += "]";
  return out;
}



string DistTypeToStr(const DistType &type)
{
  return type.str();
}

#if DOTENSORS
DimVec MapIndicesToDims(const string &indices, const string &dimIndices)
{
  DimVec map;
  map.reserve(indices.length());
  string::const_iterator iter = indices.begin();
  for(; iter != indices.end(); ++iter) {
    map.push_back(dimIndices.find(*iter));
  }
  return map;
}

bool IsPrefix(const DimVec &isPrefix, const DimVec &dims)
{
  if (dims.empty() || isPrefix.empty())
    return false;
  DimVecConstIter dimsIter = dims.begin();
  DimVecConstIter isPrefixIter = isPrefix.begin();
  for(; dimsIter != dims.end() && isPrefixIter != isPrefix.end(); ++dimsIter, ++isPrefixIter) {
    if (*isPrefixIter != *dimsIter)
      return false;
  }
  if (isPrefixIter != isPrefix.end() && dimsIter == dims.end())
    return false;
  return true;
}
#endif

void GetLocalSizes(DistType dist, const SizesArray sizes, SizesArray localSizes)
{
  const Dim  numDims = dist.m_numDims;
  for (Dim dim = 0; dim < numDims; ++ dim) {
    unsigned int distVal = dist.m_dists[dim].m_val;
    localSizes[dim] = sizes[dim];
    if (distVal != 0) {
      distVal--;
      unsigned int currStage = NUM_GRID_DIMS;
      unsigned int numDists = 0;
      unsigned int coef = 1;
      while (distVal >= currStage) {
	distVal -= currStage;
	numDists++;
	currStage *= NUM_GRID_DIMS;
      }
      string out;
      while (distVal > 0) {
	Dim distDim = distVal % NUM_GRID_DIMS;
	coef *= GridLens[distDim];
	distVal = distVal / NUM_GRID_DIMS;
	--numDists;
      }    
      localSizes[dim].SetCoeff(1.0 / coef);
    }
  }
}

void GetLocalSizes(DistType dist, Dim dim, const Sizes* sizes, Sizes* localSizes)
{
  unsigned int distVal = dist.m_dists[dim].m_val;
  *localSizes = *sizes;
  if (distVal != 0) {
    distVal--;
    unsigned int currStage = NUM_GRID_DIMS;
    unsigned int numDists = 0;
    unsigned int coef = 1;
    while (distVal >= currStage) {
      distVal -= currStage;
      numDists++;
      currStage *= NUM_GRID_DIMS;
    }
    string out;
    while (distVal > 0) {
      Dim distDim = distVal % NUM_GRID_DIMS;
      coef *= GridLens[distDim];
      distVal = distVal / NUM_GRID_DIMS;
      --numDists;
    }    
    localSizes->SetCoeff(1.0 / coef);
  }
}
#endif

#if DOELEM
string DistTypeToStr(const DistType &distType)
{
  switch(distType) {
    case D_STAR_STAR:
      return "STAR_STAR";
      break;
    case D_MC_MR:
      return "MC_MR";
      break;
    case D_VC_STAR_H:
      return "Adj_VC_STAR";
      break;
    case D_VC_STAR_T:
      return "Trans_VC_STAR";
      break;
    case D_MC_MR_T:
      return "Trans_MC_MR";
      break;
    case D_MC_MR_H:
      return "Adj_MC_MR";
      break;
    case D_MR_MC:
      return "MR_MC";
      break;
    case D_MR_MC_T:
      return "Trans_MR_MC";
      break;
    case D_MR_MC_H:
      return "Adj_MR_MC";
      break;
    case D_MC_STAR:
      return "MC_STAR";
      break;
    case D_MC_STAR_T:
      return "Trans_MC_STAR";
      break;
    case D_MC_STAR_H:
      return "Adj_MC_STAR";
      break;
    case D_STAR_MC:
      return "STAR_MC";
      break;
    case D_STAR_MC_T:
      return "Trans_STAR_MC";
      break;
    case D_STAR_MC_H:
      return "Adj_STAR_MC";
      break;
    case D_MR_STAR:
      return "MR_STAR";
      break;
    case D_MR_STAR_T:
      return "Trans_MR_STAR";
      break;
    case D_MR_STAR_H:
      return "Adj_MR_STAR";
      break;
    case D_STAR_MR:
      return "STAR_MR";
      break;
    case D_STAR_MR_T:
      return "Trans_STAR_MR";
      break;
    case D_STAR_MR_H:
      return "Adj_STAR_MR";
      break;
    case D_VC_STAR:
      return "VC_STAR";
      break;
    case D_STAR_VC:
      return "STAR_VC";
      break;
    case D_VR_STAR:
      return "VR_STAR";
      break;
    case D_STAR_VR:
      return "STAR_VR";
      break;
    default:
      throw;
      break;
  }
}


DistType GetBaseDistType(DistType distType)
{
  switch(distType) {
    case D_MC_STAR_T:
    case D_MC_STAR_H:
      return D_MC_STAR;
    case D_STAR_MC_T:
    case D_STAR_MC_H:
      return D_STAR_MC;
    case D_MR_STAR_T:
    case D_MR_STAR_H:
      return D_MR_STAR;
    case D_STAR_MR_T:
    case D_STAR_MR_H:
      return D_STAR_MR;
    default:
      return distType;
  }
}


#endif //DOELEM



bool AddPossToMMap(PossMMap &mmap, Poss *elem, size_t hash, bool deep)
{
  PossMMapRangePair pair = mmap.equal_range(hash);
  for( ; pair.first != pair.second; ++pair.first) {
    Poss *poss = (*(pair.first)).second;
    if ((deep && *poss == *elem) || (!deep && poss == elem)) {
      return false;
    }
  }
  mmap.insert(PossMMapPair(hash, elem));
  return true;
}

template<>
bool AddElemToVec(std::vector<PSet*> &vec, PSet *elem, bool deep)
{
  if (deep)
    throw;
  std::vector<PSet*>::iterator iter = vec.begin();
  for( ; iter != vec.end(); ++iter) {
    if (*iter == elem) {
      return false;
    }
  }
  vec.push_back(elem);
  return true;
}

template<class T>
bool AddElemToVec(std::vector<T*> &vec, T *elem, bool deep)
{
  typename std::vector<T*>::iterator iter = vec.begin();
  for( ; iter != vec.end(); ++iter) {
    if ((deep && **iter == *elem) || (!deep && *iter == elem)) {
      return false;
    }
  }
  vec.push_back(elem);
  return true;
}

template bool AddElemToVec<Node>(std::vector<Node*> &vec, Node *elem, bool deep);
template bool AddElemToVec<Poss>(std::vector<Poss*> &vec, Poss *elem, bool deep);


string BoolToStr(bool boolVal)
{
  if (boolVal)
    return "TRUE";
  else
    return "FALSE";
}

string DiagToStr(Diag diag)
{
  if (diag == UNIT)
    return "UNIT";
  else if (diag == NONUNIT)
    return "NONUNIT";
  else
    throw;
}


string TransToStr(Trans trans)
{
  switch(trans) {
    case (NORMAL):
      return "NORMAL";
    case (TRANS):
      return "TRANSPOSE";
    case (CONJTRANS):
      return "ADJOINT";
    default: {
      return "unknown";
    }
  }
}

char TransToChar(Trans trans)
{
  switch(trans) {
    case (NORMAL):
      return 'N';
    case (TRANS):
      return 'T';
    case (CONJTRANS):
      return 'H';
    default:
      return 'U';
  }
}


string SideToStr(Side side)
{
  switch(side) {
    case (LEFT):
      return "LEFT";
    case (RIGHT):
      return "RIGHT";
    default:
      return "unknown";
  }
}

char SideToChar(Side side)
{
  switch(side) {
    case (LEFT):
      return 'L';
    case (RIGHT):
      return 'R';
    default:
      return 'U';
  }
}

string TriToStr(Tri tri)
{
  switch(tri) {
    case (LOWER):
      return "LOWER";
    case (UPPER):
      return "UPPER";
    default:
      return "unknown";
  }
}

char TriToChar(Tri tri)
{
  switch(tri) {
    case (LOWER):
      return 'L';
    case (UPPER):
      return 'U';
    default:
      return 'K';
  }
}

Tri SwapTri(Tri tri)
{
  switch(tri) {
  case(LOWER):
    return UPPER;
  case(UPPER):
    return LOWER;
  default:
    throw;
  }
}

bool IsTrans(Trans trans)
{
  return trans == TRANS || trans == CONJTRANS;
}


string Name::str() const
{
#if DOELEM
  string name = m_name;
  if (m_type == D_MC_MR)
    return name;
  else if (m_type == UNKNOWN)
    return name;
  else
    return name + "_" + DistTypeToStr(m_type);
#elif DOTENSORS
  return m_name + "_" + DistTypeToStr(m_type);
#else
  return m_name;
#endif
}

Name& Name::operator= (const Name &rhs)
{
#if DODM
  m_type = rhs.m_type;
#endif
  m_name = rhs.m_name;
#if DOTENSORS
  m_indices = rhs.m_indices;
#endif
  return *this;
}

void Name::Flatten(ofstream &out) const
{
#if DODM
  WRITE(m_type);
#endif
#if DOTENSORS
  out << m_indices << endl;
#endif
  out << m_name << endl;

}

void Name::Unflatten(ifstream &in)
{
#if DODM
  READ(m_type);
#endif
#if DOTENSORS
  getline(in, m_indices);
#endif
  getline(in, m_name);
}

NodeConn::NodeConn(Node *n, unsigned int num)
: m_n(n), m_num(num)
{
  if (!n)
    cout << "!n\n";
}

NodeConn::NodeConn(const NodeConn *conn)
: m_n(conn->m_n), m_num(conn->m_num)
{
  if (!m_n)
    cout << "!m_n\n";
}

bool NodeConn::operator==(const NodeConn &rhs) const
{
  if (m_num != rhs.m_num)
    return false;
  if (!m_n || !rhs.m_n) {
    cout << "bad conn\n";
    throw;
  }
  return *m_n == *(rhs.m_n);
}

void NodeConn::SetNode(Node *node)
{
  if (!node) {
    cout << "SetNode(null)\n";
    throw;
  }
  m_n = node;
}

void NodeConn::Flatten(ofstream &out) const
{
  WRITE(m_n);
  WRITE(m_num);
}

void NodeConn::Unflatten(ifstream &in)
{
  READ(m_n);
  READ(m_num);
}

bool FoundInNodeVec(const NodeVec &vec, const Node *node)
{
  NodeVecConstIter iter = vec.begin();
  for(; iter != vec.end(); ++iter)
    if (*iter == node)
      return true;
  return false;
}


string LayerNumToStr(Layer layer)
{
  switch(layer) {
  case(0):
    return "0";
  case(1):
    return "1";
  case(2):
    return "2";
  case(3):
    return "3";
  case(4):
    return "4";
    case (5):
    return "5";
  default:
    throw;
  }
}

#if DOELEM
void GetLocalSizes(DistType dist, const Sizes *m, const Sizes *n, Sizes &localM, Sizes &localN)
{
  switch(dist) {
  case D_STAR_STAR:
    localM = *m;
    localN = *n;
    break;
  case D_MC_MR:
    localM = *m;
    localM.SetCoeff(1.0 / R);
    localN = *n;
    localN.SetCoeff(1.0 / C);
    break;
  case D_MR_MC:
    localM = *m;
    localM.SetCoeff(1.0/C);
    localN = *n;
    localN.SetCoeff(1.0/R);
      break;
    case D_MC_STAR:
      localM = *m;
      localM.SetCoeff(1.0/R);
      localN = *n;
      break;
    case D_MC_STAR_T:
    case D_MC_STAR_H:
      localM = *n;
      localM.SetCoeff(1.0/R);
      localN = *m;
      break;
    case D_STAR_MC:
      localM = *m;
      localN = *n;
      localN.SetCoeff(1.0/R);
      break;
    case D_STAR_MC_T:
    case D_STAR_MC_H:
      localM = *n;
      localN = *m;
      localN.SetCoeff(1.0/ R);
      break;
    case D_MR_STAR:
      localM = *m;
      localM.SetCoeff(1.0 / C);
      localN = *n;
      break;
    case D_MR_STAR_T:
    case D_MR_STAR_H:
      localM = *n;
      localM.SetCoeff(1.0 / C);
      localN = *m;
      break;
    case D_STAR_MR:
      localM = *m;
      localN = *n;
      localN.SetCoeff(1.0 / C);
      break;
    case D_STAR_MR_T:
    case D_STAR_MR_H:
      localM = *n;
      localN = *m;
      localN.SetCoeff(1.0 / C);
      break;
    case D_VC_STAR:
    case D_VR_STAR:
      localM = *m;
      localM.SetCoeff(1.0 / P);
      localN = *n;
      break;
    case D_STAR_VC:
    case D_STAR_VR:
      localM = *m;
      localN = *n;
      localN.SetCoeff(1.0 / P);
      break;
    case D_MC_MR_H:
    case D_MC_MR_T:
      localM = *n;
      localM.SetCoeff(1.0 / R);
      localN = *m;
      localN.SetCoeff(1.0 / C);
      break;
    case D_MR_MC_T:
    case D_MR_MC_H:
      localM = *n;
      localM.SetCoeff(1.0 / C);
      localN = *m;
      localN.SetCoeff(1.0 / R);
    case D_VC_STAR_T:
    case D_VC_STAR_H:
      localM = *n;
      localM.SetCoeff(1.0 / P);
      localN = *m;
      break;
    default:
      throw;
  }
}
#endif
