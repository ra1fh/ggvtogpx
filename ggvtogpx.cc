/*
    ggvtogpx main module

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

#include <QDebug>

#include "defs.h"
#include "ggv_bin.h"

static QList<Waypoint*> waypoints;
static QList<Route*> routes;

static int debug_level;

int get_debug_level()
{
  return debug_level;
};

void waypt_add(Waypoint* waypoint)
{
  if (get_debug_level() > 1) {
    qDebug("waypt_add()");
  }
  waypoints.append(waypoint);
};

void track_add_head(Route* route)
{
  if (get_debug_level() > 1) {
    qDebug("track_add_head()");
  }
  routes.append(route);
};
void track_add_wpt(Route* route, Waypoint* waypoint)
{
  if (get_debug_level() > 1) {
    qDebug("track_add_wpt()");
  }
  route->waypoint_list.append(waypoint);
};

int main(int argc, char* argv[])
{
  GgvBinFormat* format = new GgvBinFormat();

  if (argc <= 1) {
    qCritical() << "usage: ggvtogpx <filename>";
    exit(1);
  }

  QString fname = argv[1];

  debug_level = 2;
  format->rd_init(fname);
  format->read();
  format->rd_deinit();
}
