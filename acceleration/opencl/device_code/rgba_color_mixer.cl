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

/** @see imageproc::ColorMixer */
void rgba_color_mixer_add(float4* accum, float4 color, float weight)
{
	float const weighted_alpha = color.s3 * weight;
	color.s3 = 1.f;
	*accum += color * weighted_alpha;
}

/** @see imageproc::ColorMixer */
float4 rgba_color_mixer_mix(float4 accum, float total_weight)
{
	if (accum.s3 < 1e-5f) {
		return (float4)(0.f, 0.f, 0.f, 0.f);
	}

	float const scale = 1.f / accum.s3;
	accum.s3 /= total_weight * scale;
	return accum * scale;
}
