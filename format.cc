/*

    Format class

    Copyright (C) 2016-2020 Ralf Horstmann <ralf@ackstorm.de>

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

#include "format.h"

bool
Format::probe([[maybe_unused]] QIODevice* io)
{
  return false;
};

void
Format::read([[maybe_unused]] QIODevice* io, [[maybe_unused]] Geodata* geodata)
{
}

void
Format::write([[maybe_unused]] QIODevice* io, [[maybe_unused]] const Geodata* geodata)
{
}

const QString
Format::getName()
{
  return "";
};

void
Format::setDebugLevel(int _debuglevel)
{
  debuglevel = _debuglevel;
};

int
Format::getDebugLevel()
{
  return debuglevel;
};
