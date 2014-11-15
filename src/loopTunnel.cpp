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



#include "loopTunnel.h"
#include "elemRedist.h"
#include <cmath>
#include "helperNodes.h"
#include "splitBase.h"
#include "splitSingleIter.h"


LoopTunnel::LoopTunnel(TunType type)
: Tunnel(type)
{
  m_statTL = BADUP;
  m_statTR = BADUP;
  m_statBL = BADUP;
  m_statBR = BADUP;
#if TWOD
  m_msizes = NULL;
  m_nsizes = NULL;
#if DODM
  m_mlsizes = NULL;
  m_nlsizes = NULL;
#endif
#else
  m_sizes = NULL;
  m_lsizes = NULL;
#endif
  m_indepIters = false;
}

LoopTunnel::~LoopTunnel()
{
#if TWOD
  if (m_msizes) {
    delete m_msizes;
    m_msizes = NULL;
    delete m_nsizes;
    m_nsizes = NULL;
#if DODM
    delete m_mlsizes;
    m_mlsizes = NULL;
    delete m_nlsizes;
    m_nlsizes = NULL;
#endif
  }
#else
  if (m_sizes) {
    delete [] m_sizes;
    m_sizes = NULL;
    delete [] m_lsizes;
    m_lsizes = NULL;
  }
#endif
}

void LoopTunnel::SetUpStats( UpStat statTL, UpStat statTR,
                            UpStat statBL, UpStat statBR )
{
  m_statTL = statTL;
  m_statTR = statTR;
  m_statBL = statBL;
  m_statBR = statBR;
}

void LoopTunnel::SetAllStats(UpStat stat)
{
  SetUpStats(stat,stat,stat,stat);
}

void LoopTunnel::CopyTunnelInfo(const LoopTunnel *tun)
{
  m_statTL = tun->m_statTL;
  m_statTR = tun->m_statTR;
  m_statBL = tun->m_statBL;
  m_statBR = tun->m_statBR;
  m_indepIters = tun->m_indepIters;
}

Tunnel* LoopTunnel::GetSetTunnel()
{
  LoopTunnel *tun;
  if (m_tunType == POSSTUNIN || m_tunType == SETTUNIN)
    tun = new LoopTunnel(SETTUNIN);
  else if (m_tunType == POSSTUNOUT || m_tunType == SETTUNOUT)
    tun = new LoopTunnel(SETTUNOUT);
  else
    throw;
  tun->CopyTunnelInfo(this);
  return tun;
}

void LoopTunnel::Prop()
{
  if (!IsValidCost(m_cost)) {
    Tunnel::Prop();

    if (m_tunType == POSSTUNIN 
	&& !IsSplit()) 
      {
	Node *found = NULL;
	NodeConnVecIter iter = m_children.begin();
	for( ; iter != m_children.end(); ++iter) {
	  if ((*iter)->m_num == 1) {
	    if (found)
	      throw;
	    else
	      found = (*iter)->m_n;
	  }
	}
      }

    if (m_statTL == BADUP || m_statTR == BADUP
	|| m_statBL == BADUP || m_statBR == BADUP)
      {
	cout << "update status not set for " << GetType() << " " << this << endl;
	cout << "it's a " << TunTypeToStr(m_tunType) << endl;
	throw;
      }
    if (m_tunType == POSSTUNOUT) {
      Node *in = m_inputs[m_inputs.size()-1]->m_n;
      if (!in->IsLoopTunnel())
	throw;
      if (((LoopTunnel*)in)->m_tunType != POSSTUNIN)
	throw;
    }


    if (m_tunType == SETTUNIN && m_inputs.size() != 1) {
      cout << "1 m_inputs.size() = " << m_inputs.size() << endl;
      for(ConnNum i = 0; i < m_inputs.size(); ++i) {
        cout << Input(i)->GetType() << " " << Input(i)->GetNameStr(0) << endl;
      }
      throw;
    }
    if (m_tunType == POSSTUNOUT && !IsCombine() && m_inputs.size() != 2) {
      if (Input(1)->GetNodeClass() != LoopTunnel::GetClass())
        throw;
      cout << "2 m_inputs.size() = " << m_inputs.size() << endl;
      throw;
      Input(1)->Prop();
    }
    else if (m_tunType == POSSTUNIN && !IsSplit()) {
      if (Input(0)->GetNodeClass() != LoopTunnel::GetClass()) {
        cout << Input(0)->GetNodeClass() << endl;
        throw;
      }
    }
    m_cost = ZERO;
  }
}

const DataTypeInfo& LoopTunnel::DataType(ConnNum num) const
{
  if (m_tunType == SETTUNIN)
    return GetRealTunnel()->InputDataType(0);
  else if (m_tunType == SETTUNOUT)
    return GetRealTunnel()->InputDataType(0);
  else
    return InputDataType(0);
}

#if TWOD
const Sizes* LoopTunnel::GetM(ConnNum num) const
{
  switch(m_tunType) 
    {
    case (POSSTUNOUT):
      if (num > 0)
        throw;
      return ((DLANode*)(Input(1)->Input(0)))->GetInputM(0);
    case (SETTUNOUT):
      if (num > 0)
        throw;
      return GetRealTunnel()->GetInputM(0);
    case (POSSTUNIN):
      if (num == 0 || num == 1) {
        const LoopTunnel *input = (LoopTunnel*)Input(0);
        return input->m_msizes;
      }
      else
        throw;
    default:
      throw;
    }
}

const Sizes* LoopTunnel::GetN(ConnNum num) const
{
  switch(m_tunType) 
  {
    //    case (SETTUNIN):
    case (POSSTUNOUT):
      if (num > 0)
        throw;
      return ((DLANode*)(Input(1)->Input(0)))->GetInputN(0);
    case (SETTUNOUT):
      if (num > 0)
        throw;
      return GetRealTunnel()->GetInputN(0);
    case (POSSTUNIN):
      if (num == 0 || num == 1) {
        const LoopTunnel *input = (LoopTunnel*)Input(0);
        return input->m_nsizes;
      }
      else
        throw;
    default:
      throw;
  }
}

#if DODM
const Sizes* LoopTunnel::LocalM(ConnNum num) const
{
  switch(m_tunType) 
  {
    //    case (SETTUNIN):
    case (POSSTUNOUT):
      if (num > 0)
        throw;
      return ((DLANode*)(Input(1)->Input(0)))->InputLocalM(0);
    case (SETTUNOUT):
      if (num > 0)
        throw;
      return GetRealTunnel()->InputLocalM(0);
    case (POSSTUNIN):
      if (num == 0) {
        const LoopTunnel *input = (LoopTunnel*)Input(0);
        return input->m_mlsizes;
      }
      else if (num == 1) {
        return InputLocalM(0);
      }
      else
        throw;
    default:
      throw;
  }
}

const Sizes* LoopTunnel::LocalN(ConnNum num) const
{
  switch(m_tunType) 
  {
    //    case (SETTUNIN):
    case (POSSTUNOUT):
      if (num > 0)
        throw;
      return ((DLANode*)(Input(1)->Input(0)))->InputLocalN(0);
    case (SETTUNOUT):
      if (num > 0)
        throw;
      return GetRealTunnel()->InputLocalN(0);
    case (POSSTUNIN):
      if (num == 0) {
        const LoopTunnel *input = (LoopTunnel*)Input(0);
        return input->m_nlsizes;
      }
      else if (num == 1) {
        return InputLocalN(0);
      }
      else
        throw;
    default:
      throw;
  }
}
#endif

#else
const Sizes* LoopTunnel::Len(ConnNum num,Dim dim) const
{
  switch(m_tunType) 
  {
    //    case (SETTUNIN):
    case (POSSTUNOUT):
      if (num > 0)
        throw;
      return ((DLANode*)(Input(1)->Input(0)))->InputLen(0,dim);
    case (SETTUNOUT):
      if (num > 0)
        throw;
      return GetRealTunnel()->InputLen(0,dim);
    case (POSSTUNIN):
      if (num == 0) {
        const LoopTunnel *input = (LoopTunnel*)Input(0);
        return input->m_sizes+dim;
      }
      else if (num == 1) {
        return InputLen(0,dim);
      }
      else
        throw;
    default:
      throw;
  }
}

const Dim LoopTunnel::NumDims(ConnNum num) const
{
  switch(m_tunType) 
  {
    case (SETTUNIN):
      if (num > 0)
	throw;
      return InputNumDims(0);
    case (POSSTUNOUT):
      if (num > 0)
        throw;
      return ((DLANode*)(Input(1)->Input(0)))->InputNumDims(0);
    case (SETTUNOUT):
      if (num > 0)
        throw;
      return GetRealTunnel()->InputNumDims(0);
    case (POSSTUNIN):
      if (num == 0) {
        const LoopTunnel *input = (LoopTunnel*)Input(0);
	return input->InputNumDims(0);
      }
      else
        throw;
    default:
      throw;
  }
}

const Sizes* LoopTunnel::LocalLen(ConnNum num,Dim dim) const
{
  switch(m_tunType) 
  {
    case (POSSTUNOUT):
      if (num > 0)
        throw;
      return ((DLANode*)(Input(1)->Input(0)))->InputLocalLen(0,dim);
    case (SETTUNOUT):
      if (num > 0)
        throw;
      return GetRealTunnel()->InputLocalLen(0,dim);
    case (POSSTUNIN):
      if (num == 0) {
        const LoopTunnel *input = (LoopTunnel*)Input(0);
        return input->m_lsizes+dim;
      }
      else
        throw;
    default:
      throw;
  }
}
#endif

unsigned int LoopTunnel::NumOutputs() const
{
  switch(m_tunType) 
  {
    case (POSSTUNOUT) :
    case (SETTUNIN) :
    case (SETTUNOUT) :
      return 1;
    case (POSSTUNIN) :
      return 2;
    default:
      cout << "bad tunnel type\n";
      throw;
      return -1;
  } 
}

Name LoopTunnel::GetName(ConnNum num) const
{
  if (num != 0 && num != 1)
    throw;
  Name name;
  if (GetLoopType() == BLISLOOP) {
    if (m_tunType == SETTUNOUT)
      return ((LoopTunnel*)(GetRealTunnel()->Input(0)))->GetOrigName();
    else if (m_tunType == POSSTUNOUT)
      return ((LoopTunnel*)Input(1))->GetOrigName();
    else if (m_tunType == SETTUNIN) {
      name.m_name = GetInputName(0).str();
//      if (name.m_name[name.m_name.length()-1] != '_')
//        name.m_name = name.m_name + "_";
      return name;
    }
  }
  return GetRealTunnel()->GetInputName(0);
}

Name LoopTunnel::GetOrigName() const
{
  if (m_tunType == SETTUNOUT)
    return ((LoopTunnel*)(GetRealTunnel()->Input(0)))->GetOrigName();
  if (m_tunType == POSSTUNIN)
    return ((LoopTunnel*)Input(0))->GetOrigName();
  else if (m_tunType == POSSTUNOUT)
    return ((LoopTunnel*)Input(m_inputs.size()-1))->GetOrigName();
  else if (m_tunType == SETTUNIN)
    return ((LoopTunnel*)Input(0))->GetName(InputConnNum(0));
  else
    throw;
}

LoopInterface* LoopTunnel::GetMyLoop() const
{
  BasePSet *set = NULL;
  if (m_tunType == SETTUNIN)
    set = m_pset;
  else if (m_tunType == SETTUNOUT)
    set = m_pset;
  else if (m_tunType == POSSTUNIN)
    set = ((Tunnel*)Input(0))->m_pset;
  else if (m_tunType == POSSTUNOUT)
    set = ((Tunnel*)Child(0))->m_pset;
  else 
    throw;
  if (!set->IsLoop()) {
    cout << "Loop Tunnel doesn't have Loop as PSet\n";
    if ( set->IsReal())
      cout << "is real\n";
    if ( set->IsShadow())
      cout << "is shadow\n";
    cout << TunTypeToStr(m_tunType) << endl;
    throw;
  }
  return dynamic_cast<LoopInterface*>(set);
}


void LoopTunnel::Duplicate(const Node *orig, bool shallow, bool possMerging)
{
  Tunnel::Duplicate(orig, shallow, possMerging);
  const LoopTunnel *tun = (LoopTunnel*)orig;
  CopyTunnelInfo(tun);
}

NodeType LoopTunnel::GetType() const
{
  return "LoopTunnel (" + Tunnel::GetType() + ")";
}

UpStat LoopTunnel::GetUpStat(Quad quad) const
{
  if (m_tunType == SETTUNIN || m_tunType == SETTUNOUT) {
    switch(quad)
    {
      case(TL):
        return m_statTL;
      case(TR):
        return m_statTR;
      case(BL):
        return m_statBL;
      case(BR):
        return m_statBR;
      default:
        throw;
    }
  }
  else if (m_tunType == POSSTUNIN) {
    return ((LoopTunnel*)Input(0))->GetUpStat(quad);
  }
  else if (m_tunType == POSSTUNOUT) {
    return ((LoopTunnel*)Child(0))->GetUpStat(quad);
  }
  else
    throw;
}

bool LoopTunnel::AllFullyUpdated() const
{
  return (m_statTL == FULLUP &&
          m_statTR == FULLUP &&
          m_statBL == FULLUP &&
          m_statBR == FULLUP);
}

bool LoopTunnel::QuadInUse(Quad quad, bool atEnd) const
{
  if (m_tunType == SETTUNIN) {
    Node *child = GetRealTunnel()->Child(0);
    if (!child->IsLoopTunnel())
      throw;
    return ((LoopTunnel*)child)->QuadInUse(quad, atEnd);
  }
  else if (m_tunType == POSSTUNIN) {
    NodeConnVecConstIter iter = m_children.begin();
    for( ; iter != m_children.end(); ++iter) {
      if (!(*iter)->m_n->IsTunnel(POSSTUNOUT))
        return true;
    }
    return false;
  }
  else {
    cout << TunTypeToStr(m_tunType) << endl;
    throw; 
  }
}

LoopTunnel* LoopTunnel::GetMatchingOutTun() const
{
  if (m_tunType == SETTUNIN)
    if (m_pset->IsReal()) 
      return (LoopTunnel*)(((LoopTunnel*)Child(0))->GetMatchingOutTun()->Child(0));
    else {
      LoopTunnel *possTunOut = ((LoopTunnel*)(GetRealTunnel()->Child(0)))->GetMatchingOutTun();
      LoopTunnel *realSetTunOut = (LoopTunnel*)(possTunOut->Child(0));
      return (LoopTunnel*)(m_pset->m_outTuns[FindInNodeVec(m_pset->GetReal()->m_outTuns, realSetTunOut)]);
    }
  else if (m_tunType != POSSTUNIN) 
    throw;
  
  NodeConnVecConstIter iter = m_children.begin();
  for(; iter != m_children.end(); ++iter) {
    const NodeConn *con = *iter;
    const Node *child = con->m_n;
    if (child->IsTunnel(POSSTUNOUT) &&
        child->IsLoopTunnel()) {
      return (LoopTunnel*)child;
    }
  }
  cout << "Didn't find matching out tun\n";
  throw;
}

LoopTunnel* LoopTunnel::GetMatchingInTun() const
{
  if (m_tunType == SETTUNOUT)
    if (m_pset->IsReal())
      return (LoopTunnel*)(((LoopTunnel*)Input(0))->GetMatchingInTun()->Input(0));
    else {
      LoopTunnel *possTunIn = ((LoopTunnel*)(GetRealTunnel()->Input(0)))->GetMatchingInTun();
      LoopTunnel *realSetTunIn = (LoopTunnel*)(possTunIn->Input(0));
      return (LoopTunnel*)(m_pset->m_inTuns[FindInNodeVec(m_pset->GetReal()->m_inTuns, realSetTunIn)]);
    }
  else if (m_tunType != POSSTUNOUT)
    throw;
  
  const Node *in = Input(1);
  if (in->IsTunnel(POSSTUNIN) && in->IsLoopTunnel()) {
    return (LoopTunnel*)in;
  }
  else {
    cout << "Didn't find matching in tun\n";
    throw;
  }
}

bool LoopTunnel::IsConst() const
{
  return AllFullyUpdated();
}

bool LoopTunnel::InputIsTemp() const
{
  if (m_tunType == POSSTUNIN) {
    return ((LoopTunnel*)(Input(0)))->InputIsTemp();
  }
  else if (m_tunType == SETTUNIN) {
    const ClassType type = Input(0)->GetNodeClass();
#if DOELEM||DOBLIS||DOLLDLA
    if (type == TempVarNode::GetClass())
      return true;
#endif
#if DOELEM||DOBLIS
    if (type == ScaleNode::GetClass()) {
      const ScaleNode *scale = (ScaleNode*)Input(0);
      return scale->Input(0)->GetNodeClass() == TempVarNode::GetClass();
    }
#endif
    return false;
  }
  else
    throw;
}

LoopType LoopTunnel::GetLoopType() const
{
  if (m_tunType == POSSTUNIN) {
    const Node *in = Input(0);
      if (!in->IsLoopTunnel())
        throw;
    return ((LoopTunnel*)in)->GetLoopType();
  }
  else if (m_tunType == POSSTUNOUT) {
    const Node *child = Child(0);
    if (!child->IsLoopTunnel())
      throw;
    return ((LoopTunnel*)child)->GetLoopType();
  }
  if (!m_pset) {
    throw;
  }
  return (dynamic_cast<LoopInterface*>(m_pset))->GetType();
}

void LoopTunnel::FlattenCore(ofstream &out) const
{
  Tunnel::FlattenCore(out);
  WRITE(m_statTL);
  WRITE(m_statTR);
  WRITE(m_statBL);
  WRITE(m_statBR);
  WRITE(m_indepIters);
}

void LoopTunnel::UnflattenCore(ifstream &in, SaveInfo &info) 
{
  Tunnel::UnflattenCore(in,info);
  READ(m_statTL);
  READ(m_statTR);
  READ(m_statBL);
  READ(m_statBR);
  READ(m_indepIters);
}

void LoopTunnel::StartFillingSizes()
{
  if (m_tunType != SETTUNIN)
    return;
  if (!m_pset->IsReal())
    return;
#if TWOD
  if (m_msizes)
    throw;
  m_msizes = new Sizes;
  m_nsizes = new Sizes;
#if DODM
  m_mlsizes = new Sizes;
  m_nlsizes = new Sizes;
#endif
#else
  if (m_sizes)
    throw;
  Dim numDims = InputNumDims(0);
  if (numDims) {
    m_sizes = new Sizes[numDims]();
    m_lsizes = new Sizes[numDims];
  }
  else {
    //scalar
    m_sizes = new Sizes[1]();
    m_lsizes = new Sizes[1];
  }
#endif
}

#if TWOD
void LoopTunnel::AppendSizes(unsigned int execNum, unsigned int numIters, unsigned int parFactor)
{
  if (m_tunType != SETTUNIN)
    return;
  if (!m_pset->IsReal())
    return;
  /*  if (!execNum) {
    throw;
    }*/
  if (!m_msizes)
    throw;
  const DLANode *input = (DLANode*)Input(0);
  ConnNum num = InputConnNum(0);
  const Sizes *ms = input->GetM(num);
  if (!ms) {
    cout << "!ms\n";
    throw;
  }
  const Sizes *ns = input->GetN(num);
#if DODM
  const Sizes *lms = input->LocalM(num);
  const Sizes *lns = input->LocalN(num);
#endif
  unsigned int length = ms->NumSizes();
  if (length != ns->NumSizes() 
#if DODM
      || length != lms->NumSizes() 
      || length != lns->NumSizes() 
#endif
      || length <= execNum) 
  {
    cout << ms->NumSizes() << endl;
    cout << ns->NumSizes() << endl;
#if DODM
    cout << lms->NumSizes() << endl;
    cout << lns->NumSizes() << endl;
#endif
    //    (*(((Loop*)m_pset)->m_posses.begin())).second->ForcePrint();
    cout << "Error: Cannot append sizes\n";
    cout << "length = " << std::to_string((long long int) length) << endl;
    cout << "ns->NumSizes() = " << std::to_string((long long int) ns->NumSizes()) << endl;
    cout << "execNum = " << std::to_string((long long int) execNum) << endl;
    cout << "length != ns->NumSizes() ? " << (length != ns->NumSizes()) << endl;
    cout << "length <= execNum ? " << (length <= execNum) << endl;

    cout << Input(0)->Input(0)->GetType() << endl;
    LoopInterface *loop = ((LoopTunnel*)(Input(0)->Input(0)))->GetMyLoop();
    SplitSingleIter *split = (SplitSingleIter*)(loop->GetControl());
    cout << split->NumberOfLoopExecs() << endl;
    cout << "outer loop bs " << loop->GetBS() << endl;
    cout << "inner loop bs " << GetMyLoop()->GetBS() << endl;
    
    throw;
  }
  const Size m = (*ms)[execNum];
  const Size n = (*ns)[execNum];
#if DODM
  const Size lm = (*lms)[execNum];
  const Size ln = (*lns)[execNum];
#endif
  m_msizes->AddRepeatedSizes(m, numIters, parFactor);
  m_nsizes->AddRepeatedSizes(n, numIters, parFactor);
#if DODM
  m_mlsizes->AddRepeatedSizes(lm, numIters, parFactor);
  m_nlsizes->AddRepeatedSizes(ln, numIters, parFactor);
#endif
}
#else
void LoopTunnel::AppendSizes(unsigned int execNum, unsigned int numIters, unsigned int parFactor)
{
  if (m_tunType != SETTUNIN)
    return;
  if (!m_pset->IsReal())
    return;

  if (!m_sizes)
    throw;
  const DLANode *input = (DLANode*)Input(0);
  ConnNum num = InputConnNum(0);
  Dim numDims = input->NumDims(num);
  for (Dim i = 0; i < numDims; ++i) {
    const Sizes *sizes = input->Len(num,i);
    if (!sizes) {
      cout << "!sizes\n";
      throw;
    }
    unsigned int length = sizes->NumSizes();
    if (length <= execNum) 
      {
	cout << sizes->NumSizes() << endl;
	throw;
      }
    const Size size = (*sizes)[execNum];
    m_sizes[i].AddRepeatedSizes(size, numIters, parFactor);
  }
  if (!numDims) {
    //scalar
    m_sizes[0].AddRepeatedSizes(1, numIters, parFactor);
  }
}
#endif

#if DOELEM
void LoopTunnel::UpdateLocalSizes()
{
  //already calculated in AppendSizes
}
#elif DOTENSORS
void LoopTunnel::UpdateLocalSizes()
{
  if (m_tunType != SETTUNIN)
    return;
  if (!m_pset->IsReal())
    return;

  if (!m_sizes)
    throw;

  const DLANode *input = (DLANode*)Input(0);
  ConnNum num = InputConnNum(0);
  Dim numDims = input->NumDims(num);
  for (Dim i = 0; i < numDims; ++i) {
    m_lsizes[i] = m_sizes[i];
    double coeff = input->LocalLen(num,i)->m_coeff;
    if (coeff)
      m_lsizes[i].SetCoeff(coeff);
  }
  if (!numDims) {
    //scalar
    *m_lsizes = *m_sizes;
    double coeff =input->LocalLen(num,0)->m_coeff;
    if (coeff)
      m_lsizes->SetCoeff(coeff);
  }
}
#endif

#if TWOD
void LoopTunnel::ClearDataTypeCache()
{
  if (!m_msizes) 
    return;
  delete m_msizes;
  m_msizes = NULL;
  delete m_nsizes;
  m_nsizes = NULL;
#if DODM
  delete m_mlsizes;
  m_mlsizes = NULL;
  delete m_nlsizes;
  m_nlsizes = NULL;
#endif
}
#else
void LoopTunnel::ClearDataTypeCache()
{
  if (!m_sizes) 
    return;
  delete [] m_sizes;
  m_sizes = NULL;
  delete [] m_lsizes;
  m_lsizes = NULL;
}
#endif

bool LoopTunnel::Overwrites(const Node *input, ConnNum num) const
{
  if (m_tunType == POSSTUNIN || m_tunType == SETTUNIN)
    return !IsConst();
  else
    return false;
}

bool LoopTunnel::KeepsInputVarLive(Node *input, ConnNum numIn, ConnNum &numOut) const
{
  if (m_inputs.empty())
    throw;
  if (Input(m_inputs.size()-1) == input && InputConnNum(m_inputs.size()-1) == numIn) {
    numOut = numIn;
    return true;
  }
  else {
    return false;
  }
}

string LoopTunnel::GetLoopLevel(int offset) const
{
  int level = 0;
  Poss *poss = m_poss;
  if (!poss)
    throw;
  BasePSet *set = poss->m_pset;
  while (set) {
    if (set->IsLoop()) {
      ++level;
    }
    poss = set->m_ownerPoss;
    if (!poss)
      return std::to_string((long long int) level+offset);
    set = poss->m_pset;
  }
  return std::to_string((long long int) level+offset);

}

void LoopTunnel::MigrateFromOldTun(Tunnel *tunIn)
{
  if (!tunIn->IsLoopTunnel())
    throw;
  LoopTunnel *tun = (LoopTunnel*)tunIn;
  Tunnel::MigrateFromOldTun(tun);
  if (m_tunType != tun->m_tunType)
    throw;
#if TWOD
  if (m_msizes)
    throw;
  m_msizes = tun->m_msizes;
  tun->m_msizes = NULL;
  m_nsizes = tun->m_nsizes;
  tun->m_nsizes = NULL;
#if DODM
  m_mlsizes = tun->m_mlsizes;
  tun->m_mlsizes = NULL;
  m_nlsizes = tun->m_nlsizes;
  tun->m_nlsizes = NULL;
#endif
#else
  if (m_sizes)
    throw;
  m_sizes = tun->m_sizes;
  tun->m_sizes = NULL;
  m_lsizes = tun->m_lsizes;
  tun->m_lsizes = NULL;
#endif
}
