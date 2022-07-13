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

#include <QString>
#include <QDebug>

#include "defs.h"
#include "ggv_ovl.h"

#define MYNAME "ggv_ovl"

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

static int track_ct, route_ct, route_points;

void
GgvOvlFormat::rd_init(const QString& fname)
{
  inifile = new QSettings(fname, QSettings::IniFormat);


  if (get_debug_level() > 2) {
    qDebug() << MYNAME << "rd_init() fname:" << fname;
  }
  route_ct = 0;
  track_ct = 0;
  route_points = 0;
}

void
GgvOvlFormat::rd_deinit()
{
  delete inifile;
  inifile = 0;
}

void
GgvOvlFormat::read()
{
  int symbols = inifile->value("Overlay/Symbols", 0).toInt();
  if (get_debug_level() > 1) {
    qDebug() << MYNAME << "read() symbols:" << symbols;
  }

  for (int i = 1; i <= symbols; ++i) {

    QString symbol = QString("Symbol %1").arg(i);

    OVL_SYMBOL_TYP type = (OVL_SYMBOL_TYP) inifile->value(symbol + "/Typ", 0).toInt();
    int points = inifile->value(symbol + "/Punkte", -1).toInt();

    if (get_debug_level() > 1) {
      qDebug() << MYNAME << "read() points:" << points;
    }
    QString lat;
    QString lon;
    switch (type) {

      Waypoint* wpt;
      int group;

    case OVL_SYMBOL_LINE:
    case OVL_SYMBOL_POLYGON:

      group = inifile->value(symbol + "/Group", -1).toInt();

      if (points > 0) {
        Route* trk;

        auto* rte = trk = new Route;
        if (group > 1) {
          route_add_head(rte);
          route_ct++;
          rte->route_name = QString("Route %1").arg(route_ct);
        } else {
          track_add_head(trk);
          track_ct++;
          trk->route_name = QString("Track %1").arg(track_ct);
        }

        for (int j = 0; j < points; ++j) {

          wpt = new Waypoint;

          lat = inifile->value(symbol + "/YKoord" + QString::number(j), "").toString();
          if (lat.isNull()) {
            delete wpt;
            continue;
          }
          wpt->latitude = lat.toDouble();

          lon = inifile->value(symbol + "/XKoord" + QString::number(j), "").toString();
          if (lon.isNull()) {
            delete wpt;
            continue;
          }
          wpt->longitude = lon.toDouble();

          if (group > 1) {
            route_points++;
            wpt->description = QString("RPT") + QString::number(route_points).rightJustified(3, '0');
            route_add_wpt(rte, wpt);
          } else {
            track_add_wpt(trk, wpt);
          }
        }
      }
      break;

    case OVL_SYMBOL_CIRCLE:
    case OVL_SYMBOL_TRIANGLE:

      wpt = new Waypoint;
      wpt->description = symbol;

      lat = inifile->value(symbol + "/YKoord", "").toString();
      if (lat.isNull()) {
        delete wpt;
        continue;
      }
      wpt->latitude = lat.toDouble();
      lon = inifile->value(symbol + "/XKoord", "").toString();
      if (lon.isNull()) {
        delete wpt;
        continue;
      }
      wpt->longitude = lon.toDouble();

      waypt_add(wpt);
      break;

    case OVL_SYMBOL_BITMAP:
    case OVL_SYMBOL_TEXT:
    case OVL_SYMBOL_RECTANGLE:
      break;
    }
  }
}
