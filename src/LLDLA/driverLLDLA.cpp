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

#include "base.h"
#include "costs.h"
#ifdef _OPENMP
#include "omp.h"
#endif
#include "transform.h"
#include "loopSupport.h"
#include <time.h>
#include "DLAReg.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#include "LLDLAGemm.h"

#if DOLLDLA

#define DOEMPIRICALEVAL 1

#include <sstream>

#include "driverUtils.h"
#include "debug.h"
#include "LLDLAGemmTransformations.h"
#include "smmul.h"
#include "runtimeEvaluation.h"

Size one = 1;
Size smallSize = 10;
Size medSize = 100;
Size bigSize = 1000;
//Size bs = ELEM_BS;

PSet* GemmExample();
PSet* DoubleGemmExample();

Trans transA, transB;

ImplementationMap ImpStrMap(Universe *uni)
{
  ImplementationMap impMap;
  unsigned int i;
  for (i = 1; i <= uni->TotalCount(); i++) {
    std::stringbuf sbuf;
    std::ostream out(&sbuf);
    IndStream istream = IndStream(&out, LLDLASTREAM);
    uni->Print(istream, i);
    impMap.insert(NumImplementationPair(i, sbuf.str()));    
  }
  return impMap;
}

void PrintImpMap(std::map<unsigned int, vector<double>> impTimes)
{
  std::map<unsigned int, vector<double>>::iterator mit;
  for (mit = impTimes.begin(); mit != impTimes.end(); ++mit) {
    std::vector<double>::iterator vit;
    cout << "IMPLEMENTATION # " << std::to_string(mit->first) << endl;
    for (vit = mit->second.begin(); vit != mit->second.end(); ++vit) {
      cout << std::to_string(*vit) << endl;
    }
    cout << endl;
  }
}

unsigned int PrintImpMapInFlops(std::map<unsigned int, vector<double>> impTimes, double flopCost, int chunkSize) {
  /***************************************************************************
   * WARNING: These numbers are processor specific to Dillon's machine in GDC
   ***************************************************************************/
  double ticksPerSec = 1.0e6;
  double peakFLOPS = 30e9;
  unsigned int bestImpNum = 0;
  double bestFLOPS = 0;
  std::map<unsigned int, vector<double>>::iterator mit;
  for (mit = impTimes.begin(); mit != impTimes.end(); ++mit) {
    std::vector<double>::iterator vit;
    cout << "IMPLEMENTATION # " << std::to_string(mit->first) << endl;
    for (vit = mit->second.begin(); vit != mit->second.end(); ++vit) {
      double totalFlops = flopCost * chunkSize;
      double totalTimeInSecs = *vit / ticksPerSec;
      double actualFLOPS = totalFlops / totalTimeInSecs;
      double pctPeak = (actualFLOPS / peakFLOPS) * 100;
      if (actualFLOPS > bestFLOPS) {
	bestFLOPS = actualFLOPS;
	bestImpNum = (*mit).first;
      }
      cout << "FLOPS = " << std::to_string(actualFLOPS) << "\t%Peak = " << std::to_string(pctPeak) << endl;
    }
    cout << endl;
  }
  return bestImpNum;
}

void AddTrans()
{
  //Introduces loops in the m, n, and k dimensions, respectively
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMM, USELLDLAMU), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMN, USELLDLAMU), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMK, USELLDLAMU), LLDLALOOPPHASE);

#if 0
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMM, USELLDLA2MU), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMN, USELLDLA2MU), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMK, USELLDLA2MU), LLDLALOOPPHASE);

  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMM, USELLDLA3MU), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMN, USELLDLA3MU), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMK, USELLDLA3MU), LLDLALOOPPHASE);
#endif


  //Introduces loops in the m and n dimension for SMMul
  Universe::AddTrans(SMMul::GetClass(), new SMulLoopRef(ABSLAYER, ABSLAYER, DIMM, USELLDLAMU), LLDLALOOPPHASE);
  Universe::AddTrans(SMMul::GetClass(), new SMulLoopRef(ABSLAYER, ABSLAYER, DIMN, USELLDLAMU), LLDLALOOPPHASE);

  //Lowers the layer tag of a Gemm node that is LLDLA_MU in all three dimensions
  Universe::AddTrans(Gemm::GetClass(), new LLDAGemmLowerLayer(ABSLAYER, LLDLAMIDLAYER, LLDLA_MU), LLDLALOOPPHASE);


  //Lowers the layer tag of a SMMul node that is LLDLA_MU in both dimensions
  Universe::AddTrans(SMMul::GetClass(), new SMulLowerLayer(ABSLAYER, LLDLAMIDLAYER, LLDLA_MU), LLDLALOOPPHASE);


  //Changes Gemm with transposition to non-transposed version use Transpose nodes
  Universe::AddTrans(Gemm::GetClass(), new GemmTransToNotTrans(LLDLAMIDLAYER), LLDLAPRIMPHASE);

  //Replaces a Gemm node with a LLDLAGemm node
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmToPrim(LLDLAMIDLAYER, LLDLAPRIMITIVELAYER), LLDLAPRIMPHASE);

  //Lowers the layer tag of a SMMul node that is LLDLA_MU in both dimensions
  Universe::AddTrans(SMMul::GetClass(), new SMulLowerLayer(LLDLAMIDLAYER, LLDLAPRIMITIVELAYER, LLDLA_MU), LLDLAPRIMPHASE);
}

void AddSimplifiers()
{ 
}

void Usage()
{
  cout << "./driver arg1 arg2 ...\n";
  cout <<" arg1 == 0  -> Load from file arg1\n";
  cout <<"         1  -> Gemm Example N/T N/T\n";
  cout <<"         2  -> Double Gemm Example N/T N/T\n";
}

int main(int argc, const char* argv[])
{
#ifdef _OPENMP
  omp_set_num_threads(1);
  omp_set_nested(true);
#endif
  //  PrintType printType = CODE;
  int numIters = -1;
  PSet* (*algFunc)();
  //  unsigned int whichGraph = 0;
  int algNum;
  string fileName;

  if(argc < 2) {
    Usage();
    return 0;
  }
  else {
    algNum = atoi(argv[1]);
    switch(algNum) {
    case(1):
      if (argc != 4) {
	Usage();
	return 0;
      }
      algFunc = GemmExample;
      transA = CharToTrans(*argv[2]);
      transB = CharToTrans(*argv[3]);
      break;
    case(2):
      if (argc != 4) {
	Usage();
	return 0;
      }
      algFunc = DoubleGemmExample;
      transA = CharToTrans(*argv[2]);
      transB = CharToTrans(*argv[3]);
      break;
    default:
      Usage();
      return 0;
    }
  }

  RegAllLLDLANodes();
  AddTrans();
  AddSimplifiers();

  Universe uni;
  time_t start, start2, end;
  uni.PrintStats();
  Cost flopCost;
  if (algNum==0) {
    time(&start);
    uni.Init(fileName);
    time(&end);
    cout << "Unflatten took " << difftime(end,start) << " seconds\n";
    cout << "Propagating\n";
    cout.flush();
    time(&start2);
    uni.Prop();
    time(&end);
    cout << "Propagation took " << difftime(end,start2) << " seconds\n";
  }
  else {
    PSet *startSet = algFunc();
    uni.Init(startSet);
    uni.Prop();
    flopCost = startSet->EvalAndSetBest();
    cout << "Flops for operation = " << std::to_string(flopCost) << endl;
    time(&start);
  }


#if DOLLDLALOOPPHASE
  if (CurrPhase == LLDLALOOPPHASE) {
    cout << "Expanding LL DLA loop phase\n";
    uni.Expand(-1, LLDLALOOPPHASE, LLDLACull);
    time(&end);
    cout << "LLDLALOOP phase took " << difftime(end,start) << " seconds\n";
    cout << "Propagating\n";
    cout.flush();
    time(&start2);
    uni.Prop();
    time(&end);
    cout << "Propagation took " << difftime(end,start2) << " seconds\n";
  }
#endif

#if DOLLDLAPRIMPHASE
  if (CurrPhase == LLDLAPRIMPHASE) {
    cout << "Expanding LL DLA prim phase\n";
    cout << "Starting with " << uni.TotalCount() << endl;
    time(&start2);
    uni.Expand(numIters, LLDLAPRIMPHASE, LLDLACull);
    time(&end);
    cout << "LLDLAPRIM phase took " << difftime(end,start2) << " seconds\n";
    
    cout << "Propagating\n";
    cout.flush();
    time(&start2);
    uni.Prop();
    time(&end);
    cout << "Propagation took " << difftime(end,start2) << " seconds\n";
  }
#endif


  cout << "Full expansion took " << difftime(end,start) << " seconds\n";
  cout.flush();

#if DOEMPIRICALEVAL  
  cout << "Writing all implementations to runtime eval files\n";

  int chunkSize = 500;
  int numIterations = 3;
  RuntimeTest rtest("dxt_gemm", uni.m_argNames, uni.m_declarationVectors, uni.m_constantDefines, numIterations, chunkSize);
  string evalDirName = "runtimeEvaluation";
  RuntimeEvaluator evaler = RuntimeEvaluator(evalDirName);
  cout << "About to evaluate\n";
  std::map<unsigned int, vector<double>> impMap = evaler.EvaluateImplementations(rtest, ImpStrMap(&uni));
  cout << "Done evaluating\n";
  unsigned int best = PrintImpMapInFlops(impMap, flopCost, chunkSize);

#endif //DOEMPIRICALEVAL

#if 1
  uni.PrintAll(best);
#else
  uni.PrintBest();
#endif

  /*  if (whichGraph <= 0)
      uni.PrintAll();
      else
      uni.Print(cout, CODE, whichGraph); */

  return 0;
}

PSet* GemmExample()
{
  InputNode *Ain = new InputNode("A input", bigSize, 2, "A", 
				 bigSize, 1,
				 "ANumRows","ANumCols",
				 "ARowStride","AColStride");
  InputNode *Bin = new InputNode("B input", 2, bigSize, "B", 
				 2, 1,
				 "BNumRows","BNumCols",
				 "BRowStride","BColStride");
  InputNode *Cin = new InputNode("C input",  bigSize, bigSize, "C", 
				 bigSize, 1,
				 "CNumRows","CNumCols",
				 "CRowStride","CColStride");

  PossTunnel *tunA = new PossTunnel(POSSTUNIN);
  tunA->AddInput(Ain,0);

  PossTunnel *tunB = new PossTunnel(POSSTUNIN);
  tunB->AddInput(Bin,0);

  PossTunnel *tunC = new PossTunnel(POSSTUNIN);
  tunC->AddInput(Cin,0);

  Gemm *gemm = new Gemm(ABSLAYER, transA, transB, COEFONE, COEFONE, REAL);
  gemm->AddInputs(6,
		  tunA,0,
		  tunB,0,
		  tunC,0);

  Poss *innerPoss = new Poss(gemm,true);
  PSet *innerSet = new PSet(innerPoss);

  OutputNode *Cout = new OutputNode("C output");
  Cout->AddInput(innerSet->OutTun(0),0);

  Poss *outerPoss = new Poss(Cout,true);
  PSet *outerSet = new PSet(outerPoss);
  
  return outerSet;
}

PSet* DoubleGemmExample()
{
  InputNode *Ain = new InputNode("A input",  smallSize, smallSize, "A", 
				 smallSize, 1,
				 "ANumRows","ANumCols",
				 "ARowStride","AColStride");
  InputNode *Bin = new InputNode("B input", smallSize, smallSize, "B", 
				 smallSize, 1,
				 "BNumRows","BNumCols",
				 "BRowStride","BColStride");
  InputNode *Cin = new InputNode("C input",  smallSize, smallSize, "C", 
				 smallSize, 1,
				 "CNumRows","CNumCols",
				 "CRowStride","CColStride");

  PossTunnel *tunA = new PossTunnel(POSSTUNIN);
  tunA->AddInput(Ain,0);

  PossTunnel *tunB = new PossTunnel(POSSTUNIN);
  tunB->AddInput(Bin,0);

  PossTunnel *tunC = new PossTunnel(POSSTUNIN);
  tunC->AddInput(Cin,0);

  Gemm *gemm1 = new Gemm(ABSLAYER, transA, transB, COEFONE, COEFONE, REAL);
  gemm1->AddInputs(6,
		  tunA,0,
		  tunB,0,
		  tunC,0);

  Gemm *gemm2 = new Gemm(ABSLAYER, transA, transB, COEFONE, COEFONE, REAL);
  gemm2->AddInputs(6,
		  tunA,0,
		  tunB,0,
		  gemm1,0);

  Poss *innerPoss = new Poss(gemm2,true);
  PSet *innerSet = new PSet(innerPoss);

  OutputNode *Cout = new OutputNode("C output");
  Cout->AddInput(innerSet->OutTun(0),0);

  Poss *outerPoss = new Poss(Cout,true);
  PSet *outerSet = new PSet(outerPoss);
  
  return outerSet;
}


#endif //DOLLDLA
