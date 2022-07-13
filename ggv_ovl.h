/*

    Support for "GeoGrid Viewer ascii overlay files".

    Copyright (C) 2008 Olaf Klein (o.b.klein@gpsbabel.org).
    Copyright (C) 2022 Ralf Horstmann <ralf@ackstorm.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */
#ifndef GGV_OVL_H_INCLUDED_
#define GGV_OVL_H_INCLUDED_

#include <QSettings>

#include "defs.h"

class GgvOvlFormat : public Format
{
public:
  void rd_init(const QString& fname) override;
  void read() override;
  void rd_deinit() override;

private:
  QString read_fname;
  QSettings* inifile;
};

#endif
