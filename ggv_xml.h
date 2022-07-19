/*

    Support for "GeoGrid Viewer XML overlay files".

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
#ifndef GGV_XML_H_INCLUDED_
#define GGV_XML_H_INCLUDED_

#include <QIODevice>
#include <QString>

#include "format.h"
#include "geodata.h"

class GgvXmlFormat : public Format
{
public:
  GgvXmlFormat() {};
  bool probe(QIODevice* io) override;
  void read(QIODevice* io, Geodata* geodata) override;
  const QString getName() override;
};

#endif
