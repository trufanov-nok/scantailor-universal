/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015-2016  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef OPENCL_ACCELERATED_OPERATIONS_H_
#define OPENCL_ACCELERATED_OPERATIONS_H_

#include "AcceleratableOperations.h"
#include "NonCopyable.h"
#include "Grid.h"
#include "VecNT.h"
#include "dewarping/CylindricalSurfaceDewarper.h"
#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QRectF>
#include <QColor>
#include <CL/cl2.hpp>
#include <vector>
#include <memory>

namespace opencl
{

class OpenCLAcceleratedOperations : public AcceleratableOperations
{
	DECLARE_NON_COPYABLE(OpenCLAcceleratedOperations)
public:
	OpenCLAcceleratedOperations(
		cl::Context const& context,
		std::shared_ptr<AcceleratableOperations> const& fallback);

	virtual ~OpenCLAcceleratedOperations();

	virtual Grid<float> gaussBlur(
		Grid<float> const& src, float h_sigma, float v_sigma) const;

	virtual Grid<float> anisotropicGaussBlur(
		Grid<float> const& src, float dir_x, float dir_y,
		float dir_sigma, float ortho_dir_sigma) const;

	virtual std::pair<Grid<float>, Grid<uint8_t>> textFilterBank(
		Grid<float> const& src, std::vector<Vec2f> const& directions,
		std::vector<Vec2f> const& sigmas, float shoulder_length) const;

	virtual QImage dewarp(
		QImage const& src, QSize const& dst_size,
		dewarping::CylindricalSurfaceDewarper const& distortion_model,
		QRectF const& model_domain, QColor const& background_color,
		float min_density, float max_density,
		QSizeF const& min_mapping_area) const;

	virtual QImage affineTransform(
		QImage const& src, QTransform const& xform,
		QRect const& dst_rect, imageproc::OutsidePixels const& outside_pixels,
		QSizeF const& min_mapping_area) const;

	virtual imageproc::GrayImage renderPolynomialSurface(
		imageproc::PolynomialSurface const& surface, int width, int height);

	virtual imageproc::GrayImage savGolFilter(
		imageproc::GrayImage const& src, QSize const& window_size,
		int hor_degree, int vert_degree);

	virtual void hitMissReplaceInPlace(
		imageproc::BinaryImage& img, imageproc::BWColor img_surroundings,
		std::vector<Grid<char>> const& patterns);
private:
	Grid<float> gaussBlurUnguarded(
		Grid<float> const& src, float h_sigma, float v_sigma) const;

	Grid<float> anisotropicGaussBlurUnguarded(
		Grid<float> const& src, float dir_x, float dir_y,
		float dir_sigma, float ortho_dir_sigma) const;

	std::pair<Grid<float>, Grid<uint8_t>> textFilterBankUnguarded(
		Grid<float> const& src, std::vector<Vec2f> const& directions,
		std::vector<Vec2f> const& sigmas, float shoulder_length) const;

	QImage dewarpUnguarded(
		QImage const& src, QSize const& dst_size,
		dewarping::CylindricalSurfaceDewarper const& distortion_model,
		QRectF const& model_domain, QColor const& background_color,
		float min_density, float max_density,
		QSizeF const& min_mapping_area) const;

	QImage affineTransformUnguarded(
		QImage const& src, QTransform const& xform,
		QRect const& dst_rect, imageproc::OutsidePixels const& outside_pixels,
		QSizeF const& min_mapping_area) const;

	imageproc::GrayImage renderPolynomialSurfaceUnguarded(
		imageproc::PolynomialSurface const& surface, int width, int height);

	imageproc::GrayImage savGolFilterUnguarded(
		imageproc::GrayImage const& src, QSize const& window_size,
		int hor_degree, int vert_degree);

	void hitMissReplaceInPlaceUnguarded(
		imageproc::BinaryImage& img, imageproc::BWColor img_surroundings,
		std::vector<Grid<char>> const& patterns);

	cl::Context m_context;
	std::vector<cl::Device> m_devices;
	cl::CommandQueue m_commandQueue; // Has to come after m_context and m_devices.
	cl::Program m_program;
	std::shared_ptr<AcceleratableOperations> m_ptrFallback;
};

} // namespace opencl

#endif
