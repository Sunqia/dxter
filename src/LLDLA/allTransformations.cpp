#include "allTransformations.h"

#if DOLLDLA

#include "LLDLAGemm.h"
#include "LLDLAGemmTransformations.h"
#include "LLDLATranspose.h"
#include "madd.h"
#include "mvmul.h"
#include "partition.h"
#include "recombine.h"
#include "setToZero.h"
#include "smmul.h"
#include "svmul.h"
#include "vmmul.h"
#include "vadd.h"
#include "vvdot.h"

void AddGemmTrans()
{
    // Convert gemm into loop over mvmul
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmToMVMul(ABSLAYER, ABSLAYER), LLDLALOOPPHASE);

  // Transform gemm into loop over vmmuls
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmToVMMul(ABSLAYER, ABSLAYER), LLDLALOOPPHASE);

  //Introduces loops in the m, n, and k dimensions, respectively
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMM, LLDLAMuSingle, REAL_SINGLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMN, LLDLAMuSingle, REAL_SINGLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMK, LLDLAMuSingle, REAL_SINGLE), LLDLALOOPPHASE);

#if DOCOMPACTLOOPUNROLLING
#if DO2MUTRANSFORMATIONS
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMM, LLDLA2MuSingle, REAL_SINGLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMN, LLDLA2MuSingle, REAL_SINGLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMK, LLDLA2MuSingle, REAL_SINGLE), LLDLALOOPPHASE);
#endif

#if DO3MUTRANSFORMATIONS
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMM, LLDLA3MuSingle, REAL_SINGLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMN, LLDLA3MuSingle, REAL_SINGLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMK, LLDLA3MuSingle, REAL_SINGLE), LLDLALOOPPHASE);
#endif
#endif // DOCOMPACTLOOPUNROLLING


#if DOCOMPACTLOOPUNROLLING
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMM, LLDLAMuSingle, REAL_DOUBLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMN, LLDLAMuSingle, REAL_DOUBLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMK, LLDLAMuSingle, REAL_DOUBLE), LLDLALOOPPHASE);

#if DO2MUTRANSFORMATIONS
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMM, LLDLA2MuSingle, REAL_DOUBLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMN, LLDLA2MuSingle, REAL_DOUBLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMK, LLDLA2MuSingle, REAL_DOUBLE), LLDLALOOPPHASE);
#endif

#if DO3MUTRANSFORMATIONS
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMM, LLDLA3MuSingle, REAL_DOUBLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMN, LLDLA3MuSingle, REAL_DOUBLE), LLDLALOOPPHASE);
  Universe::AddTrans(Gemm::GetClass(), new LLDLAGemmLoopExp(ABSLAYER, ABSLAYER, DIMK, LLDLA3MuSingle, REAL_DOUBLE), LLDLALOOPPHASE);
#endif

#endif // DOCOMPACTLOOPUNROLLING

  return;
}

void AddVVDotTrans()
{
  Universe::AddTrans(VVDot::GetClass(), new VVDotToRegArith(ABSLAYER, ABSLAYER), LLDLALOOPPHASE);

  return;
}

void AddMAddTrans()
{
  Universe::AddTrans(MAdd::GetClass(), new MAddLoopRef(ABSLAYER, ABSLAYER, DIMM, LLDLAMuSingle), LLDLALOOPPHASE);

  Universe::AddTrans(MAdd::GetClass(), new MAddLoopRef(ABSLAYER, ABSLAYER, DIMN, LLDLAMuSingle), LLDLALOOPPHASE);

  //  Universe::AddTrans(MAdd::GetClass(), new MAddLoopRef(ABSLAYER, ABSLAYER, DIMM, LLDLAMuDouble), LLDLALOOPPHASE);

  //  Universe::AddTrans(MAdd::GetClass(), new MAddLoopRef(ABSLAYER, ABSLAYER, DIMN, LLDLAMuDouble), LLDLALOOPPHASE);

  Universe::AddTrans(MAdd::GetClass(), new MAddToVAddLoopRef(ABSLAYER, ABSLAYER, COLVECTOR), LLDLALOOPPHASE);

  Universe::AddTrans(MAdd::GetClass(), new MAddToVAddLoopRef(ABSLAYER, ABSLAYER, ROWVECTOR), LLDLALOOPPHASE);

  //  Universe::AddTrans(MAdd::GetClass(), new MAddToRegArith(ABSLAYER, ABSLAYER), LLDLALOOPPHASE);

  return;
}

void AddMVMulTrans()
{
  Universe::AddTrans(MVMul::GetClass(), new MVMulLoopRef(ABSLAYER, ABSLAYER, DIMM, LLDLAMuDouble), LLDLALOOPPHASE);

  Universe::AddTrans(MVMul::GetClass(), new MVMulLoopRef(ABSLAYER, ABSLAYER, DIMN, LLDLAMuDouble), LLDLALOOPPHASE);

  Universe::AddTrans(MVMul::GetClass(), new MVMulLoopRef(ABSLAYER, ABSLAYER, DIMM, LLDLAMuSingle), LLDLALOOPPHASE);

  Universe::AddTrans(MVMul::GetClass(), new MVMulLoopRef(ABSLAYER, ABSLAYER, DIMN, LLDLAMuSingle), LLDLALOOPPHASE);

  Universe::AddTrans(MVMul::GetClass(), new MVMulToRegArith(ABSLAYER, ABSLAYER), LLDLALOOPPHASE);

  return;
}

void AddSMMulTrans()
{
  //Introduces loops in the m and n dimension for SMMul
  Universe::AddTrans(SMMul::GetClass(), new SMulLoopRef(ABSLAYER, ABSLAYER, DIMM, LLDLAMuSingle), LLDLALOOPPHASE);

  Universe::AddTrans(SMMul::GetClass(), new SMulLoopRef(ABSLAYER, ABSLAYER, DIMN, LLDLAMuDouble), LLDLALOOPPHASE);

  return;
}

void AddUnrollingTrans()
{

#if DOCOMPACTLOOPUNROLLING
#if DO2MUTRANSFORMATIONS
  Universe::AddTrans(SplitSingleIter::GetClass(),
		     new CompactlyUnrollLoop(2), LLDLALOOPUNROLLPHASE);
#endif // DO2MUTRANSFORMATIONS

#if DO3MUTRANSFORMATIONS
  Universe::AddTrans(SplitSingleIter::GetClass(), 
		     new CompactlyUnrollLoop(3), LLDLALOOPUNROLLPHASE);
#endif // DO3MUTRANSFORMATIONS

#if DO16MUTRANSFORMATIONS
  Universe::AddTrans(SplitSingleIter::GetClass(), 
		     new CompactlyUnrollLoop(16), LLDLALOOPUNROLLPHASE);
#endif // DO3MUTRANSFORMATIONS

#if DOLARGEMUTRANSFORMATIONS
  Universe::AddTrans(SplitSingleIter::GetClass(), 
		     new CompactlyUnrollLoop(bigSize / LLDLA_MU), LLDLALOOPUNROLLPHASE);
#endif // DOLARGEMUTRANSFORMATIONS

#endif // DOCOMPACTLOOPUNROLLING

#if DOPARTIALLOOPUNROLLING
  for (unsigned int mult = PARTIALUNROLLINGSTARTCOEF; mult <= PARTIALUNROLLINGENDCOEF; mult += 2) {
    Universe::AddTrans(SplitSingleIter::GetClass(), new PartiallyUnrollLoop(mult), LLDLALOOPUNROLLPHASE);
  }
#endif // DOPARTIALLOOPUNROLLING

}

void AddSVMulTrans()
{
  Universe::AddTrans(SVMul::GetClass(), new SVMulToRegArith(ABSLAYER, ABSLAYER, ROWVECTOR), LLDLALOOPPHASE);

  Universe::AddTrans(SVMul::GetClass(), new SVMulToRegArith(ABSLAYER, ABSLAYER, COLVECTOR), LLDLALOOPPHASE);

  Universe::AddTrans(SVMul::GetClass(), new SVMulToScalarArith(ABSLAYER, ABSLAYER, ROWVECTOR), LLDLALOOPPHASE);

  Universe::AddTrans(SVMul::GetClass(), new SVMulToScalarArith(ABSLAYER, ABSLAYER, COLVECTOR), LLDLALOOPPHASE);

  Universe::AddTrans(SVMul::GetClass(), new ResidualPartitionSVMul(ABSLAYER, ABSLAYER, COLVECTOR, arch->VecRegWidth(REAL_SINGLE)), LLDLALOOPPHASE);

  return;
}

void AddVMMulTrans()
{
  // Transformers for vector matrix multiply
  Universe::AddTrans(VMMul::GetClass(), new VMMulToRegArith(ABSLAYER, ABSLAYER), LLDLALOOPPHASE);

  Universe::AddTrans(VMMul::GetClass(), new VMMulLoopRef(ABSLAYER, ABSLAYER, DIMK, LLDLAMuSingle), LLDLALOOPPHASE);

  Universe::AddTrans(VMMul::GetClass(), new VMMulLoopRef(ABSLAYER, ABSLAYER, DIMN, LLDLAMuSingle), LLDLALOOPPHASE);

  Universe::AddTrans(VMMul::GetClass(), new VMMulLoopRef(ABSLAYER, ABSLAYER, DIMK, LLDLAMuDouble), LLDLALOOPPHASE);

  Universe::AddTrans(VMMul::GetClass(), new VMMulLoopRef(ABSLAYER, ABSLAYER, DIMN, LLDLAMuDouble), LLDLALOOPPHASE);

  return;
}

void AddVAddTrans()
{
  Universe::AddTrans(VAdd::GetClass(), new VAddToRegArith(ABSLAYER, ABSLAYER), LLDLALOOPPHASE);

  return;
}

void AddTransposeTrans()
{
  Universe::AddTrans(LLDLATranspose::GetClass(), new LLDLATransposeLowerLayer(ABSLAYER, LLDLAMIDLAYER), LLDLALOOPPHASE);
  Universe::AddTrans(LLDLATranspose::GetClass(), new LLDLATransposeLowerLayer(LLDLAMIDLAYER, LLDLAPRIMITIVELAYER), LLDLALOOPPHASE);

  return;
}

void AddPartitionRecombineTrans()
{
  Universe::AddTrans(Partition::GetClass(), new PartitionLowerLayer(ABSLAYER, LLDLAMIDLAYER), LLDLALOOPPHASE);
  Universe::AddTrans(Partition::GetClass(), new PartitionLowerLayer(LLDLAMIDLAYER, LLDLAPRIMITIVELAYER), LLDLALOOPPHASE);

  Universe::AddTrans(Recombine::GetClass(), new RecombineLowerLayer(ABSLAYER, LLDLAMIDLAYER), LLDLALOOPPHASE);
  Universe::AddTrans(Recombine::GetClass(), new RecombineLowerLayer(LLDLAMIDLAYER, LLDLAPRIMITIVELAYER), LLDLALOOPPHASE);
}

void AddSetToZeroTrans() {
  Universe::AddTrans(SetToZero::GetClass(), new SetToZeroLowerLayer(ABSLAYER, LLDLAMIDLAYER), LLDLALOOPPHASE);
  Universe::AddTrans(SetToZero::GetClass(), new SetToZeroLowerLayer(LLDLAMIDLAYER, LLDLAPRIMITIVELAYER), LLDLALOOPPHASE);
}

void AddTransformations()
{
  AddGemmTrans();
  AddVVDotTrans();
  AddMAddTrans();
  AddMVMulTrans();
  AddSMMulTrans();
  AddSVMulTrans();
  AddVMMulTrans();
  AddVAddTrans();

  AddTransposeTrans();

  AddUnrollingTrans();

  AddPartitionRecombineTrans();

  AddSetToZeroTrans();
}

#endif // DOLLDLA
