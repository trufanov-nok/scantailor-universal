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

#ifndef IMAGEPROC_HOUGH_TRANSFORM_H_
#define IMAGEPROC_HOUGH_TRANSFORM_H_

#include "imageproc_config.h"
#include "Grid.h"
#include "Constants.h"
#include <QSize>
#include <QPoint>
#include <QPointF>
#include <QLineF>
#include <vector>
#include <cmath>
#include <assert.h>

namespace imageproc
{

class HoughAngle
{
public:
	HoughAngle(QPointF const& unit_normal) : m_unitNormal(unit_normal) {}

	/**
	 * \brief Unit length vector, perpendicular to our angle.
	 *
	 * The exact direction (+90 or -90 degrees) doesn't matter,
	 * provided it's used consistently.
	 */
	QPointF const& unitNormal() const { return m_unitNormal; }
private:
	QPointF m_unitNormal;
};


/**
 * \brief An ordered collection of angles.
 *
 * Exact order doesn't matter, though neighbouring indices are expected to correspond
 * to neighbouring angles.
 *
 * \tparam Angle HoughAngle, a subclass of it, or a completely different class,
 *         as long as it provides the public interface of HoughAngle.
 *         Custom classes may carry additional information that will be used
 *         by angle-dependent weight function provided by client.
 *
 * \see HoughLineDetector2::processAngleDependentSample()
 */
template<typename Angle = HoughAngle>
struct HoughAngleCollection
{
	std::vector<Angle> angles;
};


/**
 * \brief Specialization for HoughAngle.
 *
 * Provides a convenience constructor that generates a range of angles for you.
 */
template<>
struct HoughAngleCollection<HoughAngle>
{
	std::vector<HoughAngle> angles;

	/**
	 * When this constructor is used, client code has to manually populate
	 * the angles vector.
	 */
	HoughAngleCollection() {}

	/**
	 * \brief Generates a range of angles.
	 *
	 * \brief mid_angle_deg Middle angle in the range, in degrees.
	 * \brief max_angle_deviation_deg mid_angle_deg plus and minus this value
	 *        defines the range boundaries. In degrees. Must not be negative.
	 * \brief angle_step_deg Distance in degrees between two adjacent angles.
	 *        Must not be negative.
	 */
	HoughAngleCollection(double mid_angle_deg, double max_angle_deviation_deg, double angle_step_deg);
};


/**
 * \brief Transforms spatial data to a special representation called Hough space,
 *        where peaks correspond to visual lines in spatial data.
 *
 * \tparam T Type of accumulator variables. The Hough space will be a grid of these.
 * \tparam Angle Normally HoughAngle, but may be a custom class.
 *         See HoughAngleCollection for details.
 */
template<typename T, typename Angle = HoughAngle>
class HoughTransform
{
public:
	/**
	 * \brief Constructor.
	 * \param spatial_size Dimensions of input image.
	 * \param distance_step Spatial distance covered by a single column in Hough space.
	 * \param angles A range of angles to look for. Each of those will correspond
	 *        to a row in Hough space.
	 * \param hough_space_initial_value The value to initialize the Hough space to.
	 */
	HoughTransform(QSize const& spatial_size, double distance_step,
		HoughAngleCollection<Angle> const& angles, T hough_space_initial_value = T());

	QSize const& spatialSize() const { return m_spatialSize; }

	QSize houghSize() const { return QSize(m_houghSpace.width(), m_houghSpace.height()); }

	/**
	 * \brief Processes a spatial sample and updates Hough space.
	 *
	 * \p update_op will be called like this:
	 * \code
	 * T& hough_space_val = ...;
	 * update_op(hough_space_val);
	 * \code
	 * A typical example would be having gradient magnitude in spacial space
	 * and the sum of gradient magnitudes along each line in Hough space.
	 * In such case, the client code may look like this:
	 * \code
	 * HoughTransform<float> ht(...);
	 * for (int y = 0; y < height; ++y) {
	 *     for (int x = 0; x < width; ++x) {
	 *         float const grad_mag = getGradientMagnitude(image, x, y);
	 *         processSample(x, y, [=](float& hough_space_val) {
	 *             hough_space_val += grad_mag;
	 *         });
	 *     }
	 * }
	 * \endcode
	 */
	template<typename F>
	void processSample(int x, int y, F update_op);

	/**
	 * \brief An extension to processSample(), where update operation
	 *        takes the angle as an additional argument.
	 *
	 * \p update_op will be called like this:
	 * \code
	 * T& hough_space_val = ...;
	 * Angle const& angle = ...;
	 * update_op(hough_space_val, angle);
	 * \endcode
	 * Angle is a template parameter of HoughTransform. It's HoughAngle by default,
	 * but custom classes can also be used.
	 */
	template<typename F>
	void processSample2(int x, int y, F update_op);

	/**
	 * \brief An extension to processAngleDependentSample(), where update operation
	 *        takes the position in Hough space as an additional argument.
	 *
	 * \p update_op will be called like this:
	 * \code
	 * T& hough_space_val = ...;
	 * Angle const& angle = ...;
	 * QPoint const hough_space_pos = ...;
	 * update_op(housh_space_val, angle, hough_space_pos);
	 * \endcode
	 * \see houghToSpatial()
	 */
	template<typename F>
	void processSample3(int x, int y, F update_op);

	/**
	 * Use findPeaksGeneric() to locate peaks in hough space.
	 */
	Grid<T>& houghSpace() { return m_houghSpace; }

	Grid<T> const& houghSpace() const { return m_houghSpace; }

	/**
	 * \brief Returns a spatial line corresponding to a point in Hough space.
	 *
	 * Line enpoints will be set arbitrarily.
	 */
	QLineF houghToSpatial(int x, int y) const;

	QLineF houghToSpatial(QPoint hough_pt) const { return houghToSpatial(hough_pt.x(), hough_pt.y()); }

	QPointF const& spatialOrigin() const { return m_spatialOrigin; }

	Angle const& angleForRow(int row) const { return m_angles[row]; }

	/**
	 * \brief Column in Hough space corresponding to zero distance from spatial origin.
	 * \see houghSpace(), spatialOrigin()
	 */
	int zeroDistanceCol() const { return m_zeroDistanceCol; }

	/**
	 * \brief Returns the accumulated value for a point in Hough space.
	 */
	T houghValue(int x, int y) const;

	T houghValue(QPoint hough_pt) const { return houghValue(hough_pt.x(), hough_pt.y()); }
private:
	/**
	 * Horizontal axis: distances from origin.\n
	 * Vertical axis: angles.
	 */
	Grid<T> m_houghSpace;
	std::vector<Angle> m_angles;
	QSize m_spatialSize;
	QPointF m_spatialOrigin;
	double m_distanceStep;

	/** Column in m_houghSpace corresponding to zero distance from m_spatialOrigin. */
	int m_zeroDistanceCol;
};


HoughAngleCollection<HoughAngle>::HoughAngleCollection(
	double mid_angle_deg, double max_angle_deviation_deg, double angle_step_deg)
{
	assert(max_angle_deviation_deg >= 0);
	assert(angle_step_deg >= 0);

	int const num_angles = 1 + 2 * int(max_angle_deviation_deg / angle_step_deg);
	angles.reserve(num_angles);

	for (int i = 0; i < num_angles; ++i) {
		int const offset = i - (num_angles >> 1);
		double const degrees = mid_angle_deg + offset * angle_step_deg;
		double const radians = degrees * constants::DEG2RAD;
		QPointF const unit_normal(-sin(radians), cos(radians));
		angles.push_back(HoughAngle(unit_normal));
	}
}

template<typename T, typename Angle>
HoughTransform<T, Angle>::HoughTransform(
	QSize const& spatial_size, double distance_step,
	HoughAngleCollection<Angle> const& angles,
	T const hough_space_initial_value)
:	m_angles(angles.angles)
,	m_spatialSize(spatial_size)
,	m_spatialOrigin(0.5 * spatial_size.width(), 0.5 * spatial_size.height())
,	m_distanceStep(distance_step)
{
	using namespace std;

	double const max_distance = sqrt(
		m_spatialOrigin.x() * m_spatialOrigin.x() +
		m_spatialOrigin.y() * m_spatialOrigin.y()
	);
	m_zeroDistanceCol = (int)ceil(max_distance / distance_step);
	int const num_distance_steps = 1 + 2 * m_zeroDistanceCol;

	Grid<T> hough_space(num_distance_steps, m_angles.size());
	hough_space.initInterior(hough_space_initial_value);

	hough_space.swap(m_houghSpace);
}

template<typename T, typename Angle>
template<typename F>
void
HoughTransform<T, Angle>::processSample(int x, int y, F update_op)
{
	processSample3(x, y, [=](T& hough_val, Angle const&, QPoint const&) {
		update_op(hough_val);
	});
}

template<typename T, typename Angle>
template<typename F>
void
HoughTransform<T, Angle>::processSample2(int x, int y, F update_op)
{
	processSample3(x, y, [=](T& hough_val, Angle const& angle, QPoint const&) {
		update_op(hough_val, angle);
	});
}

template<typename T, typename Angle>
template<typename F>
void
HoughTransform<T, Angle>::processSample3(int x, int y, F update_op)
{
	assert(x >= 0 && x < m_spatialSize.width());
	assert(y >= 0 && y < m_spatialSize.height());

	T* hough_line = m_houghSpace.data();
	int const hough_stride = m_houghSpace.stride();
	int const num_angles = m_angles.size();
	for (int i = 0; i < num_angles; ++i, hough_line += hough_stride) {
		Angle const& angle = m_angles[i];
		double const distance = QPointF::dotProduct(
			angle.unitNormal(), QPointF(x, y) - m_spatialOrigin
		);
		int const distance_col = m_zeroDistanceCol + (int)floor(distance / m_distanceStep + 0.5);
		assert(distance_col >= 0);
		assert(distance_col < m_houghSpace.width());
		update_op(hough_line[distance_col], angle, QPoint(distance_col, i));
	}
}

template<typename T, typename Angle>
QLineF
HoughTransform<T, Angle>::houghToSpatial(int x, int y) const
{
	double const dist = (x - m_zeroDistanceCol) * m_distanceStep;
	QPointF const unit_normal(m_angles[y].unitNormal());
	QPointF const line_direction(unit_normal.y(), -unit_normal.x());
	QPointF const p1(m_spatialOrigin + unit_normal * dist);
	QPointF const p2(p1 + line_direction);
	return QLineF(p1, p2);
}

template<typename T, typename Angle>
T
HoughTransform<T, Angle>::houghValue(int x, int y) const
{
	assert(x >= 0 && x < m_houghSpace.width());
	assert(y >= 0 && y < m_houghSpace.height());

	return m_houghSpace.data()[y * m_houghSpace.stride() + x];
}

} // namespace imageproc

#endif
