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

static std::unique_ptr<WaypointList>
ggv_xml_parse_attributelist(QDomNode& attributelist)
{
  auto waypoint_list = std::make_unique<WaypointList>();
  for (QDomNode attribute = attributelist.firstChildElement("attribute"); !attribute.isNull(); attribute = attribute.nextSibling()) {
    QDomElement e = attribute.toElement();
    QString iidname = e.attribute("iidName");
    if (ggv_xml_debug_level() > 1) {
      qDebug().noquote() << "        iidName:" << iidname;
    }
    if (iidname == "IID_IGraphicTextAttributes") {
      QDomElement text = attribute.firstChildElement("text");
      if (text.isNull()) {
        continue;
      }
      if (! text.text().isEmpty()) {
        waypoint_list->name = text.text();
        if (ggv_xml_debug_level() > 1) {
          qDebug().noquote() << "            text:" << text.text();
        }
      }
    } else if (iidname == "IID_IGraphic") {
      QDomNode coordlist = attribute.firstChildElement("coordList");
      if (coordlist.isNull()) {
        continue;
      }
      for (QDomNode coord = coordlist.firstChildElement("coord"); !coord.isNull(); coord = coord.nextSibling()) {
        QDomElement coordElement = coord.toElement();
        if (!coordElement.hasAttribute("x") || !coordElement.hasAttribute("y")) {
          continue;
        }
        auto waypoint = std::make_unique<Waypoint>();
        waypoint->latitude = coordElement.attribute("y").toDouble();
        waypoint->longitude = coordElement.attribute("x").toDouble();
        if (coordElement.hasAttribute("z") && coordElement.attribute("z") != "-32768") {
          waypoint->elevation = coordElement.attribute("z").toDouble();
        }
        if (ggv_xml_debug_level() > 2) {
          qDebug().noquote() << "            coord:"
                             << waypoint->latitude
                             << waypoint->longitude
                             << waypoint->elevation;
        }
        waypoint_list->addWaypoint(waypoint);
      }
    }


  }
  if (ggv_xml_debug_level() > 1) {
    qDebug().noquote() << "            coord count:"
                       << waypoint_list->getWaypoints().size();
  }
  return waypoint_list;
}

static void
ggv_xml_parse_document(QDomDocument& xml, Geodata* geodata)
{
  QDomNode root = xml.documentElement();
  QDomNode objectList = root.firstChildElement("objectList");
  int waypoint_count = 0;
  uint32_t track_count = 0;
  uint32_t text_count = 0;

  for (QDomNode object = objectList.firstChildElement("object"); !object.isNull(); object = object.nextSibling()) {
    if (! object.isElement()) {
      continue;
    }

    QDomElement objectElement = object.toElement();
    QString uid = objectElement.attribute("uid");
    QString clsname = objectElement.attribute("clsName");
    QString clsid = objectElement.attribute("clsid");
    if (ggv_xml_debug_level() > 1) {
      qDebug().noquote() << "element name:" << objectElement.tagName();
      qDebug().noquote() << "    uid:" << uid;
      qDebug().noquote() << "    clsName:" << clsname;
      qDebug().noquote() << "    clsid:" << clsid;
    }

    if (clsname != "CLSID_GraphicLine" && clsname != "CLSID_GraphicCircle" && clsname != "CLSID_GraphicText") {
      continue;
    }

    QString name;
    QDomNode base = object.firstChildElement("base");
    if (!base.isNull()) {
      if (ggv_xml_debug_level() > 1) {
        qDebug().noquote() << "        base";
      }
      QDomElement name_element = base.firstChildElement("name").toElement();
      if (!name_element.isNull()) {
        if (ggv_xml_debug_level() > 1) {
          qDebug().noquote() << "            name";
        }
        name = name_element.text();
        if (ggv_xml_debug_level() > 1) {
          qDebug().noquote() << "                text:"
                             << name;
        }
      }
    }

    QDomNode attributelist = object.firstChildElement("attributeList");
    auto waypoint_list = ggv_xml_parse_attributelist(attributelist);
    if (waypoint_list && waypoint_list->getWaypoints().size()) {
      if (clsname == "CLSID_GraphicLine") {
        if (name.isEmpty() || name == "Teilstrecke" || name == "Line") {
          waypoint_list->name = QString("Track ") + QString::number(++track_count).rightJustified(3, '0');
        } else {
          waypoint_list->name = name;
        }
        geodata->addTrack(waypoint_list);
      } else if (clsname == "CLSID_GraphicCircle") {
        auto waypoint = waypoint_list->extractFirstWaypoint();
        if (waypoint) {
          if (name.isEmpty() || name == "Circle") {
            waypoint->name = QString("RPT") + QString::number(++waypoint_count).rightJustified(3, '0');
          } else {
            waypoint->name = name;
          }
          geodata->addWaypoint(waypoint);
        }
      } else if (clsname == "CLSID_GraphicText") {
        auto waypoint = waypoint_list->extractFirstWaypoint();
        if (waypoint) {
          if (waypoint_list->name.isEmpty() || waypoint_list->name == "Text") {
            waypoint->name = QString("Text %1").arg(++text_count);
          } else {
            waypoint->name = waypoint_list->name;
          }
          geodata->addWaypoint(waypoint);
        }
      }
    }
  }
}

static int
ggv_xml_read_zip(QByteArray& buf, Geodata* geodata)
{
  // using a shared pointer to register fini function that frees memory
  // within the non-dynamic zip_error_t
  zip_error_t error_storage;
  std::shared_ptr<zip_error_t> error(&error_storage, [](zip_error_t* error) {
    zip_error_fini(error);
  });
  zip_error_init(error.get());

  std::shared_ptr<zip_source_t> source(zip_source_buffer_create(buf.data(), buf.size(), 0, error.get()), [](zip_source_t* source) {
    zip_source_free(source);
  });
  if (!source) {
    qCritical() << "xml: create source error";
    return 1;
  }

  std::shared_ptr<zip_t> zip(zip_open_from_source(source.get(), 0, error.get()), [](zip_t *zip) {
    zip_close(zip);
  });

  if (! zip) {
    qCritical() << "xml: create zip error";
    zip_source_free(source.get());
    return 1;
  }
  zip_source_keep(source.get());

  zip_int64_t index = zip_name_locate(zip.get(), "geogrid50.xml", ZIP_FL_NODIR);
  if (ggv_xml_debug_level() > 1) {
    qDebug() << "xml: found index:" << index;
  }

  zip_stat_t stat;
  if (zip_stat_index(zip.get(), index, 0, &stat) != 0) {
    qCritical().noquote()
        << QStringLiteral("xml: zip stat failed");
    return 1;
  }
  if (ggv_xml_debug_level() > 1) {
    qDebug() << "xml: zip stat size:" << stat.size;
  }

  std::shared_ptr<zip_file_t> zip_file(zip_fopen_index(zip.get(), index, 0), [](zip_file_t* zip_file) {
    zip_fclose(zip_file);
  });
  if (! zip_file) {
    qCritical().noquote()
        << QStringLiteral("xml: error opening file ");
    return 1;
  }

  // Use a rather convervative limit here although the API supports more
  if (stat.size > INT32_MAX) {
    qCritical().noquote()
        << QStringLiteral("xml: file size exceeds limit (%1 > %2)").arg(stat.size).arg(INT32_MAX);
    return 1;
  }

  QByteArray filebuf;
  filebuf.resize(static_cast<qsizetype>(stat.size));

  zip_int64_t len = zip_fread(zip_file.get(), filebuf.data(), filebuf.size());
  if (len <= 0) {
    qCritical().noquote()
        << QStringLiteral("xml: error reading archive file (%1)").arg(len);
    return 1;
  }

  QDomDocument xml("geogrid50");
  xml.setContent(filebuf);
  ggv_xml_parse_document(xml, geodata);
  return 0;
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

  if (ggv_xml_read_zip(buf, geodata)) {
    exit(1);
  }
}

const QString GgvXmlFormat::getName()
{
  return "ggv_xml";
};
