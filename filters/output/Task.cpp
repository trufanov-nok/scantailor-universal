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
#include "Params.h"
#include "Settings.h"
#include "ColorParams.h"
#include "OutputParams.h"
#include "OutputImageParams.h"
#include "OutputFileParams.h"
#include "OutputMargins.h"
#include "RenderParams.h"
#include "FilterUiInterface.h"
#include "TaskStatus.h"
#include "BasicImageView.h"
#include "ImageViewTab.h"
#include "TabbedImageView.h"
#include "PictureZoneComparator.h"
#include "OnDemandPictureZoneEditor.h"
#include "FillZoneComparator.h"
#include "FillZoneEditor.h"
#include "DespeckleState.h"
#include "DespeckleView.h"
#include "DespeckleVisualization.h"
#include "DespeckleLevel.h"
#include "ImageId.h"
#include "PageId.h"
#include "Utils.h"
#include "ThumbnailPixmapCache.h"
#include "DebugImagesImpl.h"
#include "OutputGenerator.h"
#include "CachingFactory.h"
#include "TiffWriter.h"
#include "ImageLoader.h"
#include "ErrorWidget.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/PolygonUtils.h"
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include <QImage>
#include <QString>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTabWidget>
#include <QCoreApplication>
#include <QDebug>
#include <functional>

#include "CommandLine.h"

using namespace imageproc;
using namespace dewarping;

namespace output
{

class Task::UiUpdater : public FilterResult
{
	Q_DECLARE_TR_FUNCTIONS(output::Task::UiUpdater)
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		IntrusivePtr<Settings> const& settings,
		std::auto_ptr<DebugImagesImpl> dbg_img,
		Params const& params,
		PageId const& page_id,
		QImage const& orig_image,
		QImage const& output_image,
		std::function<QPointF(QPointF const&)> const& orig_to_output,
		std::function<QPointF(QPointF const&)> const& output_to_orig,
		BinaryImage const& picture_mask,
		CachingFactory<QImage> const& cached_transformed_orig_image,
		CachingFactory<QImage> const& cached_downscaled_transformed_orig_image,
		DespeckleState const& despeckle_state,
		DespeckleVisualization const& despeckle_visualization,
		bool batch, bool debug);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	std::shared_ptr<AcceleratableOperations> m_ptrAccelOps;
	IntrusivePtr<Settings> m_ptrSettings;
	std::auto_ptr<DebugImagesImpl> m_ptrDbg;
	Params m_params;
	PageId m_pageId;
	QImage m_origImage;
	QImage m_outputImage;
	std::function<QPointF(QPointF const&)> m_origToOutput;
	std::function<QPointF(QPointF const&)> m_outputToOrig;
	QImage m_downscaledOutputImage;
	BinaryImage m_pictureMask;
	CachingFactory<QImage> m_cachedTransformedOrigImage;
	CachingFactory<QImage> m_cachedDownscaledTransformedOrigImage;
	DespeckleState m_despeckleState;
	DespeckleVisualization m_despeckleVisualization;
	DespeckleLevel m_despeckleLevel;
	bool m_batchProcessing;
	bool m_debug;
};


Task::Task(IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	PageId const& page_id, OutputFileNameGenerator const& out_file_name_gen,
	ImageViewTab const last_tab, bool const batch, bool const debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_ptrThumbnailCache(thumbnail_cache),
	m_pageId(page_id),
	m_outFileNameGen(out_file_name_gen),
	m_lastTab(last_tab),
	m_batchProcessing(batch),
	m_debug(debug)
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
	std::shared_ptr<AbstractImageTransform const> const& orig_image_transform,
	QRectF const& content_rect, QRectF const& outer_rect)
{
	double const scaling_factor = m_ptrSettings->scalingFactor();
	std::shared_ptr<AbstractImageTransform> const scaled_transform(orig_image_transform->clone());
	QTransform const post_scale_xform(scaled_transform->scale(scaling_factor, scaling_factor));

	return processScaled(
		status, accel_ops, orig_image, gray_orig_image_factory, scaled_transform,
		post_scale_xform.mapRect(content_rect), post_scale_xform.mapRect(outer_rect)
	);
}

FilterResultPtr
Task::processScaled(
	TaskStatus const& status,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QImage const& orig_image,
	CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
	std::shared_ptr<AbstractImageTransform const> const& orig_image_transform,
	QRectF const& content_rect, QRectF const& outer_rect)
{
	status.throwIfCancelled();

	Params params(m_ptrSettings->getParams(m_pageId));
	RenderParams const render_params(params.colorParams());
	QString const out_file_path(m_outFileNameGen.filePathFor(m_pageId));
	QFileInfo const out_file_info(out_file_path);

	QString const automask_dir(Utils::automaskDir(m_outFileNameGen.outDir()));
	QString const automask_file_path(
		QDir(automask_dir).absoluteFilePath(out_file_info.fileName())
	);
	QFileInfo automask_file_info(automask_file_path);

	QString const speckles_dir(Utils::specklesDir(m_outFileNameGen.outDir()));
	QString const speckles_file_path(
		QDir(speckles_dir).absoluteFilePath(out_file_info.fileName())
	);
	QFileInfo speckles_file_info(speckles_file_path);

	bool const need_picture_editor = render_params.mixedOutput() && !m_batchProcessing;
	bool const need_speckles_image = params.despeckleLevel() != DESPECKLE_OFF
		&& params.colorParams().colorMode() != ColorParams::COLOR_GRAYSCALE && !m_batchProcessing;
	
	OutputGenerator const generator(
		orig_image_transform, content_rect, outer_rect,
		params.colorParams(), params.despeckleLevel()
	);
	
	OutputImageParams new_output_image_params(
		orig_image_transform->fingerprint(),
		generator.outputImageRect(), generator.outputContentRect(),
		params.colorParams(), params.despeckleLevel()
	);

	ZoneSet const new_picture_zones(m_ptrSettings->pictureZonesForPage(m_pageId));
	ZoneSet const new_fill_zones(m_ptrSettings->fillZonesForPage(m_pageId));
	
	bool need_reprocess = false;
	do { // Just to be able to break from it.
		
		std::auto_ptr<OutputParams> stored_output_params(
			m_ptrSettings->getOutputParams(m_pageId)
		);
		
		if (!stored_output_params.get()) {
			need_reprocess = true;
			break;
		}
		
		if (!stored_output_params->outputImageParams().matches(new_output_image_params)) {
			need_reprocess = true;
			break;
		}

		if (!PictureZoneComparator::equal(stored_output_params->pictureZones(), new_picture_zones)) {
			need_reprocess = true;
			break;
		}

		if (!FillZoneComparator::equal(stored_output_params->fillZones(), new_fill_zones)) {
			need_reprocess = true;
			break;
		}
		
		if (!out_file_info.exists()) {
			need_reprocess = true;
			break;
		}
		
		if (!stored_output_params->outputFileParams().matches(OutputFileParams(out_file_info))) {
			need_reprocess = true;
			break;
		}

		if (need_picture_editor) {
			if (!automask_file_info.exists()) {
				need_reprocess = true;
				break;
			}

			if (!stored_output_params->automaskFileParams().matches(OutputFileParams(automask_file_info))) {
				need_reprocess = true;
				break;
			}
		}

		if (need_speckles_image) {
			if (!speckles_file_info.exists()) {
				need_reprocess = true;
				break;
			}
			if (!stored_output_params->specklesFileParams().matches(OutputFileParams(speckles_file_info))) {
				need_reprocess = true;
				break;
			}
		}
	
	} while (false);
	
	QImage out_img;
	BinaryImage automask_img;
	BinaryImage speckles_img;
	
	if (!need_reprocess) {
		QFile out_file(out_file_path);
		if (out_file.open(QIODevice::ReadOnly)) {
			out_img = ImageLoader::load(out_file, 0);
		}
		need_reprocess = out_img.isNull();

		if (need_picture_editor && !need_reprocess) {
			QFile automask_file(automask_file_path);
			if (automask_file.open(QIODevice::ReadOnly)) {
				automask_img = BinaryImage(ImageLoader::load(automask_file, 0));
			}
			need_reprocess = automask_img.isNull() || automask_img.size() != out_img.size();
		}

		if (need_speckles_image && !need_reprocess) {
			QFile speckles_file(speckles_file_path);
			if (speckles_file.open(QIODevice::ReadOnly)) {
				speckles_img = BinaryImage(ImageLoader::load(speckles_file, 0));
			}
			need_reprocess = speckles_img.isNull();
		}
	}

	if (need_reprocess) {
		// Even in batch processing mode we should still write automask, because it
		// will be needed when we view the results back in interactive mode.
		// The same applies even more to speckles file, as we need it not only
		// for visualization purposes, but also for re-doing despeckling at
		// different levels without going through the whole output generation process.
		bool const write_automask = render_params.mixedOutput();
		bool const write_speckles_file = params.despeckleLevel() != DESPECKLE_OFF &&
			params.colorParams().colorMode() != ColorParams::COLOR_GRAYSCALE; 

		automask_img = BinaryImage();
		speckles_img = BinaryImage();

		out_img = generator.process(
			status, accel_ops, orig_image, gray_orig_image_factory,
			new_picture_zones, new_fill_zones,
			write_automask ? &automask_img : nullptr,
			write_speckles_file ? &speckles_img : nullptr,
			m_ptrDbg.get()
		);

		if (write_speckles_file && speckles_img.isNull()) {
			// Even if despeckling didn't actually take place, we still need
			// to write an empty speckles file.  Making it a special case
			// is simply not worth it.
			BinaryImage(out_img.size(), WHITE).swap(speckles_img);
		}

		bool invalidate_params = false;
		
		if (!TiffWriter::writeImage(out_file_path, out_img)) {
			invalidate_params = true;
		} else {
			deleteMutuallyExclusiveOutputFiles();
		}

		if (write_automask) {
			// Note that QDir::mkdir() will fail if the parent directory,
			// that is $OUT/cache doesn't exist. We want that behaviour,
			// as otherwise when loading a project from a different machine,
			// a whole bunch of bogus directories would be created.
			QDir().mkdir(automask_dir);
			// Also note that QDir::mkdir() will fail if the directory already exists,
			// so we ignore its return value here.

			if (!TiffWriter::writeImage(automask_file_path, automask_img.toQImage())) {
				invalidate_params = true;
			}
		}
		if (write_speckles_file) {
			if (!QDir().mkpath(speckles_dir)) {
				invalidate_params = true;
			} else if (!TiffWriter::writeImage(speckles_file_path, speckles_img.toQImage())) {
				invalidate_params = true;
			}
		}

		if (invalidate_params) {
			m_ptrSettings->removeOutputParams(m_pageId);
		} else {
			// Note that we can't reuse *_file_info objects
			// as we've just overwritten those files.
			OutputParams const out_params(
				new_output_image_params,
				OutputFileParams(QFileInfo(out_file_path)),
				write_automask ? OutputFileParams(QFileInfo(automask_file_path))
				: OutputFileParams(),
				write_speckles_file ? OutputFileParams(QFileInfo(speckles_file_path))
				: OutputFileParams(),
				new_picture_zones, new_fill_zones
			);

			m_ptrSettings->setOutputParams(m_pageId, out_params);
		}

		m_ptrThumbnailCache->recreateThumbnail(
			PageId(ImageId(out_file_path)),
			out_img, AffineImageTransform(generator.outputImageSize())
		);
	}

	DespeckleState const despeckle_state(
		out_img, speckles_img, params.despeckleLevel()
	);

	DespeckleVisualization despeckle_visualization;
	if (m_lastTab == TAB_DESPECKLING) {
		// Because constructing DespeckleVisualization takes a noticeable
		// amount of time, we only do it if we are sure we'll need it.
		// Otherwise it will get constructed on demand.
		despeckle_visualization = despeckle_state.visualize(accel_ops);
	}

	if (!CommandLine::get().isGui()) {
		return FilterResultPtr();
	}

	QRect const out_rect(generator.outputImageRect());

	auto transform_orig_image = [orig_image, orig_image_transform, out_rect, accel_ops]() {
		return orig_image_transform->materialize(orig_image, out_rect, Qt::transparent, accel_ops);
	};
	auto cached_transform_orig_image = cachingFactory<QImage>(transform_orig_image);

	auto downscaled_transform_orig_image = [cached_transform_orig_image, accel_ops]() {
		return ImageViewBase::createDownscaledImage(cached_transform_orig_image(), accel_ops);
	};
	auto cached_downscaled_transform_orig_image = cachingFactory<QImage>(
		downscaled_transform_orig_image
	);

	if (m_lastTab == TAB_PICTURE_ZONES) {
		// Because building a transformation of the original image takes
		// a noticeable amount of time, we only do it if we are sure we'll need it.
		// Otherwise it will get constructed on demand.
		cached_transform_orig_image();
		cached_downscaled_transform_orig_image();
	}

	return FilterResultPtr(
		new UiUpdater(
			m_ptrFilter, accel_ops, m_ptrSettings, m_ptrDbg, params,
			m_pageId, orig_image, out_img,
			generator.origToOutputMapper(),
			generator.outputToOrigMapper(),
			automask_img, cached_transform_orig_image,
			cached_downscaled_transform_orig_image,
			despeckle_state, despeckle_visualization,
			m_batchProcessing, m_debug
		)
	);
}

/**
 * Delete output files mutually exclusive to m_pageId.
 */
void
Task::deleteMutuallyExclusiveOutputFiles()
{
	switch (m_pageId.subPage()) {
		case PageId::SINGLE_PAGE:
			QFile::remove(
				m_outFileNameGen.filePathFor(
					PageId(m_pageId.imageId(), PageId::LEFT_PAGE)
				)
			);
			QFile::remove(
				m_outFileNameGen.filePathFor(
					PageId(m_pageId.imageId(), PageId::RIGHT_PAGE)
				)
			);
			break;
		case PageId::LEFT_PAGE:
		case PageId::RIGHT_PAGE:
			QFile::remove(
				m_outFileNameGen.filePathFor(
					PageId(m_pageId.imageId(), PageId::SINGLE_PAGE)
				)
			);
			break;
	}
}


/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	IntrusivePtr<Settings> const& settings,
	std::auto_ptr<DebugImagesImpl> dbg_img,
	Params const& params,
	PageId const& page_id,
	QImage const& orig_image,
	QImage const& output_image,
	std::function<QPointF(QPointF const&)> const& orig_to_output,
	std::function<QPointF(QPointF const&)> const& output_to_orig,
	BinaryImage const& picture_mask,
	CachingFactory<QImage> const& cached_transformed_orig_image,
	CachingFactory<QImage> const& cached_downscaled_transformed_orig_image,
	DespeckleState const& despeckle_state,
	DespeckleVisualization const& despeckle_visualization,
	bool const batch, bool const debug)
:	m_ptrFilter(filter),
	m_ptrAccelOps(accel_ops),
	m_ptrSettings(settings),
	m_ptrDbg(dbg_img),
	m_params(params),
	m_pageId(page_id),
	m_origImage(orig_image),
	m_outputImage(output_image),
	m_origToOutput(orig_to_output),
	m_outputToOrig(output_to_orig),
	m_downscaledOutputImage(ImageViewBase::createDownscaledImage(output_image, accel_ops)),
	m_pictureMask(picture_mask),
	m_cachedTransformedOrigImage(cached_transformed_orig_image),
	m_cachedDownscaledTransformedOrigImage(cached_downscaled_transformed_orig_image),
	m_despeckleState(despeckle_state),
	m_despeckleVisualization(despeckle_visualization),
	m_batchProcessing(batch),
	m_debug(debug)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.
	
	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	opt_widget->postUpdateUI(m_outputImage.size());
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);
	
	ui->invalidateThumbnail(m_pageId);
	
	if (m_batchProcessing) {
		return;
	}

	std::auto_ptr<ImageViewBase> image_view(
		new BasicImageView(
			m_ptrAccelOps, m_outputImage,
			m_downscaledOutputImage, OutputMargins()
		)
	);

	std::auto_ptr<QWidget> picture_zone_editor;
	if (m_pictureMask.isNull()) {
		picture_zone_editor.reset(
			new ErrorWidget(tr("Picture zones are only available in Mixed mode."))
		);
	} else {
		picture_zone_editor.reset(
			new OnDemandPictureZoneEditor(
				m_ptrAccelOps,
				m_cachedTransformedOrigImage,
				m_cachedDownscaledTransformedOrigImage,
				m_pictureMask, m_pageId, m_ptrSettings,
				m_origToOutput, m_outputToOrig
			)
		);
		QObject::connect(
			picture_zone_editor.get(), SIGNAL(invalidateThumbnail(PageId const&)),
			opt_widget, SIGNAL(invalidateThumbnail(PageId const&))
		);
	}

	std::unique_ptr<QWidget> fill_zone_editor(
		new FillZoneEditor(
			m_ptrAccelOps, m_outputImage, m_downscaledOutputImage,
			m_origToOutput, m_outputToOrig, m_pageId, m_ptrSettings
		)
	);
	QObject::connect(
		fill_zone_editor.get(), SIGNAL(invalidateThumbnail(PageId const&)),
		opt_widget, SIGNAL(invalidateThumbnail(PageId const&))
	);

	std::auto_ptr<QWidget> despeckle_view;
	if (m_params.colorParams().colorMode() == ColorParams::COLOR_GRAYSCALE) {
		despeckle_view.reset(
			new ErrorWidget(tr("Despeckling can't be done in Color / Grayscale mode."))
		);
	} else {
		despeckle_view.reset(
			new DespeckleView(
				m_ptrAccelOps, m_despeckleState, m_despeckleVisualization, m_debug
			)
		);
		QObject::connect(
			opt_widget, SIGNAL(despeckleLevelChanged(DespeckleLevel, bool*)),
			despeckle_view.get(), SLOT(despeckleLevelChanged(DespeckleLevel, bool*))
		);
	}

	std::auto_ptr<TabbedImageView> tab_widget(new TabbedImageView);
	tab_widget->setDocumentMode(true);
	tab_widget->setTabPosition(QTabWidget::East);
	tab_widget->addTab(image_view.release(), tr("Output"), TAB_OUTPUT);
	tab_widget->addTab(picture_zone_editor.release(), tr("Picture Zones"), TAB_PICTURE_ZONES);
	tab_widget->addTab(fill_zone_editor.release(), tr("Fill Zones"), TAB_FILL_ZONES);
	tab_widget->addTab(despeckle_view.release(), tr("Despeckling"), TAB_DESPECKLING);
	tab_widget->setCurrentTab(opt_widget->lastTab());

	QObject::connect(
		tab_widget.get(), SIGNAL(tabChanged(ImageViewTab)),
		opt_widget, SLOT(tabChanged(ImageViewTab))
	);

	ui->setImageWidget(tab_widget.release(), ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
}

} // namespace output
