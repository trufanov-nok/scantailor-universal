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
#include <QDomDocument>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstdio>

#include "ConsoleBatch.h"
#include "CommandLine.h"

ConsoleBatch::ConsoleBatch(std::vector<ImageFileInfo> const& images, QString const& output_directory, Qt::LayoutDirection const layout)
    :   batch(true), debug(true), m_pageErrors(0),
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
    :   batch(true), debug(true), m_pageErrors(0)
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

    bool const json = cli.isJsonProgress();

    // run filters
    for (int j = startFilterIdx; j <= endFilterIdx; j++) {
        if (cli.isVerbose()) {
            std::cout << "Filter: " << (j + 1) << "\n";
        }

        // process pages
        PageSequence page_sequence = m_ptrPages->toPageSequence(PAGE_VIEW);
        setupFilter(j, page_sequence.asPageIdSet());

        int const num_pages = int(page_sequence.numPages());
        if (json) {
            QJsonObject ev;
            ev["event"] = QStringLiteral("stage_start");
            ev["stage"] = j + 1;
            ev["name"] = stageName(j);
            ev["pages"] = num_pages;
            printJsonLine(ev);
        }

        int page_index = 0;
        for (const PageInfo& page : page_sequence) {
            if (cli.isVerbose()) {
                std::cout << "\tProcessing: " << page.imageId().filePath().toLocal8Bit().constData() << "\n";
            }

            QJsonObject ev;
            if (json) {
                ev["stage"] = j + 1;
                ev["name"] = stageName(j);
                ev["index"] = page_index;
                ev["pages"] = num_pages;
                ev["image"] = QFileInfo(page.imageId().filePath()).fileName();
                if (page.imageId().isMultiPageFile()) {
                    ev["image_page"] = page.imageId().page();
                }
                ev["side"] = page.id().subPageAsString();
            }
            ++page_index;

            BackgroundTaskPtr bgTask = createCompositeTask(page, j);
            FilterResultPtr result;
            try {
                result = (*bgTask)();
            } catch (std::exception const& e) {
                ++m_pageErrors;
                if (json) {
                    ev["event"] = QStringLiteral("page_error");
                    ev["error"] = QString::fromLocal8Bit(e.what());
                    printJsonLine(ev);
                }
                throw;
            }

            // LoadFileTask returns a result without a filter when the image does not load, thus count a page error.
            bool const load_error = result && !result->filter();
            if (load_error) {
                ++m_pageErrors;
            }
            if (!json) {
                continue;
            }
            if (load_error) {
                ev["event"] = QStringLiteral("page_error");
                ev["error"] = QStringLiteral("cannot load the image");
            } else {
                ev["event"] = QStringLiteral("page_done");
                QJsonObject const res(pageResult(j, page.id()));
                for (QJsonObject::const_iterator it = res.constBegin(); it != res.constEnd(); ++it) {
                    ev.insert(it.key(), it.value());
                }
            }
            printJsonLine(ev);
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

bool
ConsoleBatch::saveProject(QString const project_file)
{
    PageInfo fpage = m_ptrPages->toPageSequence(PAGE_VIEW).pageAt(0);
    SelectedPage sPage(fpage.id(), IMAGE_VIEW);
    ProjectWriter writer(m_ptrPages, sPage, m_outFileNameGen);
    return writer.write(project_file, m_ptrStages->filters());
}

int
ConsoleBatch::pageCount() const
{
    return int(m_ptrPages->toPageSequence(PAGE_VIEW).numPages());
}

void
ConsoleBatch::printJsonLine(QJsonObject const& obj)
{
    // The JSON goes to the C stdout stream because the CLI sends std::cout text to stderr in this mode.
    QByteArray const line(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    std::fwrite(line.constData(), 1, size_t(line.size()), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

QString
ConsoleBatch::stageName(int idx) const
{
    if (idx == m_ptrStages->fixOrientationFilterIdx()) {
        return QStringLiteral("fix_orientation");
    } else if (idx == m_ptrStages->pageSplitFilterIdx()) {
        return QStringLiteral("page_split");
    } else if (idx == m_ptrStages->deskewFilterIdx()) {
        return QStringLiteral("deskew");
    } else if (idx == m_ptrStages->selectContentFilterIdx()) {
        return QStringLiteral("select_content");
    } else if (idx == m_ptrStages->pageLayoutFilterIdx()) {
        return QStringLiteral("page_layout");
    } else if (idx == m_ptrStages->outputFilterIdx()) {
        return QStringLiteral("output");
    }
    return QString();
}

static QJsonObject
rectToJson(QRectF const& rect)
{
    QJsonObject obj;
    obj["x"] = rect.x();
    obj["y"] = rect.y();
    obj["width"] = rect.width();
    obj["height"] = rect.height();
    return obj;
}

static QJsonObject
lineToJson(QLineF const& line)
{
    QJsonObject obj;
    obj["x1"] = line.x1();
    obj["y1"] = line.y1();
    obj["x2"] = line.x2();
    obj["y2"] = line.y2();
    return obj;
}

static QJsonObject
marginsToJson(Margins const& margins)
{
    QJsonObject obj;
    obj["left"] = margins.left();
    obj["top"] = margins.top();
    obj["right"] = margins.right();
    obj["bottom"] = margins.bottom();
    return obj;
}

static QString
modeToString(AutoManualMode mode)
{
    return mode == MODE_MANUAL ? QStringLiteral("manual") : QStringLiteral("auto");
}

QJsonObject
ConsoleBatch::pageResult(int idx, PageId const& page_id) const
{
    QJsonObject res;
    if (idx == m_ptrStages->fixOrientationFilterIdx()) {
        res["rotation"] = m_ptrStages->fixOrientationFilter()->getSettings()
                          ->getRotationFor(page_id.imageId()).toDegrees();
    } else if (idx == m_ptrStages->pageSplitFilterIdx()) {
        page_split::Settings::Record const record(
            m_ptrStages->pageSplitFilter()->getSettings()->getPageRecord(page_id.imageId()));
        page_split::Params const* params = record.params();
        if (params) {
            page_split::PageLayout const& layout = params->pageLayout();
            res["layout"] = page_split::layoutTypeToString(layout.toLayoutType());
            res["split_mode"] = modeToString(params->splitLineMode());
            res["sub_pages"] = layout.numSubPages();
            QJsonArray cutters;
            for (int i = 0; i < layout.numCutters(); ++i) {
                cutters.append(lineToJson(layout.cutterLine(i)));
            }
            res["cutters"] = cutters;
        } else {
            res["layout"] = page_split::layoutTypeToString(record.combinedLayoutType());
        }
    } else if (idx == m_ptrStages->deskewFilterIdx()) {
        std::unique_ptr<deskew::Params> const params(
            m_ptrStages->deskewFilter()->getSettings()->getPageParams(page_id));
        if (params) {
            res["angle"] = params->deskewAngle();
            res["mode"] = modeToString(params->mode());
        }
    } else if (idx == m_ptrStages->selectContentFilterIdx()) {
        std::unique_ptr<select_content::Params> const params(
            m_ptrStages->selectContentFilter()->getSettings()->getPageParams(page_id));
        if (params) {
            res["content_rect"] = rectToJson(params->contentRect());
            res["page_rect"] = rectToJson(params->pageRect());
            res["mode"] = modeToString(params->mode());
        }
    } else if (idx == m_ptrStages->pageLayoutFilterIdx()) {
        page_layout::Settings const* settings = m_ptrStages->pageLayoutFilter()->getSettings();
        page_layout::Alignment const alignment(settings->getPageAlignment(page_id));
        MarginsWithAuto const margins(settings->getHardMarginsMM(page_id));
        QJsonObject margins_json(marginsToJson(margins));
        margins_json["auto"] = margins.isAutoMarginsEnabled();
        res["margins_mm"] = margins_json;
        QJsonObject align;
        align["vert"] = page_layout::Alignment::verticalToStr(alignment.vertical());
        align["hor"] = page_layout::Alignment::horizontalToStr(alignment.horizontal());
        align["null"] = alignment.isNull();
        res["alignment"] = align;
    } else if (idx == m_ptrStages->outputFilterIdx()) {
        output::Params const params(m_ptrStages->outputFilter()->getSettings()->getParams(page_id));
        QString const out_path(m_outFileNameGen.filePathFor(page_id));
        res["output_file"] = out_path;
        res["dpi"] = params.outputDpi().horizontal();
        switch (params.colorParams().colorMode()) {
        case output::ColorParams::BLACK_AND_WHITE:
            res["color_mode"] = QStringLiteral("bw");
            break;
        case output::ColorParams::COLOR_GRAYSCALE:
            res["color_mode"] = QStringLiteral("colorOrGray");
            break;
        case output::ColorParams::MIXED:
            res["color_mode"] = QStringLiteral("mixed");
            break;
        }
    }
    return res;
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
