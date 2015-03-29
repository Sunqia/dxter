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

#include "benchmarkMenu.h"
#include "blasExamples.h"
#include "costModel.h"
#include "driverUtils.h"
#include "miscellaneousExamples.h"
#include "multiBLASExamples.h"
#include "nodeTestExamples.h"
#include "problemRunner.h"
#include "singleOperationExamples.h"
#include "uniqueNameSource.h"
#include "testSuites.h"

#if DOLLDLA

#define DOEMPIRICALEVAL 1
#define PRINTCOSTS 1

#define DOCOMPACTLOOPUNROLLING 0
#define DO2MUTRANSFORMATIONS 1
#define DO3MUTRANSFORMATIONS 1
#define DO16MUTRANSFORMATIONS 1
#define DOLARGEMUTRANSFORMATIONS 0

#define DOPARTIALLOOPUNROLLING 1
#define PARTIALUNROLLINGSTARTCOEF 2
#define PARTIALUNROLLINGENDCOEF 16

#if DOCOMPACTLOOPUNROLLING + DOPARTIALLOOPUNROLLING > 1
do you really want to do compact unrolling and partial unrolling?
#endif

Trans transA, transB;

Architecture* arch;
UniqueNameSource* localInputNames;
CostModel* costModel;


void Usage()
{
  cout <<"\n\nWelcome to DxTer LLDLA! Please choose an option below:\n\n";
  cout << "./driver arg1 arg2 ...\n";
  cout <<"\n";
  cout <<"arg1 == 0   -> View benchmarks\n";
  cout <<"\n";
  cout <<"Single Operation Examples\n";
  cout <<"         3  -> Dot prod F/D M\n";
  cout <<"         4  -> Matrix add F/D M N\n";
  cout <<"         5  -> Matrix vector multiply N/T F/D M N\n";
  cout <<"         6  -> Scalar vector multiply C/R F/D M\n";
  cout <<"         7  -> Vector matrix multiply F/D M N\n";
  cout <<"         8  -> Scalar matrix multiply F/D M N\n";
  cout <<"         9  -> Vector add C/R F/D M\n";
  cout <<"        15  -> Gen Size Col Vector SVMul F/D M\n";
  cout <<"\n";
  cout <<"BLAS Examples\n";
  cout <<"         1  -> Gemm  N/T N/T F/D M N P\n";
  cout <<"        14  -> Gemv N/T F/D M N\n";
  cout <<"        16  -> Axpy C/R F/D M\n";
  cout <<"\n";
  cout <<"Miscellaneous Examples\n";
  cout <<"         2  -> Double Gemm  N/T N/T F/D M N P K\n";
  cout <<"        10  -> Vector add twice F/D M\n";
  cout <<"        11  -> Vector matrix vector multiply F/D M N\n";
  cout <<"        12  -> Matrix add twice F/D M N\n";
  cout <<"        13  -> Matrix vector multiply twice F/D M N P\n";
  cout <<"        17  -> alpha*(A0 + A1)^T*B + beta*C F/D M N P\n";
  cout <<"        18  -> alpha*A*x + beta*B*x F/D M N\n";
  cout <<"        31  -> y = alpha*x + beta*(z + y) F/D M\n";
  cout <<"\n";
  cout <<"Node test examples\n";
  cout <<"        19  -> Set to zero test (y <- Ax) F/D M N\n";
  cout <<"        20  -> Pack test F/D M\n";
  cout <<"        21  -> Copy test F/D M N\n";
  cout <<"        22  -> Vertical partition recombine test F/D M\n";
  cout <<"        23  -> Horizontal partition recombine test F/D M\n";
  cout <<"        24  -> Vertical refined pack test F/D M\n";
  cout <<"        25  -> Vertical pack unpack test F/D M\n";
  cout <<"        26  -> 2D vertical pack unpack test F/D M N\n";
  cout <<"        27  -> 2D vertical unpack test F/D M N\n";
  cout <<"        28  -> 2D horizontal unpack test F/D M N\n";
  cout <<"        29  -> 2D horizontal copy test F/D M N\n";
  cout <<"\n";
  cout <<"Automated tests\n";
  cout <<"        30  -> Basic examples, no runtime evaluation\n";
  cout <<"\n";
}

void SetUpGlobalState() {
  arch = new HaswellMacbook();
  costModel = new BasicCostModel();
  localInputNames = new UniqueNameSource("u_local_input_");
}

int main(int argc, const char* argv[])
{

#ifdef _OPENMP
  omp_set_num_threads(1);
  omp_set_nested(true);
#endif

  int m, n, p, k;
  Type precision;
  VecType vecType;
  ProblemInstance problemInstance;

  SetUpGlobalState();

  RealPSet* algPSet;
  int algNum;
  string opName;

  if (argc == 2 && *argv[1] == '0') {
    BenchmarkMenu();
  } else if(argc < 2) {
    Usage();
    return 0;
  } else {
    algNum = atoi(argv[1]);
    switch(algNum) {
    case(1):
      if (argc != 8) {
	Usage();
	return 0;
      }
      opName = "dxt_gemm";
      transA = CharToTrans(*argv[2]);
      transB = CharToTrans(*argv[3]);
      precision = CharToType(*argv[4]);
      m = atoi(argv[5]);
      n = atoi(argv[6]);
      p = atoi(argv[7]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      problemInstance.AddDimension(p, "p");
      algPSet = GemmTest(precision, transA, transB, m, n, p);
      break;
    case(2):
      if (argc != 9) {
	Usage();
	return 0;
      }
      opName = "dxt_double_gemm";
      precision = CharToType(*argv[4]);
      m = atoi(argv[5]);
      n = atoi(argv[6]);
      p = atoi(argv[7]);
      k = atoi(argv[8]);
      transA = CharToTrans(*argv[2]);
      transB = CharToTrans(*argv[3]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      problemInstance.AddDimension(p, "p");
      problemInstance.AddDimension(k, "k");
      algPSet = DoubleGemm(precision, transA, transB, m, n, p, k);
      break;
    case(3):
      if (argc != 4) {
	Usage();
	return 0;
      }
      opName = "dxt_dot";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      problemInstance.AddDimension(m, "m");
      algPSet = DotTest(precision, m);
      break;
    case(4):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_madd";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = MAddTest(precision, m, n);
      break;
    case(5):
      if (argc != 6) {
	Usage();
	return 0;
      }
      opName = "dxt_mvmul";
      precision = CharToType(*argv[3]);
      m = atoi(argv[4]);
      n = atoi(argv[5]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      if (TRANS == CharToTrans(*argv[2])) {
	algPSet = MVMulTest(precision, true, m, n);
      } else {
	algPSet = MVMulTest(precision, false, m, n);
      }
      break;
    case(6):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_sv_mul";
      vecType = CharToVecType(*argv[2]);
      precision = CharToType(*argv[3]);
      m = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      algPSet = SVMulTest(precision, vecType, m);
      break;
    case(7):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_vmmul";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = VMMulTest(precision, m, n);
      break;
    case(8):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_smmul";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = SMMulTest(precision, m, n);
      break;
    case(9):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_vadd";
      vecType = CharToVecType(*argv[2]);
      precision = CharToType(*argv[3]);
      m = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      algPSet = VAddTest(precision, vecType, m);
      break;
    case(10):
      if (argc != 4) {
	Usage();
	return 0;
      }
      opName = "dxt_vadd2";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      problemInstance.AddDimension(m, "m");
      algPSet = VAdd2(precision, m);
      break;
    case(11):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_vmvmul";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = VMVMul(precision, m, n);
      break;
    case(12):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_madd2";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = MAdd2(precision, m, n);
      break;
    case(13):
      if (argc != 6) {
	Usage();
	return 0;
      }
      opName = "dxt_mvmul2";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      p = atoi(argv[5]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      problemInstance.AddDimension(p, "p");
      algPSet = MVMul2(precision, m, n, p);
      break;
    case(14):
      if (argc != 6) {
	Usage();
	return 0;
      }
      opName = "dxt_gemv";
      precision = CharToType(*argv[3]);
      m = atoi(argv[4]);
      n = atoi(argv[5]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      if (TRANS == CharToTrans(*argv[2])) {
	algPSet = Gemv(precision, true, m, n);
      } else {
	algPSet = Gemv(precision, false, m, n);
      }
      break;
    case(15):
      if (argc != 4) {
	Usage();
	return 0;
      }
      opName = "dxt_sv_col_mul_gen";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      problemInstance.AddDimension(m, "m");
      algPSet = GenSizeColSVMul(precision, m);
      break;
    case(16):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_saxpy";
      vecType = CharToVecType(*argv[2]);
      precision = CharToType(*argv[3]);
      m = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      algPSet = Axpy(precision, vecType, m);
      break;
    case(17):
      if (argc != 6) {
	Usage();
	return 0;
      }
      opName = "dxt_sgemam";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      p = atoi(argv[5]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      problemInstance.AddDimension(p, "p");
      algPSet = Gemam(precision, m, n, p);
      break;
    case(18):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_sgemam";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = Gesummv(precision, m, n);
      break;
    case(19):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_zeroMVMul";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = SetToZeroTest(precision, m, n);
      break;
    case(20):
      if (argc != 4) {
	Usage();
	return 0;
      }
      opName = "dxt_pack_test";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      problemInstance.AddDimension(m, "m");
      algPSet = PackTest(precision, m);
      break;
    case(21):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_copy_test";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = CopyTest(precision, m, n);
      break;
    case(22):
      if (argc != 4) {
	Usage();
	return 0;
      }
      opName = "dxt_vertical_partition_recombine_test";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      problemInstance.AddDimension(m, "m");
      algPSet = VerticalPartitionRecombineTest(precision, m);
      break;
    case(23):
      if (argc != 4) {
	Usage();
	return 0;
      }
      opName = "dxt_horizontal_partition_recombine_test";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      problemInstance.AddDimension(m, "m");
      algPSet = HorizontalPartitionRecombineTest(precision, m);
      break;
    case(24):
      if (argc != 4) {
	Usage();
	return 0;
      }
      opName = "dxt_vertical_refined_pack_test";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      problemInstance.AddDimension(m, "m");
      algPSet = VerticalRefinedPackTest(precision, m);
      break;
    case(25):
      if (argc != 4) {
	Usage();
	return 0;
      }
      opName = "dxt_vertical_pack_unpack_test";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      problemInstance.AddDimension(m, "m");
      algPSet = VerticalPackUnpackTest(precision, m);
      break;
    case(26):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_vertical_2D_pack_unpack_test";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = TwoDVerticalPackUnpackTest(precision, m, n);
      break;
    case(27):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_vertical_2D_unpack_test";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = TwoDVerticalUnpackTest(precision, m, n);
      break;
    case(28):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_horizontal_2D_unpack_test";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = TwoDHorizontalUnpackTest(precision, m, n);
      break;
    case(29):
      if (argc != 5) {
	Usage();
	return 0;
      }
      opName = "dxt_horizontal_2D_copy_test";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      n = atoi(argv[4]);
      problemInstance.AddDimension(m, "m");
      problemInstance.AddDimension(n, "n");
      algPSet = HorizontalCopyTest(precision, m, n);
      break;
    case(30):
      if (argc != 2) {
	Usage();
	return 0;
      }
      BasicNoRuntimeEvalTests();
      return 0;
    case(31):
      if (argc != 4) {
	Usage();
	return 0;
      }
      opName = "dxt_vmuladd_benchmark";
      precision = CharToType(*argv[2]);
      m = atoi(argv[3]);
      problemInstance.AddDimension(m, "m");
      algPSet = VMulAddBenchmark(precision, m);
      break;
    default:
      Usage();
      return 0;
    }

    problemInstance.SetType(precision);
    problemInstance.SetName(opName);
    RunProblemWithRTE(algNum, algPSet, &problemInstance);

  }
  return 0;
}

#endif //DOLLDLA
