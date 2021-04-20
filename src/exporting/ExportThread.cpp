/*
    Scan Tailor Universal - Interactive post-processing tool for scanned
    pages. A fork of Scan Tailor by Joseph Artsimovich.
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

#include "ExportThread.h"
#include "ImageLoader.h"
#include "ImageSplitOps.h"
#include "TiffWriter.h"
#include "settings/globalstaticsettings.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#include <QDir>
#include <QMetaType>



namespace exporting {

const int dummy = qRegisterMetaType<PageId>("PageId");

ExportThread::ExportThread(const ExportSettings& settings, const QVector<ExportRec>& outpaths,
                           const QString& export_dir, QObject *parent): QThread(parent),
    m_settings(settings),
    m_outpaths_vector(outpaths),
    m_export_dir(export_dir),
    m_interrupted(false)
{
}

bool
ExportThread::isCancelRequested()
{
    if (!m_interrupted && isInterruptionRequested()) {
        m_interrupted = true;
        emit exportCanceled();
    }
    return m_interrupted;
}

void
ExportThread::run()
{
    QDir dir;
    dir.mkdir(m_export_dir);

    const QString text_dir = m_export_dir + QDir::separator() + "txt";  //folder for foreground subscans
    const QString pic_dir  = m_export_dir + QDir::separator() + "pic";  //folder for background subscans
    const QString mask_dir = m_export_dir + QDir::separator() + "mask"; //folder for zones info
    const QString zone_dir = m_export_dir + QDir::separator() + "zone"; //folder for zones info

    if (m_settings.mode != exporting::ExportMode::None) {
        if (m_settings.mode.testFlag(exporting::ExportMode::Foreground) && !m_settings.export_to_multipage) {
            dir.mkdir(text_dir);
        }
        if (m_settings.mode.testFlag(ExportMode::Background) && !m_settings.export_to_multipage) {
            dir.mkdir(pic_dir);
        }
        if ( (m_settings.mode.testFlag(ExportMode::Mask) || m_settings.mode.testFlag(ExportMode::AutoMask))
                && !m_settings.export_to_multipage) {
            dir.mkdir(mask_dir);
        }
        if (m_settings.mode.testFlag(ExportMode::Zones)) {
            dir.mkdir(zone_dir);
        }
    }

    bool need_reprocess = m_settings.mode.testFlag(ExportMode::Foreground) &&
            m_settings.page_gen_tweaks.testFlag(PageGenTweak::KeepOriginalColorIllumForeSubscans);
    const bool keep_orig = need_reprocess;
    if (!need_reprocess) {
        need_reprocess = m_settings.mode.testFlag(ExportMode::WholeImage) &&
                m_settings.page_gen_tweaks.testFlag(PageGenTweak::IgnoreOutputProcessingStage);
    }

#ifdef _OPENMP
    const int default_thread_num = omp_get_max_threads();
    if (need_reprocess) {
        // in case omp is used and images are going to be reprocessed
        // the gui becomes so irresponsible that this process is
        // impossible to cancel. So decrease max threads used by 1 and
        // return value back when export is completed.
        omp_set_num_threads(std::min(1, default_thread_num-1));
    }
#endif

#pragma omp parallel
#pragma omp for schedule(dynamic)
    for (int i = 0; i < m_outpaths_vector.count(); i++) {
        const ExportRec& rec = m_outpaths_vector[i];
        
        if (!isCancelRequested()) {
            if (need_reprocess) {
                m_paused.lock();
                // ask main thread to reprocess the image
                emit needReprocess(rec.page_id, &m_orig_fore_subscan);
                m_wait.wait(&m_paused);
                m_paused.unlock();
            }
        }

        if (!isCancelRequested()) { // don't want to mess with 'omp cancel for'

            const QString out_file_path = rec.filename;
            QString st_num = QString::number(rec.page_no);
            const QString name = QString().fill('0', std::max(0, 4 - st_num.length())) + st_num;

            if (!QFile().exists(out_file_path)) {
                emit error(tr("The file") + " \"" + out_file_path + "\" " + tr("is not found") + ".");
                exit(-1);
            }

            QImage out_img = ImageLoader::load(out_file_path);

            QString out_file_path_no_split = m_export_dir + QDir::separator() + name + ".tif";

            if (m_settings.mode.testFlag(ExportMode::Zones)) {
                const QStringList& zones_info = rec.zones_info;
                QString out_zone_file = m_export_dir + QDir::separator() + "zone" + QDir::separator() + name + ".tsv";
                if (!zones_info.isEmpty()) {
                    QFile f(out_zone_file);
                    if (f.open(QIODevice::WriteOnly)) {
                        f.write(zones_info.join("\n").toStdString().c_str());
                        f.close();
                    }
                } else if (QFile::exists(out_zone_file)) {
                    QFile::remove(out_zone_file);
                }
            }

            std::unique_ptr<QImage> img_foreground(m_settings.mode.testFlag(ExportMode::Foreground) ? new QImage() : nullptr);
            std::unique_ptr<QImage> img_background(m_settings.mode.testFlag(ExportMode::Background) ? new QImage() : nullptr);
            std::unique_ptr<QImage> img_mask(m_settings.mode.testFlag(ExportMode::Mask) ? new QImage() : nullptr);

            bool only_bw = true;

            if (out_img.format() == QImage::Format_Indexed8) {
                only_bw = ImageSplitOps::GenerateSubscans<uint8_t>(out_img, img_foreground.get(), img_background.get(), img_mask.get(), keep_orig, keep_orig ? &m_orig_fore_subscan : nullptr);
            } else if (out_img.format() == QImage::Format_RGB32 || out_img.format() == QImage::Format_ARGB32) {
                only_bw = ImageSplitOps::GenerateSubscans<uint32_t>(out_img, img_foreground.get(), img_background.get(), img_mask.get(), keep_orig, keep_orig ? &m_orig_fore_subscan : nullptr);
            } else if (out_img.format() == QImage::Format_Mono) {
                if (img_foreground) {
                    *img_foreground = out_img;
                }
                if (img_background && m_settings.generate_blank_back_subscans) {
                    *img_background = ImageSplitOps::GenerateBlankImage(out_img, out_img.format());
                } else {
                    img_background.reset(nullptr);
                }
                if (img_mask) {
                    *img_mask = ImageSplitOps::GenerateBlankImage(out_img, out_img.format(), 0x00000000);
                }

            }

            int page_no = 0;

            if (m_settings.mode.testFlag(ExportMode::WholeImage)) {
                TiffWriter::writeImage(out_file_path_no_split,
                                       m_settings.page_gen_tweaks.testFlag(PageGenTweak::IgnoreOutputProcessingStage) ? m_orig_fore_subscan : out_img,
                                       m_settings.export_to_multipage, page_no);
                if (m_settings.export_to_multipage) {
                    page_no++;
                }
            }

            if (img_foreground) {
                QString out_filepath_foreground = text_dir + QDir::separator() + name + ".tif";
                TiffWriter::writeImage(m_settings.export_to_multipage ? out_file_path_no_split : out_filepath_foreground,
                                       *img_foreground,
                                       m_settings.export_to_multipage,
                                       page_no++
                                       );
            }
            if (img_background && (!only_bw || m_settings.generate_blank_back_subscans)) {
                QString out_filepath_background = m_settings.use_sep_suffix_for_pics ? ".sep.tif" : ".tif";
                out_filepath_background = pic_dir + QDir::separator() + name + out_filepath_background;
                TiffWriter::writeImage(m_settings.export_to_multipage ? out_file_path_no_split : out_filepath_background,
                                       *img_background,
                                       m_settings.export_to_multipage,
                                       page_no++);
            }

            if (m_settings.mode.testFlag(ExportMode::AutoMask)) {
                QFileInfo fi(rec.filename);
                QString filepath_automask = fi.path() + "/cache/automask/" + fi.fileName();
                QImage automask_img = (QFile::exists(filepath_automask)) ? ImageLoader::load(filepath_automask) :
                                                                           ImageSplitOps::GenerateBlankImage(out_img, out_img.format(), 0x00000000);
                QString out_filepath_mask = mask_dir + QDir::separator() + name + ".auto.tif";
                TiffWriter::writeImage(m_settings.export_to_multipage ? out_file_path_no_split : out_filepath_mask,
                                       automask_img,
                                       m_settings.export_to_multipage,
                                       page_no);
                if (m_settings.export_to_multipage) {
                    page_no++;
                }
            }

            if (img_mask) {
                QString out_filepath_mask = mask_dir + QDir::separator() + name + ".tif";
                TiffWriter::writeImage(m_settings.export_to_multipage ? out_file_path_no_split : out_filepath_mask,
                                       *img_mask,
                                       m_settings.export_to_multipage,
                                       page_no++);
            }

            emit imageProcessed();
        }
    }

#ifdef _OPENMP
    if (need_reprocess) {
        omp_set_num_threads(default_thread_num);
    }
#endif

    if (!isCancelRequested()) {
        emit exportCompleted();
    }
}

}
