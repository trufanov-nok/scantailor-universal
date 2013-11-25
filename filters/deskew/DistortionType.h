/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef DESKEW_DISTORTION_TYPE_H_
#define DESKEW_DISTORTION_TYPE_H_

#include <QString>

namespace deskew
{

class DistortionType
{
	// Member-wise copying is OK.
public:
	enum Type {
		FIRST,
		NONE = FIRST,
		ROTATION,
		PERSPECTIVE,
		WARP,
		LAST = WARP
	};

	/**
	 * @brief Constructs a DistortionType from a numberic value.
	 *
	 * No validation is performed on the argument.
	 */
	DistortionType(Type type = DistortionType::NONE);

	/**
	 * @brief Builds a DistortionType from a string.
	 *
	 * Unknown string values are silently converted to DistortionType::NONE.
	 */
	explicit DistortionType(QString const& from_string);

	QString toString() const;

	Type get() const { return m_type; }

	operator Type() const { return m_type; }

	/**
	 * @brief Set the distortion type to a numeric value.
	 *
	 * No validation is performed on the argument.
	 */
	void set(Type type) { m_type = type; }
private:
	Type m_type;
};

} // namespace deskew

#endif
