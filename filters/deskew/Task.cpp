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

#include "Task.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "Params.h"
#include "Dependencies.h"
#include "TaskStatus.h"
#include "DebugImagesImpl.h"
#include "filters/select_content/Task.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "BasicImageView.h"
#include "OrthogonalRotation.h"
#include "DewarpingView.h"
#include "dewarping/DewarpingImageTransform.h"
#include "dewarping/DistortionModelBuilder.h"
#include "dewarping/TextLineSegmenter.h"
#include "dewarping/TextLineTracer.h"
#include "dewarping/TopBottomEdgeTracer.h"
#include "imageproc/AffineImageTransform.h"
#include "imageproc/AffineTransformedImage.h"
#include "imageproc/AffineTransform.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/BWColor.h"
#include "imageproc/GrayImage.h"
#include "imageproc/OrthogonalRotation.h"
#include "imageproc/SkewFinder.h"
#include "imageproc/RasterOp.h"
#include "imageproc/ReduceThreshold.h"
#include "imageproc/UpscaleIntegerTimes.h"
#include "imageproc/SeedFill.h"
#include "imageproc/Connectivity.h"
#include "imageproc/Morphology.h"
#include "math/LineBoundedByRect.h"
#include "math/XSpline.h"
#include <QImage>
#include <QSize>
#include <QPoint>
#include <QRect>
#include <QLineF>
#include <QPainter>
#include <QPolygonF>
#include <QTransform>
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <assert.h>
#include <stddef.h>

#include "CommandLine.h"

namespace deskew
{

using namespace imageproc;
using namespace dewarping;

class Task::NoDistortionUiUpdater : public FilterResult
{
public:
	NoDistortionUiUpdater(IntrusivePtr<Filter> const& filter,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		std::auto_ptr<DebugImagesImpl> dbg_img,
		AffineTransformedImage const& transformed_image,
		PageId const& page_id,
		Params const& page_params,
		bool batch_processing);

	virtual void updateUI(FilterUiInterface* ui);

	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	std::shared_ptr<AcceleratableOperations> m_ptrAccelOps;
	std::auto_ptr<DebugImagesImpl> m_ptrDbg;
	AffineTransformedImage m_fullSizeImage;
	QImage m_downscaledImage;
	PageId m_pageId;
	Params m_pageParams;
	bool m_batchProcessing;
};

class Task::RotationUiUpdater : public FilterResult
{
public:
	RotationUiUpdater(IntrusivePtr<Filter> const& filter,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		std::auto_ptr<DebugImagesImpl> dbg_img,
		AffineTransformedImage const& full_size_image,
		PageId const& page_id,
		Params const& page_params,
		bool batch_processing);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	std::shared_ptr<AcceleratableOperations> m_ptrAccelOps;
	std::auto_ptr<DebugImagesImpl> m_ptrDbg;
	AffineTransformedImage m_fullSizeImage;
	QImage m_downscaledImage;
	PageId m_pageId;
	Params m_pageParams;
	bool m_batchProcessing;
};

class Task::PerspectiveUiUpdater : public FilterResult
{
public:
	PerspectiveUiUpdater(IntrusivePtr<Filter> const& filter,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		std::auto_ptr<DebugImagesImpl> dbg_img,
		AffineTransformedImage const& full_size_image,
		PageId const& page_id, Params const& page_params,
		bool batch_processing);

	virtual void updateUI(FilterUiInterface* ui);

	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	std::shared_ptr<AcceleratableOperations> m_ptrAccelOps;
	std::auto_ptr<DebugImagesImpl> m_ptrDbg;
	AffineTransformedImage m_fullSizeImage;
	QImage m_downscaledImage;
	PageId m_pageId;
	Params m_pageParams;
	bool m_batchProcessing;
};

class Task::DewarpingUiUpdater : public FilterResult
{
public:
	DewarpingUiUpdater(IntrusivePtr<Filter> const& filter,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		std::auto_ptr<DebugImagesImpl> dbg_img,
		AffineTransformedImage const& full_size_image,
		PageId const& page_id, Params const& page_params,
		bool batch_processing);

	virtual void updateUI(FilterUiInterface* ui);

	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	std::shared_ptr<AcceleratableOperations> m_ptrAccelOps;
	std::auto_ptr<DebugImagesImpl> m_ptrDbg;
	AffineTransformedImage m_fullSizeImage;
	QImage m_downscaledImage;
	PageId m_pageId;
	Params m_pageParams;
	bool m_batchProcessing;
};


Task::Task(IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<select_content::Task> const& next_task,
	PageId const& page_id, bool const batch_processing, bool const debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_ptrNextTask(next_task),
	m_pageId(page_id),
	m_batchProcessing(batch_processing)
{
	if (debug) {
		m_ptrDbg.reset(new DebugImagesImpl);
	}
}

Task::~Task()
{
}

FilterResultPtr
Task::process(
	TaskStatus const& status,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QImage const& orig_image,
	CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
	AffineImageTransform const& orig_image_transform,
	OrthogonalRotation const& pre_rotation)
{
	status.throwIfCancelled();

	Dependencies const deps(orig_image_transform.origCropArea(), pre_rotation);

	std::unique_ptr<Params> params(m_ptrSettings->getPageParams(m_pageId));
	std::unique_ptr<Params> old_params;

	if (params.get()) {
		if (!deps.matches(params->dependencies())) {
			params.swap(old_params);
		}
	}
	
	if (!params.get()) {
		params.reset(new Params(deps));
		if (old_params) {
			params->takeManualSettingsFrom(*old_params);
		}
	}

	switch (params->distortionType().get()) {
		case DistortionType::NONE:
			return processNoDistortion(
				status, accel_ops, orig_image, gray_orig_image_factory,
				orig_image_transform, *params
			);
		case DistortionType::ROTATION:
			return processRotationDistortion(
				status, accel_ops, orig_image, gray_orig_image_factory,
				orig_image_transform, *params
			);
		case DistortionType::PERSPECTIVE:
			return processPerspectiveDistortion(
				status, accel_ops, orig_image, gray_orig_image_factory,
				orig_image_transform, *params
			);
		case DistortionType::WARP:
			return processWarpDistortion(
				status, accel_ops, orig_image, gray_orig_image_factory,
				orig_image_transform, *params
			);
	} // switch

	throw std::logic_error("Unexpected distortion type");
}

FilterResultPtr
Task::processNoDistortion(
	TaskStatus const& status,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QImage const& orig_image,
	CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
	AffineImageTransform const& orig_image_transform, Params& params)
{
	// Necessary to update dependencies.
	m_ptrSettings->setPageParams(m_pageId, params);

	if (m_ptrNextTask) {
		return m_ptrNextTask->process(
			status, accel_ops, orig_image, gray_orig_image_factory,
			std::make_shared<AffineImageTransform>(orig_image_transform)
		);
	} else {
		return FilterResultPtr(
			new NoDistortionUiUpdater(
				m_ptrFilter, accel_ops, m_ptrDbg,
				AffineTransformedImage(orig_image, orig_image_transform),
				m_pageId, params, m_batchProcessing
			)
		);
	}
}

FilterResultPtr
Task::processRotationDistortion(
	TaskStatus const& status,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QImage const& orig_image,
	CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
	AffineImageTransform const& orig_image_transform, Params& params)
{
	if (!params.rotationParams().isValid()) {
		QRect const transformed_crop_rect(
			orig_image_transform.transformedCropArea().boundingRect().toRect()
		);

		status.throwIfCancelled();

		if (transformed_crop_rect.isValid()) {
			BinaryImage bw_image(
				accel_ops->affineTransform(
					gray_orig_image_factory(), orig_image_transform.transform(),
					transformed_crop_rect, OutsidePixels::assumeColor(Qt::white)
				),
				BinaryThreshold::otsuThreshold(gray_orig_image_factory())
			);
			if (m_ptrDbg.get()) {
				m_ptrDbg->add(bw_image, "bw_image");
			}

			cleanup(status, bw_image);
			if (m_ptrDbg.get()) {
				m_ptrDbg->add(bw_image, "after_cleanup");
			}

			status.throwIfCancelled();

			SkewFinder skew_finder;
			Skew const skew(skew_finder.findSkew(bw_image));

			if (skew.confidence() >= skew.GOOD_CONFIDENCE) {
				params.rotationParams().setCompensationAngleDeg(-skew.angle());
			} else {
				params.rotationParams().setCompensationAngleDeg(0);
			}
			params.rotationParams().setMode(MODE_AUTO);

			m_ptrSettings->setPageParams(m_pageId, params);

			status.throwIfCancelled();
		}
	}

	if (m_ptrNextTask) {
		double const angle = params.rotationParams().compensationAngleDeg();
		return m_ptrNextTask->process(
			status, accel_ops, orig_image, gray_orig_image_factory,
			std::make_shared<AffineImageTransform>(
				orig_image_transform.adjusted(
					[angle](AffineImageTransform& xform) {
						xform.rotate(angle);
					}
				)
			)
		);
	} else {
		return FilterResultPtr(
			new RotationUiUpdater(
				m_ptrFilter, accel_ops, m_ptrDbg,
				AffineTransformedImage(orig_image, orig_image_transform),
				m_pageId, params, m_batchProcessing
			)
		);
	}
}

FilterResultPtr
Task::processPerspectiveDistortion(
	TaskStatus const& status,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QImage const& orig_image,
	CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
	AffineImageTransform const& orig_image_transform, Params& params)
{
	if (!params.perspectiveParams().isValid()) {
		DistortionModelBuilder model_builder(
			orig_image_transform.transform().inverted().map(QPointF(0, 1))
		);

		TextLineTracer::trace(
			AffineTransformedImage(gray_orig_image_factory(), orig_image_transform),
			model_builder, accel_ops, status, m_ptrDbg.get()
		);

		TopBottomEdgeTracer::trace(
			gray_orig_image_factory(), model_builder.verticalBounds(),
			model_builder, status, m_ptrDbg.get()
		);

		DistortionModel distortion_model(
			model_builder.tryBuildModel(m_ptrDbg.get(), &orig_image)
		);

		if (distortion_model.isValid()) {
			params.perspectiveParams().setCorner(
				PerspectiveParams::TOP_LEFT,
				distortion_model.topCurve().polyline().front()
			);
			params.perspectiveParams().setCorner(
				PerspectiveParams::TOP_RIGHT,
				distortion_model.topCurve().polyline().back()
			);
			params.perspectiveParams().setCorner(
				PerspectiveParams::BOTTOM_LEFT,
				distortion_model.bottomCurve().polyline().front()
			);
			params.perspectiveParams().setCorner(
				PerspectiveParams::BOTTOM_RIGHT,
				distortion_model.bottomCurve().polyline().back()
			);
		} else {
			// Set up a trivial transformation.
			QTransform const to_orig(orig_image_transform.transform().inverted());
			QRectF const transformed_box(orig_image_transform.transformedCropArea().boundingRect());

			params.perspectiveParams().setCorner(
				PerspectiveParams::TOP_LEFT, to_orig.map(transformed_box.topLeft())
			);
			params.perspectiveParams().setCorner(
				PerspectiveParams::TOP_RIGHT, to_orig.map(transformed_box.topRight())
			);
			params.perspectiveParams().setCorner(
				PerspectiveParams::BOTTOM_LEFT, to_orig.map(transformed_box.bottomLeft())
			);
			params.perspectiveParams().setCorner(
				PerspectiveParams::BOTTOM_RIGHT, to_orig.map(transformed_box.bottomRight())
			);
		}

		params.perspectiveParams().setMode(MODE_AUTO);

		m_ptrSettings->setPageParams(m_pageId, params);
	} // if (!params.isValid())

	if (m_ptrNextTask) {
		// DewarpingImageTransform can handle perspective distortion
		// as well, so we just use that.
		std::vector<QPointF> top_curve;
		std::vector<QPointF> bottom_curve;
		top_curve.push_back(params.perspectiveParams().corner(PerspectiveParams::TOP_LEFT));
		top_curve.push_back(params.perspectiveParams().corner(PerspectiveParams::TOP_RIGHT));
		bottom_curve.push_back(params.perspectiveParams().corner(PerspectiveParams::BOTTOM_LEFT));
		bottom_curve.push_back(params.perspectiveParams().corner(PerspectiveParams::BOTTOM_RIGHT));

		auto perspective_transform(
			std::make_shared<DewarpingImageTransform>(
				orig_image_transform.origSize(),
				orig_image_transform.origCropArea(),
				top_curve, bottom_curve,
				dewarping::DepthPerception() // Doesn't matter when curves are flat.
			)
		);
		return m_ptrNextTask->process(
			status, accel_ops, orig_image, gray_orig_image_factory, perspective_transform
		);
	} else {
		return FilterResultPtr(
			new PerspectiveUiUpdater(
				m_ptrFilter, accel_ops, m_ptrDbg,
				AffineTransformedImage(orig_image, orig_image_transform),
				m_pageId, params, m_batchProcessing
			)
		);
	}
}

FilterResultPtr
Task::processWarpDistortion(
	TaskStatus const& status,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QImage const& orig_image,
	CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
	AffineImageTransform const& orig_image_transform, Params& params)
{
	if (!params.dewarpingParams().isValid()) {
		DistortionModelBuilder model_builder(
			orig_image_transform.transform().inverted().map(QPointF(0, 1))
		);

		TextLineTracer::trace(
			AffineTransformedImage(gray_orig_image_factory(), orig_image_transform),
			model_builder, accel_ops, status, m_ptrDbg.get()
		);

		TopBottomEdgeTracer::trace(
			gray_orig_image_factory(), model_builder.verticalBounds(),
			model_builder, status, m_ptrDbg.get()
		);

		DistortionModel distortion_model(
			model_builder.tryBuildModel(m_ptrDbg.get(), &orig_image)
		);

		params.dewarpingParams().setDistortionModel(distortion_model);

		// Note that we don't reset depth perception, as it's a manual parameter
		// that's usually the same for all pictures in a project.

		params.dewarpingParams().setMode(MODE_AUTO);

		m_ptrSettings->setPageParams(m_pageId, params);
	} // if (!params.isValid())

	if (m_ptrNextTask) {
		auto dewarping_transform(
			std::make_shared<DewarpingImageTransform>(
				orig_image_transform.origSize(), orig_image_transform.origCropArea(),
				params.dewarpingParams().distortionModel().topCurve().polyline(),
				params.dewarpingParams().distortionModel().bottomCurve().polyline(),
				params.dewarpingParams().depthPerception()
			)
		);
		return m_ptrNextTask->process(
			status, accel_ops, orig_image, gray_orig_image_factory, dewarping_transform
		);
	} else {
		return FilterResultPtr(
			new DewarpingUiUpdater(
				m_ptrFilter, accel_ops, m_ptrDbg,
				AffineTransformedImage(orig_image, orig_image_transform),
				m_pageId, params, m_batchProcessing
			)
		);
	}
}

void
Task::cleanup(TaskStatus const& status, BinaryImage& image)
{
	// We don't have to clean up every piece of garbage.
	// The only concern are the horizontal shadows, which we remove here.

	BinaryImage reduced_image;
	
	{
		ReduceThreshold reductor(image);
		while (reductor.image().width() >= 2000 && reductor.image().height() >= 2000) {
			reductor.reduce(2);
		}
		reduced_image = reductor.image();
	}
	
	status.throwIfCancelled();
	
	QSize const brick(200, 14);
	BinaryImage opened(openBrick(reduced_image, brick, BLACK));
	reduced_image.release();
	
	status.throwIfCancelled();
	
	BinaryImage seed(upscaleIntegerTimes(opened, image.size(), WHITE));
	opened.release();
	
	status.throwIfCancelled();
	
	BinaryImage garbage(seedFill(seed, image, CONN8));
	seed.release();
	
	status.throwIfCancelled();
	
	rasterOp<RopSubtract<RopDst, RopSrc> >(image, garbage);
}


/*======================== Task::NoDistortionUiUpdater =====================*/

Task::NoDistortionUiUpdater::NoDistortionUiUpdater(
	IntrusivePtr<Filter> const& filter,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	std::auto_ptr<DebugImagesImpl> dbg_img,
	AffineTransformedImage const& full_size_image,
	PageId const& page_id,
	Params const& page_params,
	bool const batch_processing)
:	m_ptrFilter(filter),
	m_ptrAccelOps(accel_ops),
	m_ptrDbg(dbg_img),
	m_fullSizeImage(full_size_image),
	m_downscaledImage(
		ImageViewBase::createDownscaledImage(full_size_image.origImage(), accel_ops)
	),
	m_pageId(page_id),
	m_pageParams(page_params),
	m_batchProcessing(batch_processing)
{
}

void
Task::NoDistortionUiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.

	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	opt_widget->postUpdateUI(m_pageParams);
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

	ui->invalidateThumbnail(m_pageId);

	if (m_batchProcessing) {
		return;
	}

	QWidget* view = new BasicImageView(m_ptrAccelOps, m_fullSizeImage, m_downscaledImage);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
}


/*========================= Task::RotationUiUpdater =======================*/

Task::RotationUiUpdater::RotationUiUpdater(
	IntrusivePtr<Filter> const& filter,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	std::auto_ptr<DebugImagesImpl> dbg_img,
	AffineTransformedImage const& full_size_image,
	PageId const& page_id,
	Params const& page_params,
	bool const batch_processing)
:	m_ptrFilter(filter),
	m_ptrAccelOps(accel_ops),
	m_ptrDbg(dbg_img),
	m_fullSizeImage(full_size_image),
	m_downscaledImage(
		ImageViewBase::createDownscaledImage(full_size_image.origImage(), accel_ops)
	),
	m_pageId(page_id),
	m_pageParams(page_params),
	m_batchProcessing(batch_processing)
{
}

void
Task::RotationUiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.
	
	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();	
	opt_widget->postUpdateUI(m_pageParams);
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);
	
	ui->invalidateThumbnail(m_pageId);
	
	if (m_batchProcessing) {
		return;
	}
	
	double const angle = m_pageParams.rotationParams().compensationAngleDeg();
	ImageView* view = new ImageView(m_ptrAccelOps, m_fullSizeImage, m_downscaledImage, angle);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
	
	QObject::connect(
		view, SIGNAL(manualDeskewAngleSet(double)),
		opt_widget, SLOT(manualDeskewAngleSetExternally(double))
	);
	QObject::connect(
		opt_widget, SIGNAL(manualDeskewAngleSet(double)),
		view, SLOT(manualDeskewAngleSetExternally(double))
	);
}

/*======================= Task::PerspectiveUiUpdater ======================*/

Task::PerspectiveUiUpdater::PerspectiveUiUpdater(
	IntrusivePtr<Filter> const& filter,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	std::auto_ptr<DebugImagesImpl> dbg_img,
	AffineTransformedImage const& full_size_image,
	PageId const& page_id,
	Params const& page_params,
	bool const batch_processing)
:	m_ptrFilter(filter),
	m_ptrAccelOps(accel_ops),
	m_ptrDbg(dbg_img),
	m_fullSizeImage(full_size_image),
	m_downscaledImage(
		ImageViewBase::createDownscaledImage(full_size_image.origImage(), accel_ops)
	),
	m_pageId(page_id),
	m_pageParams(page_params),
	m_batchProcessing(batch_processing)
{
}

void
Task::PerspectiveUiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.

	using namespace dewarping;

	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	opt_widget->postUpdateUI(m_pageParams);
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

	ui->invalidateThumbnail(m_pageId);

	if (m_batchProcessing) {
		return;
	}

	XSpline top_curve;
	XSpline bottom_curve;
	top_curve.appendControlPoint(
		m_pageParams.perspectiveParams().corner(PerspectiveParams::TOP_LEFT), 0
	);
	top_curve.appendControlPoint(
		m_pageParams.perspectiveParams().corner(PerspectiveParams::TOP_RIGHT), 0
	);
	bottom_curve.appendControlPoint(
		m_pageParams.perspectiveParams().corner(PerspectiveParams::BOTTOM_LEFT), 0
	);
	bottom_curve.appendControlPoint(
		m_pageParams.perspectiveParams().corner(PerspectiveParams::BOTTOM_RIGHT), 0
	);
	DistortionModel distortion_model;
	distortion_model.setTopCurve(Curve(top_curve));
	distortion_model.setBottomCurve(Curve(bottom_curve));

	DewarpingView* view = new DewarpingView(
		m_ptrAccelOps, m_fullSizeImage, m_downscaledImage, m_pageId, distortion_model,

		// Doesn't matter when curves are flat.
		DepthPerception(),

		// Prevent the user from introducing curvature.
		/*fixed_number_of_control_points*/true
	);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());

	QObject::connect(
		view, SIGNAL(distortionModelChanged(dewarping::DistortionModel const&)),
		opt_widget, SLOT(manualDistortionModelSetExternally(dewarping::DistortionModel const&))
	);
}


/*======================= Task::DewarpingUiUpdater ======================*/

Task::DewarpingUiUpdater::DewarpingUiUpdater(
	IntrusivePtr<Filter> const& filter,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	std::auto_ptr<DebugImagesImpl> dbg_img,
	AffineTransformedImage const& full_size_image,
	PageId const& page_id,
	Params const& page_params,
	bool const batch_processing)
:	m_ptrFilter(filter),
	m_ptrAccelOps(accel_ops),
	m_ptrDbg(dbg_img),
	m_fullSizeImage(full_size_image),
	m_downscaledImage(
		ImageViewBase::createDownscaledImage(full_size_image.origImage(), accel_ops)
	),
	m_pageId(page_id),
	m_pageParams(page_params),
	m_batchProcessing(batch_processing)
{
}

void
Task::DewarpingUiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.

	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	opt_widget->postUpdateUI(m_pageParams);
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

	ui->invalidateThumbnail(m_pageId);

	if (m_batchProcessing) {
		return;
	}

	DewarpingView* view = new DewarpingView(
		m_ptrAccelOps, m_fullSizeImage, m_downscaledImage, m_pageId,
		m_pageParams.dewarpingParams().distortionModel(),
		m_pageParams.dewarpingParams().depthPerception(),
		/*fixed_number_of_control_points=*/false
	);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());

	QObject::connect(
		view, SIGNAL(distortionModelChanged(dewarping::DistortionModel const&)),
		opt_widget, SLOT(manualDistortionModelSetExternally(dewarping::DistortionModel const&))
	);
	QObject::connect(
		opt_widget, SIGNAL(depthPerceptionSetByUser(double)),
		view, SLOT(depthPerceptionChanged(double))
	);
}

} // namespace deskew

