/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    ConsoleBatch - Batch processing scanned pages from command line.
    Copyright (C) 2011 Petr Kovar <pejuko@gmail.com>

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

#include <vector>
#include <iostream>
#include <cmath>
#include <assert.h>

#include "Utils.h"
#include "ProjectPages.h"
#include "PageSelectionAccessor.h"
#include "StageSequence.h"
#include "ProcessingTaskQueue.h"
#include "FileNameDisambiguator.h"
#include "OutputFileNameGenerator.h"
#include "ImageInfo.h"
#include "ImageFileInfo.h"
#include "PageInfo.h"
#include "PageSequence.h"
#include "ImageId.h"
#include "ThumbnailPixmapCache.h"
#include "LoadFileTask.h"
#include "ProjectWriter.h"
#include "ProjectReader.h"
#include "OrthogonalRotation.h"
#include "SelectedPage.h"
#include "ImageLoader.h"
#include "ImageTransformation.h"
#include "ContentBoxCollector.h"
#include "settings/ini_keys.h"

#include "filters/fix_orientation/Settings.h"
#include "filters/fix_orientation/Filter.h"
#include "filters/fix_orientation/Task.h"
#include "filters/fix_orientation/CacheDrivenTask.h"
#include "filters/page_split/Settings.h"
#include "filters/page_split/Filter.h"
#include "filters/page_split/Task.h"
#include "filters/page_split/CacheDrivenTask.h"
#include "filters/deskew/Settings.h"
#include "filters/deskew/Filter.h"
#include "filters/deskew/Task.h"
#include "filters/deskew/CacheDrivenTask.h"
#include "filters/select_content/Settings.h"
#include "filters/select_content/Filter.h"
#include "filters/select_content/Task.h"
#include "filters/select_content/CacheDrivenTask.h"
#include "filters/page_layout/Settings.h"
#include "filters/page_layout/Filter.h"
#include "filters/page_layout/Task.h"
#include "filters/page_layout/CacheDrivenTask.h"
#include "filters/output/Settings.h"
#include "filters/output/Params.h"
#include "filters/output/Filter.h"
#include "filters/output/Task.h"
#include "filters/output/CacheDrivenTask.h"

#include <QMap>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QDir>
#include <QFileInfo>
#include <QDomDocument>

#include "ConsoleBatch.h"
#include "CommandLine.h"

namespace
{

class PreviewCollector : public ContentBoxCollector
{
public:
    PreviewCollector()
        :   m_xform(QRectF(0, 0, 1, 1), Dpi(300, 300)),
            m_collected(false)
    {
    }

    virtual void process(ImageTransformation const& xform, QRectF const& content_rect)
    {
        m_xform = xform;
        m_contentRect = content_rect;
        m_collected = true;
    }

    bool collected() const
    {
        return m_collected;
    }

    ImageTransformation const& xform() const
    {
        return m_xform;
    }

    QRectF const& contentRect() const
    {
        return m_contentRect;
    }
private:
    ImageTransformation m_xform;
    QRectF m_contentRect;
    bool m_collected;
};

// Draws the boxes as select_content::ImageView::onPaint() does, to show the page as the GUI shows it.
void drawPreviewBoxes(
    QPainter& painter, QRectF const& display_rect,
    QRectF const& page_rect, QRectF const& content_rect, QColor const& content_color)
{
    if (content_rect.isNull()) {
        return;
    }

    if (!page_rect.isNull() && page_rect != display_rect) {
        QPen pen(QColor(0xee, 0xee, 0x00, 0xcc));
        pen.setWidth(1);
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.setBrush(QColor(0xee, 0xee, 0x00, 0xcc));

        QRectF const& r = display_rect;
        painter.drawRect(QRectF(QPointF(page_rect.left(), r.top()), QPointF(page_rect.right(), page_rect.top())));
        painter.drawRect(QRectF(QPointF(r.left(), r.top()), QPointF(page_rect.left(), r.bottom())));
        painter.drawRect(QRectF(QPointF(page_rect.left(), page_rect.bottom()), QPointF(page_rect.right(), r.bottom())));
        painter.drawRect(QRectF(QPointF(page_rect.right(), r.top()), QPointF(r.right(), r.bottom())));
    }

    painter.setRenderHints(QPainter::Antialiasing, true);

    QColor pen_color(content_color);
    pen_color.setAlpha(255);
    QPen pen(pen_color);
    pen.setWidth(1);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setBrush(content_color);
    painter.drawRect(content_rect);
}

QImage renderPreview(
    QImage const& image, ImageTransformation const& xform,
    QRectF const& page_rect, QRectF const& content_rect,
    QColor const& content_color, int const max_size)
{
    QPolygonF const crop_area(xform.resultingPreCropArea());
    QRectF const display_rect(crop_area.boundingRect());
    double const long_side = std::max(display_rect.width(), display_rect.height());
    double const scale = long_side > max_size ? max_size / long_side : 1.0;

    QImage preview(
        std::max(1, qRound(display_rect.width() * scale)),
        std::max(1, qRound(display_rect.height() * scale)),
        QImage::Format_RGB32
    );
    preview.fill(QColor(0xef, 0xef, 0xef));

    QTransform virtual_to_preview;
    virtual_to_preview.scale(scale, scale);
    virtual_to_preview.translate(-display_rect.left(), -display_rect.top());

    // Scale the image smoothly first, because a direct transform of a large scan gives jagged lines.
    QTransform const& t = xform.transform();
    double const scale_x = std::min(1.0, scale * std::hypot(t.m11(), t.m12()));
    double const scale_y = std::min(1.0, scale * std::hypot(t.m21(), t.m22()));
    QImage const small_image(
        (scale_x < 1.0 || scale_y < 1.0) ? image.scaled(
            std::max(1, qRound(image.width() * scale_x)),
            std::max(1, qRound(image.height() * scale_y)),
            Qt::IgnoreAspectRatio, Qt::SmoothTransformation
        ) : image
    );
    QTransform const pixmap_to_image(QTransform::fromScale(
        double(image.width()) / small_image.width(),
        double(image.height()) / small_image.height()
    ));

    QPainter painter(&preview);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPolygonF const image_area(xform.transform().map(QPolygonF(QRectF(image.rect()))));
    QPainterPath clip_path;
    clip_path.addPolygon(image_area.intersected(crop_area));
    painter.setWorldTransform(virtual_to_preview);
    painter.setClipPath(clip_path);

    painter.setWorldTransform(pixmap_to_image * xform.transform() * virtual_to_preview);
    painter.drawImage(QPointF(0, 0), small_image);

    painter.setClipping(false);
    painter.setWorldTransform(virtual_to_preview);
    drawPreviewBoxes(painter, display_rect, page_rect, content_rect, content_color);
    painter.end();

    return preview;
}

} // namespace

ConsoleBatch::ConsoleBatch(std::vector<ImageFileInfo> const& images, QString const& output_directory, Qt::LayoutDirection const layout)
    :   batch(true), debug(true),
        m_ptrDisambiguator(new FileNameDisambiguator),
        m_ptrPages(new ProjectPages(images, ProjectPages::AUTO_PAGES, layout))
{
    PageSelectionAccessor const accessor((IntrusivePtr<PageSelectionProvider>())); // Won't really be used anyway.
    m_ptrStages = IntrusivePtr<StageSequence>(new StageSequence(m_ptrPages, accessor));

    //m_ptrThumbnailCache = IntrusivePtr<ThumbnailPixmapCache>(new ThumbnailPixmapCache(output_dir+"/cache/thumbs", QSize(200,200), 40, 5));
    m_ptrThumbnailCache = Utils::createThumbnailCache(output_directory);
    m_outFileNameGen = OutputFileNameGenerator(m_ptrDisambiguator, output_directory, m_ptrPages->layoutDirection());
}

ConsoleBatch::ConsoleBatch(QString const project_file)
    :   batch(true), debug(true)
{
    QFile file(project_file);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Unable to open the project file.");
    }

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        throw std::runtime_error("The project file is broken.");
    }

    file.close();

    m_ptrReader.reset(new ProjectReader(doc));
    m_ptrPages = m_ptrReader->pages();

    PageSelectionAccessor const accessor((IntrusivePtr<PageSelectionProvider>())); // Won't be used anyway.
    m_ptrDisambiguator = m_ptrReader->namingDisambiguator();

    m_ptrStages = IntrusivePtr<StageSequence>(new StageSequence(m_ptrPages, accessor));
    m_ptrReader->readFilterSettings(m_ptrStages->filters());

    CommandLine const& cli = CommandLine::get();
    QString output_directory = m_ptrReader->outputDirectory();
    if (!cli.outputDirectory().isEmpty()) {
        output_directory = cli.outputDirectory();
    }

    //m_ptrThumbnailCache = IntrusivePtr<ThumbnailPixmapCache>(new ThumbnailPixmapCache(output_directory+"/cache/thumbs", QSize(200,200), 40, 5));
    m_ptrThumbnailCache = Utils::createThumbnailCache(output_directory);
    m_outFileNameGen = OutputFileNameGenerator(m_ptrDisambiguator, output_directory, m_ptrPages->layoutDirection());
}

BackgroundTaskPtr
ConsoleBatch::createCompositeTask(
    PageInfo const& page,
    int const last_filter_idx)
{
    IntrusivePtr<fix_orientation::Task> fix_orientation_task;
    IntrusivePtr<page_split::Task> page_split_task;
    IntrusivePtr<deskew::Task> deskew_task;
    IntrusivePtr<select_content::Task> select_content_task;
    IntrusivePtr<page_layout::Task> page_layout_task;
    IntrusivePtr<output::Task> output_task;

    if (batch) {
        debug = false;
    }

    if (last_filter_idx >= m_ptrStages->outputFilterIdx()) {
        output_task = m_ptrStages->outputFilter()->createTask(
                          page.id(), m_ptrThumbnailCache, m_outFileNameGen, batch, debug
                      );
        debug = false;
    }
    if (last_filter_idx >= m_ptrStages->pageLayoutFilterIdx()) {
        page_layout_task = m_ptrStages->pageLayoutFilter()->createTask(
                               page.id(), output_task, batch, debug
                           );
        debug = false;
    }
    if (last_filter_idx >= m_ptrStages->selectContentFilterIdx()) {
        select_content_task = m_ptrStages->selectContentFilter()->createTask(
                                  page.id(), page_layout_task, batch, debug
                              );
        debug = false;
    }
    if (last_filter_idx >= m_ptrStages->deskewFilterIdx()) {
        deskew_task = m_ptrStages->deskewFilter()->createTask(
                          page.id(), select_content_task, batch, debug
                      );
        debug = false;
    }
    if (last_filter_idx >= m_ptrStages->pageSplitFilterIdx()) {
        page_split_task = m_ptrStages->pageSplitFilter()->createTask(
                              page, deskew_task, batch, debug
                          );
        debug = false;
    }
    if (last_filter_idx >= m_ptrStages->fixOrientationFilterIdx()) {
        fix_orientation_task = m_ptrStages->fixOrientationFilter()->createTask(
                                   page.id(), page_split_task, batch
                               );
        debug = false;
    }
    assert(fix_orientation_task);

    return BackgroundTaskPtr(
               new LoadFileTask(
                   BackgroundTask::BATCH,
                   page, m_ptrThumbnailCache, m_ptrPages, fix_orientation_task
               )
           );
}

// process the image vector **images** and save output to **output_dir**
void
ConsoleBatch::process()
{
    CommandLine const& cli = CommandLine::get();

    // get first filter id
    int startFilterIdx = m_ptrStages->fixOrientationFilterIdx();
    if (cli.hasStartFilterIdx()) {
        unsigned int sf = cli.getStartFilterIdx();
        if (sf >= m_ptrStages->filters().size()) {
            throw std::runtime_error("Start filter out of range");
        }
        startFilterIdx = sf;
    }

    // get last filter id
    int endFilterIdx = m_ptrStages->outputFilterIdx();
    if (cli.hasEndFilterIdx()) {
        unsigned int ef = cli.getEndFilterIdx();
        if (ef >= m_ptrStages->filters().size()) {
            throw std::runtime_error("End filter out of range");
        }
        endFilterIdx = ef;
    }

    // run filters
    for (int j = startFilterIdx; j <= endFilterIdx; j++) {
        if (cli.isVerbose()) {
            std::cout << "Filter: " << (j + 1) << "\n";
        }

        // process pages
        PageSequence page_sequence = m_ptrPages->toPageSequence(PAGE_VIEW);
        setupFilter(j, page_sequence.asPageIdSet());
        for (const PageInfo& page : page_sequence) {
            if (cli.isVerbose()) {
                std::cout << "\tProcessing: " << page.imageId().filePath().toLocal8Bit().constData() << "\n";
            }
            BackgroundTaskPtr bgTask = createCompositeTask(page, j);
            (*bgTask)();
        }
    }

    // setup rest filters with params from cli
    const std::set<PageId> select_all = m_ptrPages->toPageSequence(PAGE_VIEW).asPageIdSet();
    for (int j = endFilterIdx + 1; j <= m_ptrStages->count(); j++) {
        setupFilter(j, select_all);
    }

    // update statistics for executed filters
    for (int j = 0; j <= endFilterIdx; j++) {
        m_ptrStages->filterAt(j)->updateStatistics();
    }
}

int
ConsoleBatch::renderPreviews(QString const& directory, int const max_size)
{
    CommandLine const& cli = CommandLine::get();
    QDir const dir(directory);
    if (!dir.mkpath(".")) {
        throw std::runtime_error("Unable to create the preview directory.");
    }

    // Use the GUI cache tasks, and give select_content a next task, because without it the task changes the thumbnail flags.
    IntrusivePtr<select_content::CacheDrivenTask> const select_content_task(
        m_ptrStages->selectContentFilter()->createCacheDrivenTask(
            m_ptrStages->pageLayoutFilter()->createCacheDrivenTask(
                IntrusivePtr<output::CacheDrivenTask>()
            )
        )
    );
    IntrusivePtr<fix_orientation::CacheDrivenTask> const task(
        m_ptrStages->fixOrientationFilter()->createCacheDrivenTask(
            m_ptrStages->pageSplitFilter()->createCacheDrivenTask(
                m_ptrStages->deskewFilter()->createCacheDrivenTask(select_content_task)
            )
        )
    );

    QSettings const settings;
    QColor const content_color(
        settings.value(_key_content_sel_content_color, _key_content_sel_content_color_def).toString()
    );

    int failures = 0;
    ImageId loaded_image_id;
    QImage image;
    for (PageInfo const& page : m_ptrPages->toPageSequence(PAGE_VIEW)) {
        QString const file_name(
            QFileInfo(m_outFileNameGen.fileNameFor(page.id())).completeBaseName() + ".png"
        );

        PreviewCollector collector;
        task->process(page, &collector);
        if (!collector.collected()) {
            std::cerr << "No preview for " << file_name.toLocal8Bit().constData()
                      << ": filters 1-4 are not complete for this page." << std::endl;
            ++failures;
            continue;
        }

        QRectF page_rect;
        std::unique_ptr<select_content::Params> const params(
            m_ptrStages->selectContentFilter()->getSettings()->getPageParams(page.id())
        );
        if (params.get()) {
            page_rect = params->pageRect();
        }

        // The two pages of a split image use the same image, thus load it one time only.
        if (!(loaded_image_id == page.imageId())) {
            image = ImageLoader::load(page.imageId());
            loaded_image_id = page.imageId();
        }
        if (image.isNull()) {
            std::cerr << "Unable to load " << page.imageId().filePath().toLocal8Bit().constData() << std::endl;
            ++failures;
            continue;
        }

        QImage const preview(
            renderPreview(image, collector.xform(), page_rect, collector.contentRect(), content_color, max_size)
        );
        QString const path(dir.filePath(file_name));
        if (!preview.save(path, "PNG")) {
            std::cerr << "Unable to write " << path.toLocal8Bit().constData() << std::endl;
            ++failures;
        } else if (cli.isVerbose()) {
            std::cout << "Preview: " << path.toLocal8Bit().constData() << "\n";
        }
    }

    return failures;
}

void
ConsoleBatch::saveProject(QString const project_file)
{
    PageInfo fpage = m_ptrPages->toPageSequence(PAGE_VIEW).pageAt(0);
    SelectedPage sPage(fpage.id(), IMAGE_VIEW);
    ProjectWriter writer(m_ptrPages, sPage, m_outFileNameGen);
    writer.write(project_file, m_ptrStages->filters());
}

void
ConsoleBatch::setupFilter(int idx, std::set<PageId> allPages)
{
    if (idx == m_ptrStages->fixOrientationFilterIdx()) {
        setupFixOrientation(allPages);
    } else if (idx == m_ptrStages->pageSplitFilterIdx()) {
        setupPageSplit(allPages);
    } else if (idx == m_ptrStages->deskewFilterIdx()) {
        setupDeskew(allPages);
    } else if (idx == m_ptrStages->selectContentFilterIdx()) {
        setupSelectContent(allPages);
    } else if (idx == m_ptrStages->pageLayoutFilterIdx()) {
        setupPageLayout(allPages);
    } else if (idx == m_ptrStages->outputFilterIdx()) {
        setupOutput(allPages);
    }
}

void
ConsoleBatch::setupFixOrientation(std::set<PageId> allPages)
{
    IntrusivePtr<fix_orientation::Filter> fix_orientation = m_ptrStages->fixOrientationFilter();
    CommandLine const& cli = CommandLine::get();

    for (PageId const& page : allPages) {

        OrthogonalRotation rotation;
        // FIX ORIENTATION FILTER
        if (cli.hasOrientation()) {
            switch (cli.getOrientation()) {
            case CommandLine::LEFT:
                rotation.prevClockwiseDirection();
                break;
            case CommandLine::RIGHT:
                rotation.nextClockwiseDirection();
                break;
            case CommandLine::UPSIDEDOWN:
                rotation.nextClockwiseDirection();
                rotation.nextClockwiseDirection();
                break;
            default:
                break;
            }
            fix_orientation->getSettings()->applyRotation(page.imageId(), rotation);
        }
    }
}

void
ConsoleBatch::setupPageSplit(std::set<PageId> allPages)
{
    IntrusivePtr<page_split::Filter> page_split = m_ptrStages->pageSplitFilter();
    CommandLine const& cli = CommandLine::get();

    // PAGE SPLIT
    if (cli.hasLayout()) {
        page_split->getSettings()->setLayoutTypeForAllPages(cli.getLayout());
    }
}

void
ConsoleBatch::setupDeskew(std::set<PageId> allPages)
{
    IntrusivePtr<deskew::Filter> deskew = m_ptrStages->deskewFilter();
    CommandLine const& cli = CommandLine::get();

    for (PageId const& page : allPages) {

        // DESKEW FILTER
        OrthogonalRotation rotation;
        if (cli.hasDeskewAngle() || cli.hasDeskew()) {
            double angle = 0.0;
            if (cli.hasDeskewAngle()) {
                angle = cli.getDeskewAngle();
            }
            deskew::Dependencies deps(QPolygonF(), rotation);
            deskew::Params params(angle, deps, MODE_MANUAL);
            deskew->getSettings()->setPageParams(page, params);
        }
    }

    if (cli.hasSkewDeviation()) {
        deskew->getSettings()->setMaxDeviation(cli.getSkewDeviation());
    }
}

void
ConsoleBatch::setupSelectContent(std::set<PageId> allPages)
{
    IntrusivePtr<select_content::Filter> select_content = m_ptrStages->selectContentFilter();
    CommandLine const& cli = CommandLine::get();

    for (PageId const& page : allPages) {
        select_content::Dependencies deps;

        select_content::Params params(deps);
        std::unique_ptr<select_content::Params> old_params = select_content->getSettings()->getPageParams(page);

        if (old_params.get()) {
            params = *old_params;
        }

        // SELECT CONTENT FILTER
        if (cli.hasContentRect()) {
            params.setContentRect(cli.getContentRect());
            //QRectF rect(cli.getContentRect());
            //QSizeF size_mm(rect.width(), rect.height());
            //select_content::Params params(rect, size_mm, deps, MODE_MANUAL);
        }

        params.setContentDetect(cli.isContentDetectionEnabled());
        params.setPageDetect(cli.isPageDetectionEnabled());
        params.setFineTuneCorners(cli.isFineTuningEnabled());
        if (cli.hasPageBorders()) {
            params.setPageBorders(cli.getPageBorders());
        }

        select_content->getSettings()->setPageParams(page, params);
    }

    if (cli.hasContentDeviation()) {
        select_content->getSettings()->setMaxDeviation(cli.getContentDeviation());
    }

    if (cli.hasPageDetectionBox()) {
        select_content->getSettings()->setPageDetectionBox(cli.getPageDetectionBox());
    }

    if (cli.hasPageDetectionTolerance()) {
        select_content->getSettings()->setPageDetectionTolerance(cli.getPageDetectionTolerance());
    }
}

void
ConsoleBatch::setupPageLayout(std::set<PageId> allPages)
{
    IntrusivePtr<page_layout::Filter> page_layout = m_ptrStages->pageLayoutFilter();
    CommandLine const& cli = CommandLine::get();
    QMap<QString, float> img_cache;

    for (PageId const& page : allPages) {
        // PAGE LAYOUT FILTER
        page_layout::Alignment alignment = cli.getAlignment();
        if (cli.hasMatchLayoutTolerance()) {
            QString const path = page.imageId().filePath();
            if (!img_cache.contains(path)) {
                QImage img(path);
                img_cache[path] = float(img.width()) / float(img.height());
            }
            float imgAspectRatio = img_cache[path];
            float tolerance = cli.getMatchLayoutTolerance();
            std::vector<float> diffs;
            for (PageId const& page : allPages) {
                ImageId pimageId = page.imageId();
                QString ppath = pimageId.filePath();
                if (!img_cache.contains(ppath)) {
                    QImage img(ppath);
                    img_cache[ppath] = float(img.width()) / float(img.height());
                }
                float pimgAspectRatio = img_cache[ppath];
                float diff = imgAspectRatio - pimgAspectRatio;
                if (diff < 0.0) {
                    diff *= -1;
                }
                diffs.push_back(diff);
            }
            unsigned bad_diffs = 0;
            for (unsigned j = 0; j < diffs.size(); j++) {
                if (diffs[j] > tolerance) {
                    bad_diffs += 1;
                }
            }
            if (bad_diffs > (diffs.size() / 2)) {
                alignment.setNull(true);
            }
        }
        if (cli.hasMargins()) {
            page_layout->getSettings()->setHardMarginsMM(page, MarginsWithAuto(cli.getMargins()));
        }
        if (cli.hasAlignment()) {
            page_layout->getSettings()->setPageAlignment(page, alignment);
        }
    }
}

void
ConsoleBatch::setupOutput(std::set<PageId> allPages)
{
    IntrusivePtr<output::Filter> output = m_ptrStages->outputFilter();
    CommandLine const& cli = CommandLine::get();

    for (PageId const& page : allPages) {

        // OUTPUT FILTER
        output::Params params(output->getSettings()->getParams(page));
        if (cli.hasOutputDpi()) {
            Dpi outputDpi = cli.getOutputDpi();
            params.setOutputDpi(outputDpi);
        }

        output::ColorParams colorParams = params.colorParams();
        if (cli.hasColorMode()) {
            colorParams.setColorMode(cli.getColorMode());
        }

        if (cli.hasWhiteMargins() || cli.hasNormalizeIllumination() ||
                cli.hasAutoLayer() || cli.hasPictureZonesLayer() || cli.hasForegroundLayer()) {
            output::ColorGrayscaleOptions cgo;
            if (cli.hasWhiteMargins()) {
                cgo.setWhiteMargins(true);
            }
            if (cli.hasNormalizeIllumination()) {
                cgo.setNormalizeIllumination(true);
            }
            if (cli.hasAutoLayer()) {
                cgo.setAutoLayerEnabled(true);
            }
            if (cli.hasPictureZonesLayer()) {
                cgo.setPictureZonesLayerEnabled(true);
            }
            if (cli.hasForegroundLayer()) {
                cgo.setForegroundLayerEnabled(true);
            }
            colorParams.setColorGrayscaleOptions(cgo);
        }

        if (cli.hasThreshold()) {
            output::BlackWhiteOptions bwo;
            bwo.setThresholdAdjustment(cli.getThreshold());
            colorParams.setBlackWhiteOptions(bwo);
        }

        params.setColorParams(colorParams);

        if (cli.hasDespeckle()) {
            params.setDespeckleLevel(cli.getDespeckleLevel());
        }

        if (cli.hasDewarping()) {
            params.setDewarpingMode(cli.getDewarpingMode());
        }
        if (cli.hasDepthPerception()) {
            params.setDepthPerception(cli.getDepthPerception());
        }

        if (cli.hasTiffCompressionBW()) {
            GlobalStaticSettings::setTiffCompressionBW(cli.getTiffCompressionBW());
        }

        if (cli.hasTiffCompressionColor()) {
            GlobalStaticSettings::setTiffCompressionColor(cli.getTiffCompressionColor());
        }

        output->getSettings()->setParams(page, params);
    }

}
