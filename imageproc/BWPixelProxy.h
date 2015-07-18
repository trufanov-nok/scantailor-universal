/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) 2015  Joseph Artsimovich <joseph.artsimich@gmail.com>

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

#ifndef IMAGEPROC_BW_PIXEL_PROXY_H_
#define IMAGEPROC_BW_PIXEL_PROXY_H_

#include "imageproc_config.h"
#include <cstdint>
#include <cassert>

namespace imageproc
{

/**
 * @brief Used as a proxy type to represent a pixel from a BinaryImage
 *        in rasterOpGeneric().
 */
class IMAGEPROC_EXPORT BWPixelProxy
{
	// Member-wise copying is OK.
public:
	/**
	 * @param word A reference to a 4-byte word containing the bit that represents
	 *        the pixel we are interested in.
	 * @param shift The position of the pixel of intereset in the word, counting
	 *        from the *most significant* bit. That is, a shift of 0 corresponds
	 *        to the most significant bit in the word.
	 */
	BWPixelProxy(uint32_t& word, int shift) : m_pWord(&word), m_rightShift(31 - shift) {
		assert((shift & ~31) == 0);
	}

	/**
	 * @brief Switches the bit representing the pixel of interest to on or off.
	 *
	 * @param bit Either 0 or 1, indicating whether to turn the bit on or off.
	 * @return The reference to this proxy object.
	 */
	BWPixelProxy& operator=(uint32_t bit) {
		assert(bit <= 1);
		uint32_t const mask = uint32_t(1) << m_rightShift;
		*m_pWord = (*m_pWord & ~mask) | (bit << m_rightShift);
		return *this;
	}

	/**
	 * Implicit conversion to uint32_t, which takes values 0 or 1, representing
	 * the state of the particular pixel of intereset.
	 */
	operator uint32_t() const { return (*m_pWord >> m_rightShift) & uint32_t(1); }
private:
	uint32_t* m_pWord;

	/**
	 * How many positions to shift *m_pWord to the right until the pixel of interest
	 * is at position 0.
	 */
	int m_rightShift;
};

} // namespace imageproc

#endif
