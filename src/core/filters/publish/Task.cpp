/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2020 Alexander Trufanov <trufanovan@gmail.com>

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
#include "ThumbnailPixmapCache.h"
#include "FilterData.h"
#include "ImageTransformation.h"
#include "TaskStatus.h"
#include "FilterUiInterface.h"
#include "OutputParams.h"
#include "DjbzDispatcher.h"
#include "PageSequence.h"
#include "CommandLine.h"
#include "ExportSuggestions.h"
#include "exporting/ExportThread.h"
#include "EncodingProgressInfo.h"
#include "DjVuBookmarkDispatcher.h"
#include "tesseract2djvused/hocr2djvused.h"
#include "ProcessingIndicationPropagator.h"

#include <QFileDialog>
#include <QTemporaryFile>
#include <QProcess>
#include <QTextStream>
#include <QDebug>
#include <omp.h>

#include <iostream>

using namespace exporting;

namespace publish
{

class QAutoTerminatedProcess: public QProcess
{
public:
    // don't leave process running in system
    // as it may take a lot of time (tesseract, minidjvu-mod..)
    ~QAutoTerminatedProcess() {
        if (state() != QProcess::NotRunning) {
            terminate();
        }
    }
};

class Task::UiUpdater : public FilterResult
{
public:
    UiUpdater(IntrusivePtr<Filter> const& filter, const PageId &page_id,
              const IntrusivePtr<DjbzDispatcher> &djbzDispatcher,
              bool show_foreground_settings,
              bool show_background_settings,
              bool batch_processing);

    virtual void updateUI(FilterUiInterface* wnd);

    virtual IntrusivePtr<AbstractFilter> filter()
    {
        return m_ptrFilter;
    }
private:
    IntrusivePtr<Filter> m_ptrFilter;
    PageId m_pageId;
    const IntrusivePtr<DjbzDispatcher> m_ptrDjbzDispatcher;
    bool m_show_foreground_settings;
    bool m_show_background_settings;
    bool m_batchProcessing;
};

Task::Task(PageId const& page_id,
           IntrusivePtr<Filter> const& filter,
           IntrusivePtr<Settings> const& settings,
           const IntrusivePtr<ThumbnailPixmapCache> &thumbnail_cache,
           const OutputFileNameGenerator &out_file_name_gen,
           bool const batch_processing)
    :
      m_pageId(page_id),
      m_ptrFilter(filter),
      m_ptrSettings(settings),
      m_ptrThumbnailCache(thumbnail_cache),
      m_refOutFileNameGen(out_file_name_gen),
      m_ptrDjbzDispatcher(m_ptrSettings->djbzDispatcher()),
      m_batchProcessing(batch_processing),
      m_ptrExportSuggestions(nullptr),

      m_out_path(m_refOutFileNameGen.outDir() + "/"),
      m_djvu_path( m_out_path + GlobalStaticSettings::m_djvu_pages_subfolder + "/"),
      m_export_path( m_djvu_path + GlobalStaticSettings::m_djvu_layers_subfolder + "/")
{
    QDir dir(m_djvu_path);
    if (!dir.exists()) {
        dir.mkpath(m_djvu_path);
    }
}

Task::~Task()
{
}

bool
Task::needPageReprocess(const PageId& page_id) const
{
    assert(m_ptrExportSuggestions->contains(page_id));

    std::unique_ptr<Params> params(m_ptrSettings->getPageParams(page_id));
    if (!params.get()) {
        return true;
    }

    Params::Regenerate val = params->getForceReprocess();
    if (val & Params::RegeneratePage) {
        return true;
    }

    const DjbzDict* dict = m_ptrDjbzDispatcher->dictionaryByPage(page_id);
    if (!dict) {
        return true;
    }

    SourceImagesInfo new_ImagesInfo(page_id, m_refOutFileNameGen, *m_ptrExportSuggestions);
    if (new_ImagesInfo != params->sourceImagesInfo()) {
        return true;
    }

    if (params->hasOutputParams()) {
        OutputParams const output_params_to_use(*params, dict->params());
        OutputParams const& output_params_was_used(params->outputParams());
        return !output_params_was_used.matches(output_params_to_use) ||
                !params->isDjVuCached();
    } else {
        return true;
    }

    return false;

}

bool
Task::needDjbzReprocess(const DjbzDict* dict) const
{
    // needPageReprocess(m_PageId) must be called first !

    assert(dict);
    if (dict->isDummy()) {
        return false;
    }

    for (const PageId& p: dict->pages()) {
        if (p != m_pageId) { // should be already checked in Task::process
            if (needPageReprocess(p)) {
                return true;
            }
        }
    }
    return false;
}

bool
Task::needReprocess(bool* djbz_is_cached) const
{
    const DjbzDict* dict = m_ptrDjbzDispatcher->dictionaryByPage(m_pageId);
    if (dict) {
        *djbz_is_cached = m_ptrDjbzDispatcher->isDictionaryCached(dict);
        if (*djbz_is_cached) {
            if (!needPageReprocess(m_pageId)) {
                // will check all pages except m_pageId
                return needDjbzReprocess(dict);
            }
        }
    }

    return true;
}

void
Task::validateParams(const PageId& page_id)
{
    std::unique_ptr<Params> params_ptr = m_ptrSettings->getPageParams(page_id);
    if (!params_ptr) {
        params_ptr.reset(new Params());
        m_ptrSettings->setPageParams(page_id, *params_ptr);
    }

    const DjbzDict* dict = m_ptrDjbzDispatcher->dictionaryByPage(page_id);
    if (!dict) {
        params_ptr->setDjbzId( m_ptrDjbzDispatcher->addNewPage(page_id)->id() );
        m_ptrSettings->setPageParams(page_id, *params_ptr);
    }

    const SourceImagesInfo new_ImagesInfo(page_id, m_refOutFileNameGen, *m_ptrExportSuggestions);
    const SourceImagesInfo info = params_ptr->sourceImagesInfo();
    if (!info.isValid() || info != new_ImagesInfo) {
        params_ptr->setSourceImagesInfo(
                    new_ImagesInfo
                    );
        m_ptrSettings->setPageParams(page_id, *params_ptr);
    }
}

void
Task::validateDjbzParams(const QString& dict_id)
{
    // validateParams(m_PageId) must be called first !

    const DjbzDict* dict = m_ptrDjbzDispatcher->dictionary(dict_id);
    for (const PageId& p: dict->pages()) {
        if (p != m_pageId) {
            validateParams(p);
        }
    }
}

void
Task::validateParams()
{
    validateParams(m_pageId);
    validateDjbzParams(m_ptrDjbzDispatcher->dictionaryIdByPage(m_pageId));
}

void
Task::startExport(TaskStatus const& status, const QSet<PageId> &pages_to_export)
{
    if (pages_to_export.isEmpty()) {
        return;
    }

    emit displayProgressInfo(0, EncodingProgressProcess::Export, EncodingProgressState::InProgress);

    ExportSettings sett;
    sett.mode = ExportMode(ExportMode::Foreground | ExportMode::Background);
    sett.page_gen_tweaks = PageGenTweak(PageGenTweak::NoTweaks);
    sett.save_blank_background = false;
    sett.save_blank_foreground = false;
    sett.export_selected_pages_only = true;
    sett.export_to_multipage = false;
    sett.use_sep_suffix_for_pics = true;

    QVector<ExportThread::ExportRec> recs;
    for (const PageId& p : pages_to_export) {
        std::unique_ptr<Params> params_ptr = m_ptrSettings->getPageParams(p);
        const SourceImagesInfo info = params_ptr->sourceImagesInfo();
        ExportThread::ExportRec rec;
        rec.page_id = p;
        rec.page_no = 0;
        rec.filename = info.source_filename();
        rec.override_background_filename = info.export_backgroundFilename();
        rec.override_foreground_filename = info.export_foregroundFilename();
        recs += rec;
    }

    const int total_pages = recs.size();
    int processed_pages_no = 0;
    ExportThread thread(sett, recs, m_export_path, *m_ptrExportSuggestions, this);
    connect(&thread, &ExportThread::imageProcessed, this, [&processed_pages_no, total_pages, this](){
        processed_pages_no++;
        const float progress = 100.*processed_pages_no/total_pages;
        emit displayProgressInfo(progress, EncodingProgressProcess::Export, EncodingProgressState::InProgress);

    });
    thread.start();
    while (!thread.wait(1000)) {
        if (status.isCancelled()) {
            thread.requestInterruption();
        }
        status.throwIfCancelled();
    }

    emit displayProgressInfo(100., EncodingProgressProcess::Export, EncodingProgressState::Completed);

    for (const PageId& p : pages_to_export) {
        std::unique_ptr<Params> params_ptr = m_ptrSettings->getPageParams(p);
        SourceImagesInfo info = params_ptr->sourceImagesInfo();
        info.update();
        params_ptr->setSourceImagesInfo(info);
        m_ptrSettings->setPageParams(p, *params_ptr);
    }
}

bool isSourceChanged(const std::unique_ptr<Params>& params)
{
    if (params) {
        if (params->hasOutputParams()) {
            return !params->outputParams().params().sourceImagesInfo().isSourceCached();
        }
    }
    return true;
}

void canUseCache(const std::unique_ptr<Params>& params, bool & use_jb2, bool& use_bg44)
{
    use_bg44 = use_jb2 = false;
    if (params->hasOutputParams()) {
        const Params& used_params = params->outputParams().params();
        use_jb2 = params->matchJb2Part(used_params);
        use_bg44 = params->matchBg44Part(used_params);
    }
}

bool requireReassembling(const std::unique_ptr<Params>& params)
{
    if (params->hasOutputParams()) {
        const Params& used_params = params->outputParams().params();
        return !params->matchAssemblePart(used_params);
    }
    return true;
}

bool requirePostprocessing(const std::unique_ptr<Params>& params, bool& force_reassambling)
{
    force_reassambling = false;
    bool require_postprocessing = true;
    if (params->hasOutputParams()) {
        const Params& used_params = params->outputParams().params();

        require_postprocessing = !params->matchPostprocessPart(used_params);

        if (require_postprocessing) {
            if (params->pageTitle().isEmpty() &&
                    !used_params.pageTitle().isEmpty()) {
                // the only way to clear title is to reassemble doc and not set it at all.
                force_reassambling = true;
            }
            if (!params->getOCRParams().applyOCR() &&
                    used_params.getOCRParams().applyOCR()) {
                // otherwise "djvused's remove-txt " left "(page 0 0 0 0 "")" in a chunk
                force_reassambling = true;
            }
        }
    }
    return require_postprocessing;
}

bool nonTrivialPostprocessing(const std::unique_ptr<Params>& params)
{
    if (params->getOCRParams().applyOCR()) {
        return true;
    }
    if (!params->pageTitle().isEmpty()) {
        return true;
    }
    if (params->pageRotation() != 0) {
        return true;
    }

    return false;
}


FilterResultPtr
Task::process(TaskStatus const& status, FilterData const& data)
{
    status.throwIfCancelled();

    m_ptrFilter->imageViewer()->hide();
    m_ptrFilter->djVuWidget()->setDocument(new QDjVuDocument(true)/*nullptr*/);

    m_ptrExportSuggestions = data.exportSuggestions();
    assert(m_ptrExportSuggestions);

    const PageSequence page_seq = m_ptrFilter->pages()->toPageSequence(PAGE_VIEW);
    const std::vector<PageId> all_pages_ordered = page_seq.asPageIdVector();
    const bool single_page_project = page_seq.numPages() == 1;

    m_ptrDjbzDispatcher->autosetToDictionaries(page_seq, m_ptrExportSuggestions, m_ptrSettings);


    bool djbz_is_cached;
    // check if this page or any of pages in shared dictionary that this page belongs is changed
    const bool need_reprocess = needReprocess(&djbz_is_cached);

    if (need_reprocess) {

        emit m_ptrSettings->bundledDocReady(false);
        emit setProgressPanelVisible(true);

        // check that all params are exist for every page in shared dictionary
        // and create default params if needed
        validateParams();

        // Ok, we are going to rebuild the shared dictionary

        const QSet<PageId> dictionary_pages = m_ptrDjbzDispatcher->pagesOfSameDictionary(m_pageId);

        // There are several processing steps that page may or may not
        // pass depending on its content.
        // 1. Export foreground and background layers
        // 2. Encode source images or just their background layers with c44-fi encoder to bg44 chunk
        // 3. Encode source image or just their foreground layers with minidjvu-mod encoder to
        //    indirect multipage document. Such document has one djbz.
        // 4. Export jb2 chunks from pages and reassemble them with corresponding bg44 chunks back into djvu
        // 4.5 OCR
        // 5. Apply postprocessing (djvused) settings to djvu pages.
        // 6. If doable - bundle whole pages in project into bundled multipage document (djvm).
        // 7. Apply postprocessing (djvused) settings to bundled document.

        QSet<PageId> to_export;
        QSet<PageId> to_c44;
        QSet<PageId> to_c44_cached; // pages that can reuse existing bg44 chunk
        QSet<PageId> to_minidjvu;
        QSet<PageId> to_minidjvu_cached; // pages that can reuse existing jb2 chunk
        QSet<PageId> to_assemble;
        QSet<PageId> to_ocr;
        QSet<PageId> to_postprocess;

        ProcessingIndicationPropagator::instance().emitPageProcessingStarted(dictionary_pages);

        for (const PageId& p: dictionary_pages) {
            const ExportSuggestion es = m_ptrExportSuggestions->value(p);
            assert (es.isValid);

            bool is_source_changed = true;
            bool may_reuse_jb2 = false;
            bool may_reuse_bg44 = false;
            bool reuse_jb2 = false;
            bool reuse_bg44 = false;

            const std::unique_ptr<Params> params = m_ptrSettings->getPageParams(p);
            Params::Regenerate val = params->getForceReprocess();
            if (val & Params::RegeneratePage) {
                val = (Params::Regenerate)(val & ~Params::RegeneratePage);
                params->setForceReprocess(val);
                m_ptrSettings->setPageParams(p, *params);
            } else {
                is_source_changed = isSourceChanged(params);
                if (!is_source_changed) {
                    canUseCache(params, may_reuse_jb2, may_reuse_bg44);
                }
            }

            const SourceImagesInfo& info = params->sourceImagesInfo();

            //            const bool djvu_cached = isDjVuCached(params);
            if (es.hasBWLayer) { // the page or its layer require jb2 encoding
                if ( may_reuse_jb2 && djbz_is_cached &&
                     info.isJB2cached() ) {
                    reuse_jb2 = true;
                    to_minidjvu_cached += p; // reuse existing result
                } else {
                    if (es.format != QImage::Format_Mono &&
                            es.format != QImage::Format_MonoLSB) {
                        // minidjvu-mod doesn't support images with a non-bw format even all pixels are b&w in them.
                        to_export += p;
                    }
                    to_minidjvu += p;
                    if (params->getOCRParams().applyOCR()) {
                        to_ocr += p;
                    }
                }
            }

            if (es.hasColorLayer) { // the page or its layer require bg44 encoding
                if ( may_reuse_bg44 && info.isBG44cached() ) {
                    reuse_bg44 = true;
                    to_c44_cached += p; // reuse existing result
                } else {
                    to_c44 += p;
                }
            }

            if (es.hasColorLayer && es.hasBWLayer) {
                if (!reuse_bg44 || !reuse_jb2) {
                    if (is_source_changed || !info.isExportCached()) {
                        // the page require splitting to layers again
                        to_export += p;
                    }
                }
            } else // empty page
                if (!es.hasBWLayer && !es.hasColorLayer) {
                    if ( !is_source_changed && params->isDjVuCached() ) {
                        to_minidjvu_cached += p; // reuse existing result
                    } else {
                        to_minidjvu += p; // empty page will generate virtual entry in encoder settings
                    }
                }

            if (requireReassembling(params) || !params->isDjVuCached()) {
                to_assemble += p;
            }

            if (params->getOCRParams().applyOCR() && es.hasBWLayer) {
                if (!params->getOCRParams().isCached()) {
                    to_ocr += p;
                }
            }

            bool force_reassembling;
            if (requirePostprocessing(params, force_reassembling)) {
                to_postprocess += p;
                if (force_reassembling) {
                    // some postprocessing can't be undone without reassambling (page titles)
                    to_assemble += p;
                }
            }

        }

        to_assemble += to_c44;
        to_assemble += to_minidjvu;
        to_postprocess += to_ocr;

        /************************************
         * Export pages to layers
         ************************************/

        if (!to_export.isEmpty()) {
            // some pages require splitting to layers
            startExport(status, to_export);
        } else {
            emit displayProgressInfo(100., EncodingProgressProcess::Export, EncodingProgressState::Skipped);
        }

        // we don't need to_export anymore

        /************************************
         * Encode bg44 chunks
         ************************************/


        if (!to_c44.isEmpty()) {

            emit displayProgressInfo(0, EncodingProgressProcess::EncodePic, EncodingProgressState::InProgress);

            const int total_pages = to_c44.size();
            int pages_processed = 0;
            const QVector<PageId> pages = to_c44.toList().toVector(); // omp don't work with qset
#pragma omp parallel shared(pages_processed, status)
#pragma omp for schedule(dynamic)
            for (const PageId& p: qAsConst(pages)) {

                if (status.isCancelled()) {  // don't throw inside omp
                    continue;
                }

                QStringList args;
                // params
                const std::unique_ptr<Params> params = m_ptrSettings->getPageParams(p);
                const int bsf = params->bsf();
                args << "-iff" << "-dpi" << QString::number(params->outputDpi().horizontal());
                if (bsf > 1) {
                    args << "-bsf" << QString::number(bsf)
                         << "-bsm" << scale_filter2str(params->scaleMethod());
                }

                // image to encode
                const SourceImagesInfo& info = params->sourceImagesInfo();
                const QString base_name = m_djvu_path + QFileInfo(info.source_filename()).completeBaseName();
                const bool layered = !info.export_backgroundFilename().isEmpty();
                const QString source = layered? info.export_backgroundFilename() :
                                                info.source_filename();

                args << source << base_name + ".bg44";

                qDebug() << args.join(' ');
                QAutoTerminatedProcess proc;
                proc.setProcessChannelMode(QProcess::ForwardedChannels);
                proc.start(GlobalStaticSettings::m_djvu_bin_c44, args);
                proc.waitForFinished(-1);

                pages_processed++;
                const float progress = 100.*pages_processed/total_pages;
                emit displayProgressInfo(progress, EncodingProgressProcess::EncodePic, EncodingProgressState::InProgress);
            }
            status.throwIfCancelled(); // don't throw inside omp
            emit displayProgressInfo(100., EncodingProgressProcess::EncodePic, EncodingProgressState::Completed);
        } else {
            emit displayProgressInfo(100., EncodingProgressProcess::EncodePic, EncodingProgressState::Skipped);
        }


        /************************************
         * Encode jb2 chunks
         ************************************/

        if (!to_minidjvu.isEmpty()) {

            emit displayProgressInfo(0, EncodingProgressProcess::EncodeTxt, EncodingProgressState::InProgress);

            QStringList encoder_params;

            if (!to_minidjvu_cached.isEmpty()) {
                // as jb2 pages are coded with shared dictionary
                // we can reuse cache if only all pages
                // in shared dictionary can do the same
                to_minidjvu += to_minidjvu_cached;
                // pages must be reassembled with new djbz
                to_assemble += to_minidjvu_cached;

                to_minidjvu_cached.clear();

            }

            for (const PageId& p: qAsConst(to_c44_cached)) {
                // fulfill pages with those which were cached
                // if they need to take part in reassembling djvu page
                if (to_minidjvu.contains(p)) {
                    to_c44 += p;
                }
            }


            QVector<PageId> to_minidjvu_ordered;
            for (const PageId& p: all_pages_ordered) {
                if (to_minidjvu.contains(p)) {
                    to_minidjvu_ordered.append(p);
                    if (to_minidjvu_ordered.size() == dictionary_pages.size()) break;
                }
            }


            m_ptrSettings->generateEncoderSettings(to_minidjvu_ordered, *m_ptrExportSuggestions, encoder_params);
            m_ptrDjbzDispatcher->generateDjbzEncodingParams(m_pageId, *m_ptrSettings, encoder_params);

            qDebug() << "" << encoder_params << "";

            QTemporaryFile encoder_settings;
            if (encoder_settings.open()) {
                encoder_settings.write(encoder_params.join('\n').toUtf8());
                encoder_settings.flush();

                {
                    QAutoTerminatedProcess proc;
                    QStringList args;
                    args << "-u" << "-r" << "-j" << "-S" << encoder_settings.fileName();

                    if (to_minidjvu.size() == 1) {
                        QFileInfo fi;
                        fi.setFile(m_refOutFileNameGen.fileNameFor(m_pageId));
                        args << m_djvu_path + fi.completeBaseName() + ".djvu";
                    } else {
                        const DjbzDict * dict = m_ptrDjbzDispatcher->dictionaryByPage(m_pageId);
                        args << m_djvu_path + "_djbz_" + dict->id() + ".djvu";
                    }

                    proc.setProcessChannelMode(QProcess::QProcess::ForwardedErrorChannel);
                    proc.start(GlobalStaticSettings::m_djvu_bin_minidjvu, args);
                    proc.waitForStarted();
                    while (proc.state() != QProcess::NotRunning) {
                        while (proc.waitForReadyRead(1000)) {
                            if (proc.canReadLine()) {
                                const QString val(proc.readAll());
#if QT_VERSION >= 0x050E00
                                const QStringList sl = val.split("\n", Qt::SkipEmptyParts);
#else
                                const QStringList sl = val.split("\n", QString::SkipEmptyParts);
#endif
                                float progress = -1;
                                for (const QString& s: sl) {
                                    printf(" found %s\n", s.toStdString().c_str());
                                    if (s.startsWith("[") && s.endsWith("%]")) {
                                        progress = s.midRef(1,s.length()-3).toFloat();
                                    }
                                }
                                if (progress != -1) {
                                    emit displayProgressInfo(progress, EncodingProgressProcess::EncodeTxt, EncodingProgressState::InProgress);
                                }
                            }

                            status.throwIfCancelled();
                        }

                        status.throwIfCancelled();
                    }

                    emit displayProgressInfo(100., EncodingProgressProcess::EncodeTxt, EncodingProgressState::Completed);
                }
            }
        } else {
            emit displayProgressInfo(100., EncodingProgressProcess::EncodeTxt, EncodingProgressState::Skipped);
        }

        /************************************
         * Assemble djvu pages
         ************************************/

        if (!to_assemble.isEmpty()) {

            emit displayProgressInfo(0, EncodingProgressProcess::Assemble, EncodingProgressState::InProgress);

            const int total_pages = to_assemble.size();
            int pages_processed = 0;
            const QVector<PageId> pages = to_assemble.toList().toVector(); // omp don't work with qset
#pragma omp parallel shared(pages_processed)
#pragma omp for schedule(dynamic)
            for (const PageId& p: qAsConst(pages)) {

                if (status.isCancelled()) {  // don't throw inside omp
                    continue;
                }

                QFileInfo fi;
                fi.setFile(m_refOutFileNameGen.fileNameFor(p));
                const QString base_name = m_djvu_path + fi.completeBaseName();

                QAutoTerminatedProcess proc;
                QStringList args;
                // overwrites target djvu if any
                args << base_name + ".djvu";
                const ExportSuggestion es = m_ptrExportSuggestions->value(p);
                args << QString("INFO=%1,%2,%3").arg(es.width).arg(es.height).arg(es.dpi);
                const std::unique_ptr<Params> params = m_ptrSettings->getPageParams(p);
                QString dict_id = params->djbzId();
                const DjbzDict* dict = m_ptrDjbzDispatcher->dictionary(dict_id);
                if (!dict->isDummy()) {
                    // must be before Sjbz chunk
                    dict_id += "." + dict->params().extension();
                    args << "INCL=" + dict_id;
                }

                if (params->fgbzOptions().isEmpty()) {
                    args << "FGbz=#black" + params->colorRectsAsTxt();
                } else {
                    args << "FGbz=" + params->fgbzOptions() + params->colorRectsAsTxt();
                }

                if (es.hasBWLayer) {
                    args << "Sjbz=" + base_name + ".jb2";
                }
                if (es.hasColorLayer) {
                    args << "BG44=" + base_name + ".bg44";
                }


                proc.setWorkingDirectory(m_djvu_path);
                qDebug() << args.join(" ");
                proc.start(GlobalStaticSettings::m_djvu_bin_djvumake, args);
                proc.waitForFinished(-1);


                pages_processed++;
                const float progress = 100.*pages_processed/total_pages;
                emit displayProgressInfo(progress, EncodingProgressProcess::Assemble, EncodingProgressState::InProgress);
            }

            status.throwIfCancelled(); // don't throw inside omp
            emit displayProgressInfo(100., EncodingProgressProcess::Assemble, EncodingProgressState::Completed);
        } else {
            emit displayProgressInfo(100., EncodingProgressProcess::Assemble, EncodingProgressState::Skipped);
        }

        /************************************
         * OCR b/w layers
         ************************************/
        if (!to_ocr.isEmpty()) {

            emit displayProgressInfo(0, EncodingProgressProcess::OCR, EncodingProgressState::InProgress);

            const int total_pages = to_ocr.size();
            int pages_processed = 0;
            const QVector<PageId> pages = to_ocr.toList().toVector(); // omp don't work with qset

            const int pages_count = pages.size();
#pragma omp parallel shared(pages_processed)
#pragma omp for schedule(dynamic)
            for (const PageId& p: qAsConst(pages)) {

                if (status.isCancelled()) {  // don't throw inside omp
                    continue;
                }

                const std::unique_ptr<Params> params = m_ptrSettings->getPageParams(p);
                const SourceImagesInfo si = params->sourceImagesInfo();

                QStringList args;
                const QString image_fname = si.export_foregroundFilename().isEmpty() ?
                            si.source_filename() :
                            si.export_foregroundFilename();
                args << image_fname;

                QFileInfo fi;
                fi.setFile(m_refOutFileNameGen.fileNameFor(p));
                QString hocr_fname = m_djvu_path + fi.completeBaseName();
                args << hocr_fname;
                hocr_fname += ".hocr";
                const QString ocr_fname = m_djvu_path + fi.completeBaseName() + ".ocr";

                args << "-l" << GlobalStaticSettings::m_ocr_langs;
                if (!GlobalStaticSettings::m_ocr_additional_args.isEmpty()) {
                    args << GlobalStaticSettings::m_ocr_additional_args;
                }

                args << "hocr"; // instruct tesseract to use hOCR format

                QAutoTerminatedProcess proc;
                proc.setProcessChannelMode(QProcess::ForwardedChannels);

                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                if (pages_count > 1) {
                    env.insert("OMP_THREAD_LIMIT", "1"); // https://github.com/tesseract-ocr/tesseract/issues/3109
                }
                proc.setProcessEnvironment(env);
                proc.start(GlobalStaticSettings::m_ocr_path_to_exe, args);
                proc.waitForFinished(-1);

                HOCR2DjVuSed::convertFile(hocr_fname, ocr_fname);

                if (!GlobalStaticSettings::m_ocr_keep_hocr) {
                    QFile::remove(hocr_fname);
                }

                pages_processed++;
                const float progress = 100.*pages_processed/total_pages;
                emit displayProgressInfo(progress, EncodingProgressProcess::OCR, EncodingProgressState::InProgress);
            }

            status.throwIfCancelled(); // don't throw inside omp
            emit displayProgressInfo(100., EncodingProgressProcess::OCR, EncodingProgressState::Completed);
        } else {
            emit displayProgressInfo(100., EncodingProgressProcess::OCR, EncodingProgressState::Skipped);
        }

        /************************************
         * Postprocess djvu pages
         ************************************/

        // reassembling resets features set at postprocessing step
        QSet<PageId> assembled_but_not_postprocessed =
                to_assemble - to_postprocess;
        for (const PageId& p: assembled_but_not_postprocessed) {
            const std::unique_ptr<Params> params = m_ptrSettings->getPageParams(p);
            if (nonTrivialPostprocessing(params)) {
                to_postprocess += p;
            }
        }

        // check if we need to set page title or metadata
        if (!to_postprocess.isEmpty()) {
            //const int total_pages = to_postprocess.size();
//            int pages_processed = 0;
            const QVector<PageId> pages = to_postprocess.toList().toVector(); // omp don't work with qset
//#pragma omp parallel shared(pages_processed)
#pragma omp for schedule(dynamic)
            for (const PageId& p: qAsConst(pages)) {
                const std::unique_ptr<Params> params = m_ptrSettings->getPageParams(p);
                QString djvused_cmd;
                if (!params->pageTitle().isEmpty()) {
                    djvused_cmd += QString("select 1; set-page-title \"%1\"; ")
                            .arg(params->pageTitle());
                }

                djvused_cmd += QString("select 1; set-rotation \"%1\"; ")
                        .arg(params->pageRotation());

                QFileInfo fi;
                fi.setFile(m_refOutFileNameGen.fileNameFor(p));
                const QString djvu_fname = m_djvu_path + fi.completeBaseName() + ".djvu";

                if (params->getOCRParams().applyOCR()) {
                    const QString ocr_fname = m_djvu_path + fi.completeBaseName() + ".ocr";
                    djvused_cmd += QString(" select 1; set-txt \"%1\"; ").arg(ocr_fname);
                }

                QStringList args;
                if (!single_page_project) {
                    // can't use -s as it saves file as a bundled doc
                    djvused_cmd += QString("save-page \"%1\"; ").arg(djvu_fname);
                    args << djvu_fname << "-e" << djvused_cmd;
                } else {
                    args << djvu_fname << "-e" << djvused_cmd << "-s";
                }

                QAutoTerminatedProcess proc;
                proc.setProcessChannelMode(QProcess::ForwardedChannels);
                proc.start(GlobalStaticSettings::m_djvu_bin_djvused, args);
                proc.waitForFinished(-1);

//                pages_processed++;
            }
        }

        /************************************
         * Update params
         ************************************/
        {
            const std::unique_ptr<Params> params = m_ptrSettings->getPageParams(m_pageId);

            const QString dict_id = params->djbzId();
            DjbzDict* dict = m_ptrDjbzDispatcher->dictionary(dict_id);
            if (!dict->isDummy()) {
                const QString djbz_filename = m_djvu_path + "/" + dict_id + "." + dict->params().extension();
                dict->setOutputFilename(djbz_filename);
                dict->updateOutputFileInfo();
            }
            QDateTime revision = dict->revision();


            QFileInfo fi;
            for (const PageId& p: dictionary_pages) {
                const std::unique_ptr<Params> params = m_ptrSettings->getPageParams(p);
                params->setDjbzRevision(revision);
                fi.setFile(m_refOutFileNameGen.fileNameFor(p));
                const QString djvu_fname = m_djvu_path + fi.completeBaseName() + ".djvu";
                const QString ocr_fname = m_djvu_path + fi.completeBaseName() + ".ocr";
                params->setDjvuFilename(djvu_fname);
                fi.setFile(djvu_fname);
                assert(fi.exists());
                params->setDjVuSize(fi.size());
                params->setDjVuLastChanged(fi.lastModified());

                if (params->getOCRParams().applyOCR()) {
                    params->getOCRParamsRef().setOCRFile(ocr_fname);
                }

                SourceImagesInfo new_ImagesInfo = params->sourceImagesInfo();
                new_ImagesInfo.update();
                params->setSourceImagesInfo(new_ImagesInfo);

                params->rememberOutputParams(dict->params());

                m_ptrSettings->setPageParams(p, *params);
                m_ptrThumbnailCache->recreateThumbnail(ImageId(djvu_fname));
            }
        }

        ProcessingIndicationPropagator::instance().emitPageProcessingFinished(dictionary_pages);
    } //need_reprocess

    emit setProgressPanelVisible(false);

    if (need_reprocess || m_ptrSettings->bundledDocNeedsUpdate()) {
        if (m_ptrSettings->checkPagesReady(all_pages_ordered)) {
            emit generateBundledDocument();
        }
    }


    if (CommandLine::get().isGui()) {
        const ExportSuggestion es = m_ptrExportSuggestions->value(m_pageId);

        return FilterResultPtr(
                    new UiUpdater(
                        m_ptrFilter, m_pageId,
                        m_ptrDjbzDispatcher,
                        es.hasBWLayer,
                        es.hasColorLayer,
                        m_batchProcessing
                        )
                    );
    } else {
        return FilterResultPtr(nullptr);
    }

}


/*============================ Task::UiUpdater ========================*/

Task::UiUpdater::UiUpdater(IntrusivePtr<Filter> const& filter,
                           PageId const& page_id,
                           const IntrusivePtr<DjbzDispatcher> &djbzDispatcher,
                           bool show_foreground_settings,
                           bool show_background_settings,
                           bool batch_processing)
    : m_ptrFilter(filter),
      m_pageId(page_id),
      m_ptrDjbzDispatcher(djbzDispatcher),
      m_show_foreground_settings(show_foreground_settings),
      m_show_background_settings(show_background_settings),
      m_batchProcessing(batch_processing)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
    // This function is executed from the GUI thread.
    OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
    if (!m_batchProcessing) {
        opt_widget->postUpdateUI(m_show_foreground_settings, m_show_background_settings);
    }
    ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

    m_ptrFilter->suppressDjVuDisplay(m_pageId, m_batchProcessing);
    const QSet<PageId> pages = m_ptrDjbzDispatcher->pagesOfSameDictionary(m_pageId);
    for (const PageId& p: pages) {
        ui->invalidateThumbnail(p);
    }


    if (m_batchProcessing) {
        return;
    }

    ui->setImageWidget(m_ptrFilter->imageViewer(), ui->KEEP_OWNERSHIP);
    m_ptrFilter->updateDjVuDocument(m_pageId);
}

} // namespace publish
