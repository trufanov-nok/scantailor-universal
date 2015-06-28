/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "QuadraticFunction.h"
#include <algorithm>
#include <cstddef>
#include <cassert>

using namespace Eigen;

QuadraticFunction::QuadraticFunction(size_t num_vars)
:	A(num_vars, num_vars),
	b(num_vars),
	c(0)
{
	A.setZero();
	b.setZero();
}

void
QuadraticFunction::reset()
{
	A.setZero();
	b.setZero();
	c = 0;
}

double
QuadraticFunction::evaluate(VectorXd const& x) const
{
	return (x.transpose() * A * x + b.transpose() * x).value() + c;
}

QuadraticFunction::Gradient
QuadraticFunction::gradient() const
{
	return Gradient{ A.transpose() + A, b };
}

void
QuadraticFunction::recalcForTranslatedArguments(double const* translation)
{
	size_t const num_vars = numVars();

	for (size_t i = 0; i < num_vars; ++i) {
		// Bi * (Xi + Ti) = Bi * Xi + Bi * Ti
		c += b[i] * translation[i];
	}

	for (size_t i = 0; i < num_vars; ++i) {
		for (size_t j = 0; j < num_vars; ++j) {
			// (Xi + Ti)*Aij*(Xj + Tj) = Xi*Aij*Xj + Aij*Tj*Xi + Aij*Ti*Xj + Aij*Ti*Tj
			double const a = A(i, j);
			b[i] += a * translation[j];
			b[j] += a * translation[i];
			c += a * translation[i] * translation[j];
		}
	}
}

void
QuadraticFunction::swap(QuadraticFunction& other)
{
	A.swap(other.A);
	b.swap(other.b);
	std::swap(c, other.c);
}

QuadraticFunction&
QuadraticFunction::operator+=(QuadraticFunction const& other)
{
	A += other.A;
	b += other.b;
	c += other.c;
	return *this;
}

QuadraticFunction&
QuadraticFunction::operator*=(double scalar)
{
	A *= scalar;
	b *= scalar;
	c *= scalar;
	return *this;
}
