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



#include "transform.h"
#include <sstream>
#include <time.h>
#include "helperNodes.h"
#include "realLoop.h"
#include "critSect.h"
#include <iomanip>
#include "graphIter.h"

//Print out code for all generated implementations
// This takes a while for large search spaces
#if DOLLDLA
#define PRINTCODE
#endif
//#define PRINTCODE

#define OUTPUTCODEATEACHITER 0

//Save generated implementations to disk
// so they can be loaded on a separate run
//#define SAVETODISK


static unsigned int CURRENTSAVEVERSION = 1;
unsigned int CurrPhase = -1;

TransMap Universe::M_trans[NUMPHASES];
TransMap Universe::M_simplifiers;
TransPtrMap Universe::M_transNames;
TransNameMap Universe::M_transPtrs;
unsigned int Universe::M_transCount[NUMPHASES+2];
ConsFuncMap Universe::M_consFuncMap;

Universe::Universe() {
  m_pset = NULL;
}

void Universe::Init(RealPSet *seed)
{
  m_pset = seed;
  m_pset->m_flags |= SETTOPLEVELFLAG;
  CurrPhase = FIRSTPHASE;
  m_pset->Simplify(M_simplifiers);
  m_pset->BuildDataTypeCache();

#if DOTENSORS
  CheckMaxDims();
#endif

#if DOLLDLA
  int pSize = seed->m_posses.size();
  cout << std::to_string((long long int) pSize) << endl;
  Poss *poss = seed->m_posses.begin()->second;
  NodeVecIter nodeIter = poss->m_possNodes.begin();
  for(; nodeIter != poss->m_possNodes.end(); ++nodeIter) {
    Node *node = *nodeIter;
    if (node->GetNodeClass() == InputNode::GetClass()) {
      InputNode *inNode = (InputNode*) node;
      m_declarationVectors.push_back(inNode->DataDeclaration());
      m_constantDefines.push_back(inNode->NumRowsDefine());
      m_constantDefines.push_back(inNode->NumColsDefine());
      m_constantDefines.push_back(inNode->RowStrideDefine());
      m_constantDefines.push_back(inNode->ColStrideDefine());
      m_argNames.push_back(inNode->GetName(0).str());
    }
  }
 
#endif
}

void Universe::Init(string fileName)
{
  LoadFromFile(fileName);
}

Universe::~Universe()
{
  NodeVecIter iter;
  TransPtrMapIter iter2 = M_transNames.begin();
  M_transPtrs.clear();
  for(; iter2 != M_transNames.end(); ++iter2) {
    delete iter2->first;
  }
  M_transNames.clear();
  TransMapIter iter3 = M_simplifiers.begin();
  for(; iter3 != M_simplifiers.end(); ++iter3)
    delete iter3->second;
  M_simplifiers.clear();
  for(int i = 0; i < NUMPHASES; ++i) {
    iter3 = M_trans[i].begin();
    for(; iter3 != M_trans[i].end(); ++iter3)
      delete iter3->second;
    M_trans[i].clear();
  }
  if (m_pset != NULL) {
    NodeVec in = m_pset->m_inTuns;
    NodeVec out = m_pset->m_outTuns;
    delete m_pset;
    iter = in.begin();
    for(; iter != in.end(); ++iter) {
      delete *iter;
    }
    in.clear();
    iter = out.begin();
    for(; iter != out.end(); ++iter) {
      delete *iter;
    }
    out.clear();
  }
}

#if DOTENSORS
void Universe::CheckMaxDims()
{
  Dim maxDims = 0;
  PossMMapIter iter = m_pset->m_posses.begin();
  for(; iter != m_pset->m_posses.end(); ++iter) {
    Poss *poss = (*iter).second;
    NodeVecIter iter2 = poss->m_possNodes.begin();
    for(; iter2 != poss->m_possNodes.end(); ++iter2) {
      Node *node = *iter2;
      if (node->GetNodeClass() == InputNode::GetClass()) {
	InputNode *in = (InputNode*)node;
	Dim numDims = in->NumDims(0);
	if (numDims > maxDims)
	  maxDims = numDims;
      }
    }
  }  
  if (maxDims != NUM_GRID_DIMS) {
    cout << "maxDims = " << maxDims << endl;
    cout << "NUM_GRID_DIMS = " << NUM_GRID_DIMS << endl;
    cout << "update it!\n";
    throw;
  }
}
#endif

bool Universe::TakeIter(unsigned int phase)
{
  bool newOne = false;

  cout << "\tStarting iteration\n";
  cout.flush();

  m_pset->BuildDataTypeCache();
  
  newOne = m_pset->TakeIter(M_trans[phase], M_simplifiers);

  cout << "\tFinishing iteration\n";
  cout.flush();

  return newOne;
}

GraphNum Universe::Expand(unsigned int numIters, unsigned int phase, CullFunction cullFunc)
{
  /*
  if (phase == ROPHASE || phase == SR1PHASE) {
    m_pset->FormSets(phase);
    cout << "Formed sets\n";
  }
  */
#if DOSOPHASE
  if (phase == SOPHASE) {
    m_pset->FormSets(phase);   
  }
#elif DOSUMSCATTERTENSORPHASE

  if (false && phase == SUMSCATTERTENSORPHASE) {
    m_pset->FormSets(phase);
    cout << "\t\tFormed set\n";
    cout << "\t\t\t" << TotalCount() << " impl's\n";
  }
  
#endif

  CurrPhase = phase;
  
  ClearFullyExpanded();

#if DOLLDLA
  if (!M_simplifiers.empty()) {
    if (CurrPhase == LLDLAPRIMPHASE) {
      m_pset->Simplify(M_simplifiers, true);
    }
  }
#elif DOFINALOPTPHASE
  if (!M_simplifiers.empty()) {
    if (CurrPhase == FINALOPTPHASE) {
      m_pset->Simplify(M_simplifiers, true);
    }
  }
#endif

  GraphNum count = 0;
  GraphNum prevAlgs = TotalCount();
  bool foundNew = true;
  while ( foundNew ) {
    time_t start, end;
    time(&start);
    foundNew = TakeIter(phase);

#if OUTPUTCODEATEACHITER
    stringstream str;
    GraphNum optGraph = 0;
    str << "codeOutput" << count << ".txt";
    ofstream out;
    out.open(str.str());
    IndStream codeOut(&out,LLDLASTREAM);
    Print(codeOut, optGraph);
    out.close();
#endif //OUTPUTCODEATEACHITER
    
    GraphNum total = TotalCount();
    ++count;
    time(&end);
    cout << "//Done iteration " << count << " with " 
	 << total << " algorithms";
    if (prevAlgs) {
      double percent =  100.0 * (total / prevAlgs - 1 );
      if (percent < 0)
	throw;
      cout << ";   increase of " << percent << "%";
    }
    else {
      if (m_pset->m_posses.empty())
	throw;
      cout << ";   0 algorithms, but there are still posses at the high level\n";
    }
    cout << ";   took " << difftime(end,start) << " seconds";
    cout << endl;
    cout.flush();
    if (!foundNew) {
      time(&start);
      Prop();
#if DOSOPHASE
      //In SOPHASE, we intentionally form sets
      // to separate pack operations so the
      // same pack buffer can be used for
      // different pieces of data, sequentially.
      //We don't want to now get rid of those sets
      if (phase < SOPHASE)
#elif DOSUMSCATTERTENSORPHASE
	if (phase == FUSEANDOPTTENSORPHASE)
#endif
	  foundNew = m_pset->MergePosses(M_simplifiers, cullFunc);
      time(&end);
      if(difftime(end,start) > 10) {
	cout << "Took " << difftime(end,start) << " seconds to merge\n";
	cout.flush();
      }
      if (foundNew) {
	total = TotalCount();
	cout << total << " algorithms now" << endl;
	cout << "\tProp'ing\n";
	Prop();
	cout << "\tDone Prop'ing\n";
	cout.flush();
      }
    }
    prevAlgs = total;
  }
  ++CurrPhase;

  cout << "Culling\n";
  cout.flush();
  time_t start, end;
  time(&start);
  Cull();
  time(&end);
  cout << "Done culling in " << difftime(end,start) << " seconds; left with " << TotalCount() << " impl's\n";
  cout.flush();

  return count;
}

void Universe::AddToMaps(Transformation *trans) 
{
  M_transNames.insert(pair<Transformation*,string>(trans,trans->GetType()));
  if (M_transPtrs.find(trans->GetType()) != M_transPtrs.end()) {
    cout << "duplicate trans name " << trans->GetType() << endl;
    cout.flush();
    throw;
  }
  M_transPtrs.insert(pair<string,Transformation*>(trans->GetType(),trans));
}

void Universe::AddTrans(const ClassType &classType, Transformation *trans, int phase)
{
  static bool hasInited = false;
  if (!hasInited) {
    for (unsigned int i = 0; i < NUMPHASES+2; ++i)
      M_transCount[i] = 0;
    hasInited = true;
  }
  if (trans->IsSingle())
    AddToMaps(trans);
  else if (trans->IsVarRef()) {
    VarTrans *var = (VarTrans*)trans;
    if (var->IsMultiRef()) {
      MultiTrans *multi = (MultiTrans*)trans;
      TransConstVecIter iter = multi->m_trans.begin();
      for(; iter != multi->m_trans.end(); ++iter)
	AddToMaps(const_cast<Transformation*>(*iter));
    }
    else
      AddToMaps(trans);
  }
  else
    throw;
  if (phase == SIMP) {
    if (trans->IsSingle()) 
      M_transCount[NUMPHASES]++;
    else 
      M_transCount[NUMPHASES] += ((MultiTrans*)trans)->NumTransformations();
    TransMapIter mapIter = M_simplifiers.find(classType);
    if (mapIter != M_simplifiers.end()) {
      mapIter->second->push_back(trans);
    }
    else {
      TransVec *vec = new TransVec;
      vec->push_back(trans);
      M_simplifiers[classType] = vec;
    }
  }
  else if (phase < 0 || phase >= NUMPHASES) 
    throw;
  else {
    if (trans->IsSingle()) 
      M_transCount[phase]++;
    else 
      M_transCount[phase] += ((MultiTrans*)trans)->NumTransformations();
    TransMapIter mapIter = M_trans[phase].find(classType);
    if (mapIter != M_trans[phase].end()) {
      mapIter->second->push_back(trans);
    }
    else {
      TransVec *vec = new TransVec;
      vec->push_back(trans);
      M_trans[phase][classType] = vec;
    }
  }
}

void Universe::Prop()
{
  m_pset->ClearBeforeProp();
  m_pset->Prop();
}

void Universe::Cull()
{
  m_pset->Cull(CurrPhase);
}

void Universe::PrintCosts(const ImplementationRuntimeMap &impTimes)
{
  time_t start,end;
  ofstream out;

#ifdef MATLAB
    out.open("totalCostOutput.m");
#else
    throw;
    out.open("totalCostOutput.r");
#endif
    IndStream costOut(&out, OTHERSTREAM);
    time(&start);

#ifdef MATLAB
    *costOut << "transMap = containers.Map('KeyType','uint64','ValueType','char');\n";
    TransPtrMapIter iter2 = M_transNames.begin();
    for(; iter2 != M_transNames.end(); ++iter2) {
      *costOut << "transMap(" << (size_t)(iter2->first) << ") = '" << iter2->second << "';\n";
    }
    
    *costOut << "cost = zeros(" << TotalCount() << ",1);\n";
    *costOut << "refs = cell( [" << TotalCount() << " 1]);\n";
#else
    *costOut << "cost = array(0,dim=c(" << TotalCount() << ",1));\n";
#endif
    
    GraphNum graphNum = 0;
    ++graphNum;
    PossMMapIter iter = m_pset->m_posses.begin();
    for(; iter != m_pset->m_posses.end(); ++iter) {
      Poss *poss = (*iter).second;

      bool keepGoing = true;
  
      GraphIter graphIter(poss);
  
      while (keepGoing) {
	TransVec transList;
	graphIter.GetCurrTransVec(transList);
	ImplementationRuntimeMapConstIter found = impTimes.find(graphNum);
	if (found == impTimes.end())
	  throw;
	Cost tot = (Cost)MinTime(found->second);
	
	*costOut << "cost(" << graphNum << ") = "
		 << setprecision(15) << tot << ";\n";
	
	*costOut << "refs(" << graphNum << ") = {[ ";
	TransVecConstIter iter = transList.begin();
	for(; iter != transList.end(); ++iter) {
	  *costOut << (size_t)(*iter) << " ";
	}
	*costOut << "]};\n";
	if (!(graphNum % 1000)) {
	  *costOut << "'loaded " << graphNum << "'\n";
	}
	++graphNum;
	keepGoing = !graphIter.Increment();
      }
    }
    
    *costOut << "numAlgs = " << TotalCount() << endl;
    
    out.close();
    time(&end);
    cout << "\tCost printing took " << difftime(end,start) << " seconds\n";
    cout << "Done printing\n";
    cout.flush();
}

void Universe::PrintAll(int algNum, GraphNum optGraph)
{
  time_t start,end;
  ofstream out;

  //  cout << "Printing costOutput.txt\n";
  //  out.open("costOutput.txt");
  //  Print(out, COST, 0);
  //  out.close();


  if (!optGraph) {
#ifdef MATLAB
    out.open("totalCostOutput.m");
#else
    out.open("totalCostOutput.r");
#endif
    IndStream costOut(&out, OTHERSTREAM);
    optGraph = 0;
    time(&start);
    EvalCosts(costOut, optGraph);
    out.close();
    time(&end);
    cout << "\tCost eval took " << difftime(end,start) << " seconds\n";
    cout << "Done printing\n";
    cout.flush();
  }


  if (optGraph > 0) {
    cout << "\tOptimal graph ( " << optGraph << " ) :\n";
#if DOELEM
    IndStream optOut(&cout,ELEMSTREAM);
#elif DOSQM || DOSM
    IndStream optOut(&cout,BLISSTREAM);
#elif DOTENSORS
    IndStream optOut(&cout,TENSORSTREAM);
#elif DOLLDLA
    IndStream optOut(&cout,LLDLASTREAM);
#endif
    Print(optOut, optGraph, false);
  }
  else {
    cout << "optGraph = " << optGraph << endl;
  }

#ifdef PRINTCODE
  cout << "Printing codeOutput.txt\n";
  cout.flush();
  out.open("codeOutput.txt");
#if DOELEM
    IndStream codeOut(&out,ELEMSTREAM);
#elif DOSQM || DOSM
    IndStream codeOut(&out,BLISSTREAM);
#elif DOTENSORS
    IndStream codeOut(&out,TENSORSTREAM);
#elif DOLLDLA
    IndStream codeOut(&out,LLDLASTREAM);
#endif
    optGraph = 0;
    time(&start);
    Print(codeOut, optGraph);
    out.close();
    time(&end);
    cout << "\tTook " << difftime(end,start) << " seconds\n";
#endif

#ifdef SAVETODISK
  if (algNum != 0) {
    cout << "Saving to disk\n";
    cout.flush();
    std::stringstream str;
    str << "GraphsOut" << algNum;
    time(&start);
    SaveToFile(str.str());
    time(&end);
    cout << "\tTook " << difftime(end,start) << " seconds\n";
  }
#endif
}

void Universe::PrintBest()
{
  time_t start,end;

  time(&start);
  Cost trash;
  GraphIter iter = EvalCostsAndSetBest(trash);
  time(&end);
  cout << "\tCost eval took " << difftime(end,start) << " seconds\n";
  cout.flush();
#if DOELEM
    IndStream optOut(&cout,ELEMSTREAM);
#elif DOSQM || DOSM
    IndStream optOut(&cout,BLISSTREAM);
#elif DOTENSORS
    IndStream optOut(&cout,TENSORSTREAM);
#elif DOLLDLA
    IndStream optOut(&cout,LLDLASTREAM);
#endif

    iter.PrintRoot(optOut, 0, true, m_pset);
}

void Universe::Print(IndStream &out, GraphNum &whichGraph, bool currOnly)
{
  PossMMapIter iter = m_pset->m_posses.begin();
  for(; iter != m_pset->m_posses.end(); ++iter) {
    Poss *poss = (*iter).second;
    GraphIter graphIter(poss);
    graphIter.ClearPrintedRecursively();
    graphIter.PrintRoot(out, whichGraph, currOnly, m_pset);
  }

  *out << "// numAlgs = " << TotalCount() << endl;
}

void Universe::EvalCosts(IndStream &out, GraphNum &whichGraph)
{
  GraphNum optGraph;
  double optCost;

#ifdef MATLAB
  *out << "transMap = containers.Map('KeyType','uint64','ValueType','char');\n";
  TransPtrMapIter iter2 = M_transNames.begin();
  for(; iter2 != M_transNames.end(); ++iter2) {
    *out << "transMap(" << (size_t)(iter2->first) << ") = '" << iter2->second << "';\n";
  }

  *out << "cost = zeros(" << TotalCount() << ",1);\n";
  *out << "refs = cell( [" << TotalCount() << " 1]);\n";
#else
  *out << "cost = array(0,dim=c(" << TotalCount() << ",1));\n";
#endif

  Prop();
  optCost = -1;
  GraphNum graphNum = 0;
  ++graphNum;
  PossMMapIter iter = m_pset->m_posses.begin();
  for(; iter != m_pset->m_posses.end(); ++iter) {
    Poss *poss = (*iter).second;
    GraphIter graphIter(poss);
    graphIter.EvalRoot(out, graphNum, whichGraph, optGraph, optCost);
  }
    
  cout << "Opt is graph " << optGraph << endl;
  cout << "\t\t " << optCost << endl;
  cout.flush();
    
  whichGraph = optGraph;
  
  *out << "numAlgs = " << TotalCount() << endl;
}

GraphIter Universe::EvalCostsAndSetBest(Cost &best)
{
  Prop();
  PossMMapIter iter = m_pset->m_posses.begin();
  GraphIter graphIter(iter->second);
  best = graphIter.EvalAndSetBest();
  ++iter;
  for(; iter != m_pset->m_posses.end(); ++iter) {
    GraphIter compIter(iter->second);
    Cost cost = compIter.EvalAndSetBest();
    if (cost < best) {
      best = cost;
      graphIter = compIter;
    }
  }
  return graphIter;
}

GraphNum Universe::TotalCount() const
{
  return m_pset->TotalCount();
}

void Universe::ClearFullyExpanded()
{
  m_pset->ClearFullyExpanded();
}

void Universe::PrintStats()
{
  cout << "The universe has\n";
  for (unsigned int i = 0; i < NUMPHASES; ++i)
    cout << "\t" << M_transCount[i] << " in phase " << i << endl;
  cout << "\t" << M_transCount[NUMPHASES] << " simplifiers\n";
}

void Universe::RegCons(ClassType type, ConstructorFunc func)
{
  if (M_consFuncMap.find(type) != M_consFuncMap.end()) {
    cout << "dup constructor for " << type << endl;
    throw;
  }
  M_consFuncMap[type] = func;
}

Node* Universe::GetBlankClassInst(ClassType type)
{
  ConsFuncMapIter iter = M_consFuncMap.find(type);
  if (iter == M_consFuncMap.end()) {
    cout << "didn't find node type " << type << endl;
    throw;
  }
  ConstructorFunc func = iter->second;
  return func();
}

void Universe::SaveToFile(string fileName) const
{
  ofstream out;
  out.open(fileName.c_str());//, ios::binary);
  Flatten(out);
  out.close();
}

void Universe::Flatten(ofstream &out) const
{
  WRITE(CURRENTSAVEVERSION);
  unsigned int tmp = M_transNames.size();
  WRITE(tmp);
  TransPtrMapConstIter iter = M_transNames.begin();
  for(; iter != M_transNames.end(); ++iter) {
    WRITE((*iter).first);
    out << (*iter).second << endl;
    //    WRITE((*iter).second);
  }
  WRITE(CurrPhase);
  Poss::FlattenStatic(out);
  RealLoop::FlattenStatic(out);
  WRITE(m_pset);
  bool isLoop = m_pset->IsLoop();
  WRITE(isLoop);
#if DOBLIS
  bool isCrit = m_pset->IsCritSect();
  WRITE(isCrit);
#endif
  m_pset->Flatten(out);
  WRITE(END);
}

void Universe::LoadFromFile(string fileName)
{
  ifstream in;
  in.open(fileName.c_str(), ios::binary);
  Unflatten(in);
  in.close();
}

void Universe::Unflatten(ifstream &in) 
{
  unsigned int version;
  READ(version);
  if (version != CURRENTSAVEVERSION) {
    cout << version << " vs. " 
	 << CURRENTSAVEVERSION << endl;
    throw;
  }
  unsigned int numTrans;
  READ(numTrans);
  PtrMap transMap;
  for(unsigned int i = 0; i < numTrans; ++i) {
    void *old;
    READ(old);
    string name;
    getline(in, name);
    TransNameMapIter iter = M_transPtrs.find(name);
    if (iter == M_transPtrs.end()) {
      cout << "Missing transformation "
	   << name << endl;
      throw;
    }
    transMap[old] = (*iter).second;
  }
  READ(CurrPhase);
  Poss::UnflattenStatic(in);
  RealLoop::UnflattenStatic(in);
  PtrMap psetMap;
  BasePSet *oldPset;
  READ(oldPset);
  bool isLoop;
  READ(isLoop);  
  bool isCrit;
  READ(isCrit);
  if (isLoop)
    m_pset = new RealLoop;
  else
    m_pset = new RealPSet;
  psetMap[oldPset] = m_pset;

  PtrMap possMap;
  NodeMap nodeMap;
  SaveInfo info;
  info.transMap = &transMap;
  info.possMap = &possMap;
  info.psetMap = &psetMap;
  info.nodeMap = &nodeMap;
  
  m_pset->Unflatten(in, info);
  char tmp;
  READ(tmp);
  if (tmp != END) {
    cout << "Bad end!\n";
    throw;
  }
}

void Universe::CullWorstPerformers(double percentToCull, int ignoreThreshold)
{
  m_pset->CullWorstPerformers(percentToCull, ignoreThreshold);
}


void Universe::CullAllBut(int num)
{
  m_pset->CullAllBut(num);
}

void Universe::InlineAllSets()
{
  m_pset->InlineAllSets();
}
