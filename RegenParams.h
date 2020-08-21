/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef REGEN_PARAMS_H_
#define REGEN_PARAMS_H_

class RegenParams
{
public:

    enum Regenerate {
        RegenerateNone = 0,
        RegeneratePage = 1,
        RegenerateThumbnail = 2,
        RegenerateAll = 3
    };

    RegenParams(Regenerate val = RegenerateNone): m_forceReprocess(val) {}

    virtual void setForceReprocess(Regenerate val)
    {
        m_forceReprocess = val;
    }

    virtual Regenerate getForceReprocess() const
    {
        return m_forceReprocess;
    }

private:
    Regenerate m_forceReprocess;
};

#endif
