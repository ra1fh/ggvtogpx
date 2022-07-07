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
#include <QFile>
#include <QXmlStreamWriter>

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

  if (argc <= 2) {
    qCritical() << "usage: ggvtogpx <infile> <outfile>";
    exit(1);
  }

  QString fname = argv[1];

  debug_level = 2;
  format->rd_init(fname);
  format->read();
  format->rd_deinit();

  QString oname = argv[2];
  QFile ofile(oname);
  if (!ofile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qCritical() << "usage: ggvtogpx <filename>";
    exit(1);
  }

  QXmlStreamWriter xml;
  xml.setAutoFormatting(true);
  xml.setAutoFormattingIndent(2);
  xml.setDevice(&ofile);
  xml.writeStartDocument();
  xml.writeStartElement(QStringLiteral("gpx"));
  xml.writeAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
  xml.writeAttribute(QStringLiteral("creator"), QStringLiteral("ggvtogpx"));
  xml.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.topografix.com/GPX/1/0"));
  xml.writeTextElement(QStringLiteral("time"), QStringLiteral("1970-01-01T00:00:00"));

  xml.writeStartElement(QStringLiteral("bounds"));
  xml.writeAttribute(QStringLiteral("minlat"), QStringLiteral("1.23"));
  xml.writeAttribute(QStringLiteral("minlon"), QStringLiteral("1.23"));
  xml.writeAttribute(QStringLiteral("maxlat"), QStringLiteral("1.23"));
  xml.writeAttribute(QStringLiteral("maxlon"), QStringLiteral("1.23"));

  for (auto&& route : std::as_const(routes)) {
    xml.writeStartElement(QStringLiteral("trk"));
    xml.writeStartElement(QStringLiteral("trkseg"));
    for (auto&& waypoint : std::as_const(route->waypoint_list)) {
      xml.writeStartElement(QStringLiteral("trkpt"));
      xml.writeAttribute(QStringLiteral("lat"), QString::number(waypoint->latitude, 'f', 9));
      xml.writeAttribute(QStringLiteral("lon"), QString::number(waypoint->longitude, 'f', 9));
      xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndElement();
  }

  xml.writeEndElement();
  xml.writeEndDocument();
}
