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

#include "problemInstanceStats.h"

#if DOLLDLA

#include "lldlaUniverse.h"

ProblemInstanceStats* RunProblemWithRTE(int algNum, RealPSet* algPSet, ProblemInstance* problemInstance);

LLDLAUniverse* RunProblem(int algNum, RealPSet* startSet, ProblemInstance* problemInstance);

ProblemInstanceStats* RuntimeEvaluation(int algNum, LLDLAUniverse* uni, ProblemInstance* problemInstance);


#endif // DOLLDLA
