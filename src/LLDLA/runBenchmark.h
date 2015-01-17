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

#ifndef RUN_BENCHMARK_H_
#define RUN_BENCHMARK_H_

#include "allTransformations.h"
#include "base.h"
#include "costs.h"
#include "DLAReg.h"
#include "loopSupport.h"
#include "problemInstanceStats.h"
#include "runnerUtils.h"
#include "runtimeEvaluation.h"
#include "transform.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <climits>
#include <sstream>
#include <time.h>

#if DOLLDLA

ProblemInstanceStats* RunBenchmark(int algNum, RealPSet* algPSet, ProblemInstance* problemInstance);

#endif // DOLLDLA

#endif // RUN_BENCHMARK_H_
