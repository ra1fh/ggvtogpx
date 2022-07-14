/*

    Support for GPX writing

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

#include <QDateTime>
#include <QXmlStreamWriter>

#include "gpx.h"

void
GpxFormat::write(QIODevice* io, const Geodata* geodata)
{
  QXmlStreamWriter xml;
  xml.setAutoFormatting(true);
  xml.setAutoFormattingIndent(2);
  xml.setDevice(io);
  xml.writeStartDocument();
  xml.writeStartElement(QStringLiteral("gpx"));
  xml.writeAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
  xml.writeAttribute(QStringLiteral("creator"), creator);
  xml.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.topografix.com/GPX/1/0"));

  QString time;
  if (testmode) {
    time = QDateTime::fromSecsSinceEpoch(0, Qt::UTC).toString(Qt::ISODate);
  } else {
    time = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  }
  xml.writeTextElement(QStringLiteral("time"), time);

  auto bounds = geodata->getBounds();
  Waypoint min = bounds.first;
  Waypoint max = bounds.second;

  const auto kNumDigits = 9;

  if (! geodata->getRoutes().empty() || ! geodata->getTracks().empty() || ! geodata->getWaypoints().empty()) {
    xml.writeStartElement(QStringLiteral("bounds"));
    xml.writeAttribute(QStringLiteral("minlat"), QString::number(min.latitude, 'f', kNumDigits));
    xml.writeAttribute(QStringLiteral("minlon"), QString::number(min.longitude, 'f', kNumDigits));
    xml.writeAttribute(QStringLiteral("maxlat"), QString::number(max.latitude, 'f', kNumDigits));
    xml.writeAttribute(QStringLiteral("maxlon"), QString::number(max.longitude, 'f', kNumDigits));
    xml.writeEndElement();
  }

  for (auto&& waypoint : std::as_const(geodata->getWaypoints())) {
    xml.writeStartElement(QStringLiteral("wpt"));
    xml.writeAttribute(QStringLiteral("lat"), QString::number(waypoint->latitude, 'f', kNumDigits));
    xml.writeAttribute(QStringLiteral("lon"), QString::number(waypoint->longitude, 'f', kNumDigits));
    if (! waypoint->name.isEmpty()) {
      xml.writeTextElement(QStringLiteral("name"), waypoint->name);
      xml.writeTextElement(QStringLiteral("cmt"), waypoint->name);
      xml.writeTextElement(QStringLiteral("desc"), waypoint->name);
    }
    xml.writeEndElement();
  }

  for (auto&& route : std::as_const(geodata->getRoutes())) {
    xml.writeStartElement(QStringLiteral("rte"));
    if (! route->name.isEmpty()) {
      xml.writeTextElement(QStringLiteral("name"), route->name);
    }
    for (auto&& waypoint : std::as_const(route->getWaypoints())) {
      xml.writeStartElement(QStringLiteral("rtept"));
      xml.writeAttribute(QStringLiteral("lat"), QString::number(waypoint->latitude, 'f', kNumDigits));
      xml.writeAttribute(QStringLiteral("lon"), QString::number(waypoint->longitude, 'f', kNumDigits));
      if (! waypoint->name.isEmpty()) {
        xml.writeTextElement(QStringLiteral("name"), waypoint->name);
      }
      xml.writeEndElement();
    }
    xml.writeEndElement();
  }

  for (auto&& track : std::as_const(geodata->getTracks())) {
    xml.writeStartElement(QStringLiteral("trk"));
    if (! track->name.isEmpty()) {
      xml.writeTextElement(QStringLiteral("name"), track->name);
    }
    xml.writeStartElement(QStringLiteral("trkseg"));
    for (auto&& waypoint : std::as_const(track->getWaypoints())) {
      xml.writeStartElement(QStringLiteral("trkpt"));
      xml.writeAttribute(QStringLiteral("lat"), QString::number(waypoint->latitude, 'f', kNumDigits));
      xml.writeAttribute(QStringLiteral("lon"), QString::number(waypoint->longitude, 'f', kNumDigits));
      xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndElement();
  }

  xml.writeEndElement();
  xml.writeEndDocument();
}

void GpxFormat::setCreator(const QString& _creator)
{
  creator = _creator;
}

void GpxFormat::setTestmode(bool _testmode)
{
  testmode = _testmode;
}

const QString GpxFormat::getName()
{
  return "gpx";
}
