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

#include <QByteArray>
#include <QDebug>
#include <QSettings>
#include <QString>
#include <QTemporaryFile>

#include "ggv_ovl.h"

enum OVL_SYMBOL_TYP {
  OVL_SYMBOL_BITMAP = 1,
  OVL_SYMBOL_TEXT,
  OVL_SYMBOL_LINE,
  OVL_SYMBOL_POLYGON,
  OVL_SYMBOL_RECTANGLE,
  OVL_SYMBOL_CIRCLE,
  OVL_SYMBOL_TRIANGLE
};

/* some hints:
		# "col":   color
		# "group": 1 means NO GROUP
		# "size":  size in pixels PLUS 100
		# "with":
		# "zoom":
		# "art":   line-style
 */

bool
GgvOvlFormat::probe(QIODevice* io)
{
  QByteArray buf;

  std::list<QByteArray> magic;
  magic.push_back(QByteArray("[Symbol"));
  magic.push_back(QByteArray("[Overlay]"));

  io->reset();
  for (auto&& m : std::as_const(magic)) {
    buf = io->peek(m.size());
    if (buf.startsWith(m)) {
      return true;
    }
  }
  return false;
}

void
GgvOvlFormat::read(QIODevice* io, Geodata* geodata)
{
  io->reset();
  // QSettings does not handle QIODevice. Therefore we write
  // the data to a tempfile here to be able to keep the nice
  // generic interface with QIODevice
  QTemporaryFile tempfile;
  tempfile.open();
  tempfile.write(io->readAll());
  tempfile.close();

  QSettings inifile(tempfile.fileName(), QSettings::IniFormat);

  int route_count = 0;
  int track_count = 0;
  int waypoint_count = 0;
  int symbols = inifile.value("Overlay/Symbols", 0).toInt();
  if (getDebugLevel() > 1) {
    qDebug() << "ggv_ovl::read() symbols:" << symbols;
  }

  QString latitude;
  QString longitude;
  int group = 0;
  for (int i = 1; i <= symbols; ++i) {

    QString symbol = QString("Symbol %1").arg(i);
    OVL_SYMBOL_TYP type = static_cast<OVL_SYMBOL_TYP>(inifile.value(symbol + "/Typ", 0).toInt());
    int points = inifile.value(symbol + "/Punkte", -1).toInt();

    if (getDebugLevel() > 1) {
      qDebug() << "ggv_ovl::read() points:" << points;
    }

    switch (type) {
    case OVL_SYMBOL_LINE:
    case OVL_SYMBOL_POLYGON: {
      group = inifile.value(symbol + "/Group", -1).toInt();
      if (points > 0) {
        auto waypoint_list = std::make_unique<WaypointList>();
        for (int j = 0; j < points; ++j) {
          latitude = inifile.value(symbol + "/YKoord" + QString::number(j), "").toString();
          if (latitude.isEmpty()) {
            continue;
          }
          longitude = inifile.value(symbol + "/XKoord" + QString::number(j), "").toString();
          if (longitude.isEmpty()) {
            continue;
          }
          auto waypoint = std::make_unique<Waypoint>();
          waypoint->latitude = latitude.toDouble();
          waypoint->longitude = longitude.toDouble();
          if (group > 1) {
            waypoint_count++;
            waypoint->name = QString("RPT") + QString::number(waypoint_count).rightJustified(3, '0');
          }
          waypoint_list->addWaypoint(waypoint);
        }
        waypoint_list->name = inifile.value(symbol + "/Text", "").toString();
        if (waypoint_list->name.isEmpty()) {
          if (group > 1) {
            waypoint_list->name = QString("Route %1").arg(++route_count);
          } else {
            waypoint_list->name = QString("Track %1").arg(++track_count);
          }
        }
        if (group > 1) {
          geodata->addRoute(waypoint_list);
        } else {
          geodata->addTrack(waypoint_list);
        }
      }
    }
    break;

    case OVL_SYMBOL_TEXT:
    case OVL_SYMBOL_RECTANGLE:
    case OVL_SYMBOL_CIRCLE:
    case OVL_SYMBOL_TRIANGLE: {
      latitude = inifile.value(symbol + "/YKoord", "").toString();
      if (latitude.isEmpty()) {
        continue;
      }
      longitude = inifile.value(symbol + "/XKoord", "").toString();
      if (longitude.isEmpty()) {
        continue;
      }
      auto waypoint = std::make_unique<Waypoint>();
      waypoint->latitude = latitude.toDouble();
      waypoint->longitude = longitude.toDouble();
      waypoint->name = inifile.value(symbol + "/Text", "").toString();
      if (waypoint->name.isEmpty()) {
        waypoint->name = symbol;
      }
      geodata->addWaypoint(waypoint);
    }
    break;

    case OVL_SYMBOL_BITMAP:
      break;
    }
  }
}

const QString GgvOvlFormat::getName()
{
  return "ggv_ovl";
};
