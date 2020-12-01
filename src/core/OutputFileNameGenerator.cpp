/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "OutputFileNameGenerator.h"
#include "PageId.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include <QFileInfo>
#include <QDir>
#include <assert.h>

OutputFileNameGenerator::OutputFileNameGenerator()
    :   m_ptrDisambiguator(new FileNameDisambiguator),
        m_outDir(),
        m_layoutDirection(Qt::LeftToRight)
{
}

OutputFileNameGenerator::OutputFileNameGenerator(
    IntrusivePtr<FileNameDisambiguator> const& disambiguator,
    QString const& out_dir, Qt::LayoutDirection layout_direction)
    :   m_ptrDisambiguator(disambiguator),
        m_outDir(out_dir),
        m_layoutDirection(layout_direction)
{
    assert(m_ptrDisambiguator.get());
}

void
OutputFileNameGenerator::performRelinking(AbstractRelinker const& relinker)
{
    m_ptrDisambiguator->performRelinking(relinker);
    m_outDir = relinker.substitutionPathFor(RelinkablePath(m_outDir, RelinkablePath::Dir));
}

QString
OutputFileNameGenerator::fileNameFor(PageId const& page) const
{
    bool const ltr = (m_layoutDirection == Qt::LeftToRight);
    PageId::SubPage const sub_page = page.subPage();
    QFileInfo fi(page.imageId().filePath());
    QString name(fi.completeBaseName());
    const bool is_empty_page = fi.path().startsWith(QStringLiteral(":"));

    int label; QString prefix;
    bool found = m_ptrDisambiguator->getLabelAndOverridenFilename(page.imageId().filePath(), page.imageId().page(), label, prefix);
    if (found) {
        if (!prefix.isEmpty()) {
            name = prefix.contains("%1") ? prefix.arg(name) : prefix;
        }

        if (label != 0) {
            name += QString::fromLatin1("(%1)").arg(label);
        }
    }

    if (page.imageId().isMultiPageFile() && !is_empty_page) {
        name += QString::fromLatin1("_page%1").arg(
                    page.imageId().page(), 4, 10, QLatin1Char('0')
                );
    }
    if (sub_page != PageId::SINGLE_PAGE) {
        name += QLatin1Char('_');
        name += QLatin1Char(ltr == (sub_page == PageId::LEFT_PAGE) ? '1' : '2');
        name += QLatin1Char(sub_page == PageId::LEFT_PAGE ? 'L' : 'R');
    }
    name += QString::fromLatin1(".tif");

    return name;
}

QString
OutputFileNameGenerator::filePathFor(PageId const& page) const
{
    QString const file_name(fileNameFor(page));
    return QDir(m_outDir).absoluteFilePath(file_name);
}

void get_empty_page_suffix(QString const& name, QString& base_name, int& order_num)
{
    int idx = name.lastIndexOf(QLatin1String("&ep"));
    if (idx != -1) {
        base_name = name.left(idx);
        order_num = name.rightRef(name.size() - idx - 3).toInt();
    } else {
        base_name = name;
    }
}

QString intToOrderNum(int order)
{
    QString order_num = QString::number(order);
    while (order_num.size() < 4) {
        order_num.prepend(QStringLiteral("0"));
    }
    return order_num;
}

const QString alph("&0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

QString
OutputFileNameGenerator::suggestOverridenFileName(QStringList const& insert_to_filenames, bool after) const
{
    assert(insert_to_filenames.size() <= 2 && !insert_to_filenames.isEmpty());

    if (insert_to_filenames.size() == 2) {
        // we are inserting an empty page somewhere between 2 output files
        int val1 = 0; int val2 = 0;
        QString base_name1; QString base_name2;
        get_empty_page_suffix(insert_to_filenames.first(), base_name1, val1);
        get_empty_page_suffix(insert_to_filenames.back(), base_name2, val2);
        if (base_name1 == base_name2) {
            return base_name1 + "&ep" + intToOrderNum((val2 + val1) / 2);
        } else {
            return base_name1 + "&ep" + intToOrderNum(val1 + 1000);
        }
    } else if (after) {
        // empty page filename should be the last one in alphabetical order among all output filenames
        int val1 = 0;
        QString base_name1;
        get_empty_page_suffix(insert_to_filenames.first(), base_name1, val1);
        return base_name1 + "&ep" + intToOrderNum(val1 + 1000);
    } else {
        // empty page filename should be the first one in alphabetical order among all output filenames
        // we shall find a new filename for it
        const QString& old_name = insert_to_filenames.first();
        const QString alph_start = alph[0];
        const QChar& alph_end = alph[alph.size() - 1];
        QString new_name = old_name;
        while (!new_name.isEmpty() && new_name != alph_start) {
            QChar char_to_test = new_name[new_name.size() - 1];
            if (char_to_test > alph_end) {
                char_to_test = alph_end;
            }

            int i = alph.indexOf(char_to_test);
            if (i <= 0) {
                new_name.chop(1);
                continue;
            }
            char_to_test = alph[i - 1];
            new_name = new_name.left(new_name.size() - 1) + char_to_test;
            if (new_name < old_name) {
                return new_name + "&ep" + intToOrderNum(1000);
            }
        }
    }
    return QString();
}
