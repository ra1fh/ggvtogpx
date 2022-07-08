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

#include <QCoreApplication>
#include <QCommandLineParser>
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

int process_files(const QString& infile, const QString& outfile, QString& creator)
{
  GgvBinFormat* format = new GgvBinFormat();

  format->rd_init(infile);
  format->read();
  format->rd_deinit();

  QFile ofile(outfile);
  if (!ofile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qCritical() << "usage: ggvtogpx <filename>";
    return 1;
  }

  QXmlStreamWriter xml;
  xml.setAutoFormatting(true);
  xml.setAutoFormattingIndent(2);
  xml.setDevice(&ofile);
  xml.writeStartDocument();
  xml.writeStartElement(QStringLiteral("gpx"));
  xml.writeAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
  xml.writeAttribute(QStringLiteral("creator"), creator);
  xml.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.topografix.com/GPX/1/0"));
  xml.writeTextElement(QStringLiteral("time"), QStringLiteral("1970-01-01T00:00:00Z"));

  double minlat, minlon, maxlat, maxlon;
  minlat = 360;
  minlon = 360;
  maxlat = 0;
  maxlon = 0;
  for (auto&& route : std::as_const(routes)) {
    for (auto&& waypoint : std::as_const(route->waypoint_list)) {
      if (waypoint->latitude > maxlat) {
        maxlat = waypoint->latitude;
      }
      if (waypoint->latitude < minlat) {
        minlat = waypoint->latitude;
      }
      if (waypoint->longitude > maxlon) {
        maxlon = waypoint->longitude;
      }
      if (waypoint->longitude < minlon) {
        minlon = waypoint->longitude;
      }
    }
  }
  for (auto&& waypoint : std::as_const(waypoints)) {
    if (waypoint->latitude > maxlat) {
      maxlat = waypoint->latitude;
    }
    if (waypoint->latitude < minlat) {
      minlat = waypoint->latitude;
    }
    if (waypoint->longitude > maxlon) {
      maxlon = waypoint->longitude;
    }
    if (waypoint->longitude < minlon) {
      minlon = waypoint->longitude;
    }
  }

  if (! routes.isEmpty() || ! waypoints.isEmpty()) {
    xml.writeStartElement(QStringLiteral("bounds"));
    xml.writeAttribute(QStringLiteral("minlat"), QString::number(minlat, 'f', 9));
    xml.writeAttribute(QStringLiteral("minlon"), QString::number(minlon, 'f', 9));
    xml.writeAttribute(QStringLiteral("maxlat"), QString::number(maxlat, 'f', 9));
    xml.writeAttribute(QStringLiteral("maxlon"), QString::number(maxlon, 'f', 9));
    xml.writeEndElement();
  }

  for (auto&& waypoint : std::as_const(waypoints)) {
    xml.writeStartElement(QStringLiteral("wpt"));
    xml.writeAttribute(QStringLiteral("lat"), QString::number(waypoint->latitude, 'f', 9));
    xml.writeAttribute(QStringLiteral("lon"), QString::number(waypoint->longitude, 'f', 9));
    if (! waypoint->description.isEmpty()) {
      xml.writeTextElement(QStringLiteral("name"), waypoint->description);
      xml.writeTextElement(QStringLiteral("cmt"), waypoint->description);
      xml.writeTextElement(QStringLiteral("desc"), waypoint->description);
    }
    xml.writeEndElement();
  }

  for (auto&& route : std::as_const(routes)) {
    xml.writeStartElement(QStringLiteral("trk"));
    if (! route->route_name.isEmpty()) {
      xml.writeTextElement(QStringLiteral("name"), route->route_name);
    }
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
  return 0;
}

int main(int argc, char* argv[])
{
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("ggvtogpx");
  QCoreApplication::setApplicationVersion("1.0");

  QCommandLineParser parser;
  parser.setApplicationDescription("GeoGrid Viewer to GPX Converter");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption creatorStringOption("C", "creator <creator>", "creator");
  parser.addOption(creatorStringOption);

  QCommandLineOption debugLevelOption("D", "debug <level>", "debug");
  parser.addOption(debugLevelOption);

  QCommandLineOption inputTypeOption("i", "input type (ignored)", "type");
  parser.addOption(inputTypeOption);

  QCommandLineOption inputFileOption("f", "input <file>", "file");
  parser.addOption(inputFileOption);

  QCommandLineOption outputTypeOption("o", "output type (ignored)", "type");
  parser.addOption(outputTypeOption);

  QCommandLineOption outputFileOption("F", "output <file>", "file");
  parser.addOption(outputFileOption);

  parser.addPositionalArgument("infile", "input file (alternative to -f)");
  parser.addPositionalArgument("outfile","output file (alternative to -F)");

  parser.process(app);

  debug_level = 0;
  if (parser.isSet(debugLevelOption)) {
    bool ok;
    int num = parser.value(debugLevelOption).toInt(&ok);
    if (ok && num >= 0 && num <= 9) {
      debug_level = num;
    } else {
      qCritical() << qPrintable(app.applicationName()) << ": invalid debug level";
    }
  }

  QString infile("");
  QString outfile("");
  if (parser.positionalArguments().size() > 2) {
    qCritical() << qPrintable(app.applicationName()) << ": too many positional arguments";
    exit(1);
  } else if (parser.positionalArguments().size() == 2) {
    infile = parser.positionalArguments().at(0);
    outfile = parser.positionalArguments().at(1);
  } else if (parser.positionalArguments().size() == 1) {
    infile = parser.positionalArguments().at(0);
  }

  if (parser.isSet(inputFileOption)) {
    infile = parser.value(inputFileOption);
  }

  if (parser.isSet(outputFileOption)) {
    outfile = parser.value(outputFileOption);
  }

  QString creator = "ggvtogpx";
  if (parser.isSet(creatorStringOption)) {
    creator = parser.value(creatorStringOption);
  }

  exit(process_files(infile, outfile, creator));
}
