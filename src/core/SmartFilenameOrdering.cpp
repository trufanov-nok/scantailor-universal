/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "SmartFilenameOrdering.h"
#include <QFileInfo>
#include <QString>
#include <QRegularExpression>

bool
SmartFilenameOrdering::operator()(QFileInfo const& lhs, QFileInfo const& rhs) const
{
    // First compare directories.
    if (int comp = lhs.absolutePath().compare(rhs.absolutePath())) {
        return comp < 0;
    }

    QRegularExpression re_num("[0-9]+");
    const QString zero('0');

    const QString left_filename(lhs.fileName());
    const QString right_filename(rhs.fileName());

    QRegularExpressionMatchIterator match_left_it = re_num.globalMatch(left_filename);
    QRegularExpressionMatchIterator match_right_it = re_num.globalMatch(right_filename);

    int pos1 = 0;
    int pos2 = 0;
    QString fn1;
    QString fn2;

    while (match_left_it.hasNext() && match_right_it.hasNext()) {
        QRegularExpressionMatch match_left = match_left_it.next();
        QRegularExpressionMatch match_right = match_right_it.next();

        fn1 += left_filename.midRef(pos1, match_left.capturedStart() - pos1);
        pos1 =  match_left.capturedEnd();

        fn2 += right_filename.midRef(pos2, match_right.capturedStart() - pos2);
        pos2 =  match_right.capturedEnd();

        QString left_num = match_left.captured();
        QString right_num = match_right.captured();
        int diff = left_num.size() - right_num.size();

        if (diff < 0) {
            left_num.prepend(zero.repeated(-1 * diff));
        } else if (diff > 0) {
            right_num.prepend(zero.repeated(diff));
        }

        fn1 += left_num;
        fn2 += right_num;
    }
    if (pos1 < left_filename.size() - 1) {
        fn1 += left_filename.rightRef(left_filename.size() - pos1);
    }
    if (pos2 < right_filename.size() - 1) {
        fn2 += right_filename.rightRef(right_filename.size() - pos2);
    }

    if (int comp = fn1.compare(fn2)) {
        return comp < 0;
    }

    return left_filename < right_filename;
}
