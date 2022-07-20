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

#include <QByteArray>
#include <QDebug>
#include <QString>
#include <QLatin1String>
#include <QDomDocument>

#include <zip.h>

#include "ggv_xml.h"


/***************************************************************************
 *           local helper functions                                        *
 ***************************************************************************/


static int
ggv_xml_debug_level(const int* update = nullptr)
{
  static int debug_level = 0;
  if (update) {
    debug_level = *update;
  }
  return debug_level;
}

static void
ggv_xml_parse_document(QDomDocument& xml, Geodata* geodata)
{
  QDomNode root = xml.documentElement();
  QDomNode objectList = root.firstChildElement("objectList");

  for (QDomNode n = objectList.firstChildElement("object"); !n.isNull(); n = n.nextSibling()) {
    if (! n.isElement()) {
      continue;
    }

    QDomElement e = n.toElement();
    QString uid = e.attribute("uid");
    QString clsname = e.attribute("clsName");
    QString clsid = e.attribute("clsid");
    if (ggv_xml_debug_level() > 1) {
      qDebug().noquote() << "element name:" << e.tagName();
      qDebug().noquote() << "    uid:" << uid;
      qDebug().noquote() << "    clsName:" << clsname;
      qDebug().noquote() << "    clsid:" << clsid;
    }

    if (clsname != "CLSID_GraphicLine") {
      continue;
    }

    auto waypoint_list = std::make_unique<WaypointList>();
    QDomNode base = n.firstChildElement("base");
    if (!base.isNull()) {
      QDomElement name = base.firstChildElement("name").toElement();
      if (!name.isNull()) {
        waypoint_list->name = name.text();
        if (ggv_xml_debug_level() > 1) {
          qDebug().noquote() << "        track_name:" << waypoint_list->name;
        }
      }
    }

    QDomNode attributelist = n.firstChildElement("attributeList");
    for (QDomNode attribute = attributelist.firstChildElement("attribute"); !attribute.isNull(); attribute = attribute.nextSibling()) {
      QDomElement e = attribute.toElement();
      QString iidname = e.attribute("iidName");
      if (ggv_xml_debug_level() > 1) {
        qDebug().noquote() << "        iidName:" << iidname;
      }
      if (iidname != "IID_IGraphic") {
        continue;
      }
      QDomNode coordlist = attribute.firstChildElement("coordList");
      if (coordlist.isNull()) {
        continue;
      }
      for (QDomNode coord = coordlist.firstChildElement("coord"); !coord.isNull(); coord = coord.nextSibling()) {
        QDomElement e = coord.toElement();
        if (!e.hasAttribute("x") || !e.hasAttribute("y")) {
          continue;
        }
        auto waypoint = std::make_unique<Waypoint>();
        waypoint->latitude = e.attribute("y").toDouble();
        waypoint->longitude = e.attribute("x").toDouble();
        if (e.hasAttribute("z") && e.attribute("z") != "-32768") {
          waypoint->elevation = e.attribute("z").toDouble();
        }
        if (ggv_xml_debug_level() > 2) {
          qDebug().noquote() << "            coord:"
                             << waypoint->latitude
                             << waypoint->longitude
                             << waypoint->elevation;
        }
        waypoint_list->addWaypoint(waypoint);
      }

      if (ggv_xml_debug_level() > 1) {
        qDebug().noquote() << "            coord count:" << waypoint_list->getWaypoints().size();
      }

      if (waypoint_list->getWaypoints().size()) {
        geodata->addTrack(waypoint_list);
      }
    }
  }
}

/***************************************************************************
 *              entry points called by ggvtogpx main process               *
 ***************************************************************************/

bool
GgvXmlFormat::probe(QIODevice* io)
{
  int debug_level = getDebugLevel();
  ggv_xml_debug_level(&debug_level);

  QByteArray buf;
  QByteArray magic = "PK\x03\x04";
  io->reset();
  buf = io->peek(magic.size());
  if (buf.startsWith(magic)) {
    return true;
  }
  return false;
}

void
GgvXmlFormat::read(QIODevice* io, Geodata* geodata)
{
  int debug_level = getDebugLevel();
  ggv_xml_debug_level(&debug_level);

  QByteArray buf;
  io->reset();
  buf = io->readAll();

  zip_error_t error;
  zip_error_init(&error);
  zip_source_t* source = zip_source_buffer_create(buf.data(), buf.size(), 0, &error);
  if (source == nullptr) {
    qCritical() << "xml: create source error";
    zip_error_fini(&error);
    exit(1);
  }

  zip_t* zip = zip_open_from_source(source, 0, &error);
  if (zip == nullptr) {
    qCritical() << "xml: create zip error";
    zip_source_free(source);
    zip_error_fini(&error);
    exit(1);
  }
  zip_error_fini(&error);
  zip_source_keep(source);

  zip_int64_t index = zip_name_locate(zip, "geogrid50.xml", ZIP_FL_NODIR);
  if (ggv_xml_debug_level() > 1) {
    qDebug() << "xml: found index:" << index;
  }

  zip_stat_t stat;
  if (zip_stat_index(zip, index, 0, &stat) != 0) {
    qCritical().noquote()
        << QStringLiteral("xml: zip stat failed");
    exit(1);
  }
  if (ggv_xml_debug_level() > 1) {
    qDebug() << "xml: zip stat size:" << stat.size;
  }

  zip_file_t* zip_file = zip_fopen_index(zip, index, 0);
  if (zip_file == nullptr) {
    qCritical().noquote()
        << QStringLiteral("xml: error opening file ");
    exit(1);
  }

  // Use a rather convervative limit here although the API supports more
  if (stat.size > INT32_MAX) {
    qCritical().noquote()
        << QStringLiteral("xml: file size exceeds limit (%1 > %2)").arg(stat.size).arg(INT32_MAX);
    exit(1);
  }

  QByteArray filebuf;
  filebuf.resize(static_cast<qsizetype>(stat.size));

  zip_int64_t len = zip_fread(zip_file, filebuf.data(), filebuf.size());
  if (len <= 0) {
    qCritical().noquote()
        << QStringLiteral("xml: error reading archive file (%1)").arg(len);
    exit(1);
  }

  QDomDocument xml("geogrid50");
  xml.setContent(filebuf);
  ggv_xml_parse_document(xml, geodata);

  zip_fclose(zip_file);
  zip_close(zip);
  zip_source_free(source);
}

const QString GgvXmlFormat::getName()
{
  return "ggv_xml";
};
