/*

    Handle Geogrid-Viewer binary overlay file format (.ovl)

    Copyright (C) 2016-2020 Ralf Horstmann <ralf@ackstorm.de>
    Copyright (C) 2016-2020 Robert Lipe, robertlipe+source@gpsbabel.org

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
#ifndef GGV_BIN_H_INCLUDED_
#define GGV_BIN_H_INCLUDED_

#include <QString>
#include <QIODevice>

#include "format.h"
#include "geodata.h"

class GgvBinFormat : public Format
{
public:
  GgvBinFormat() {};
  bool probe(QIODevice* io) override;
  void read(QIODevice* io, Geodata* geodata) override;
  const QString getName() override;
};

#endif
