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
#include <QXmlStreamReader>

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


static QString
ggv_xml_parse_base(QXmlStreamReader& xml)
{
  QString name;
  if (ggv_xml_debug_level() > 1) {
    qDebug() << "xml: parse_base";
  };

  while (xml.readNextStartElement()) {
    if (ggv_xml_debug_level() > 1) {
      qDebug() << "xml: parse_base start:" << xml.name();
    }

    if (xml.name() == QLatin1String("name")) {
      name = xml.readElementText();
    }

    return name;

    if (ggv_xml_debug_level() > 1) {
      qDebug() << "xml: parse_base name:" << name;
    }

    xml.skipCurrentElement();
  }
  return name;
}

static void
ggv_xml_parse_attributelist(QXmlStreamReader& xml, Geodata* geodata, const QString& name)
{
  if (ggv_xml_debug_level() > 1) {
    qDebug() << "xml: parse_attributelist";
  }

  while (xml.readNextStartElement()) {
    if (ggv_xml_debug_level() > 1) {
      qDebug() << "xml: parse_attributelist start:" << xml.name();
    }

    if (xml.name() == QLatin1String("attribute")) {
      QStringView iid = xml.attributes().value(QLatin1String("iidName"));
      if (ggv_xml_debug_level() > 1) {
        qDebug() << "xml: parse_attributelist iid:  " << iid;
      }
      if (iid == QLatin1String("IID_IGraphic")) {
        auto waypoint_list = std::make_unique<WaypointList>();
        waypoint_list->name = name;
        while (xml.readNextStartElement()) {
          if (ggv_xml_debug_level() > 1) {
            qDebug() << "xml: parse_attributelist start:" << xml.name();
          }
          if (xml.name() == QLatin1String("coordList")) {
            while (xml.readNextStartElement()) {
              auto waypoint = std::make_unique<Waypoint>();
              waypoint->latitude = xml.attributes().value(QLatin1String("y")).toDouble();
              waypoint->longitude = xml.attributes().value(QLatin1String("x")).toDouble();
              waypoint->elevation = xml.attributes().value(QLatin1String("z")).toDouble();
              if (ggv_xml_debug_level() > 2) {
                qDebug() << "xml: parse_attributelist"
                         << "lat:" << waypoint->latitude
                         << "lon:" << waypoint->longitude
                         << "ele:" << waypoint->elevation;
              }
              waypoint_list->addWaypoint(waypoint);

              xml.skipCurrentElement();
            }
          }
          xml.skipCurrentElement();
        }
        geodata->addTrack(waypoint_list);
      }
    }
    xml.skipCurrentElement();
  }
}

static void
ggv_xml_parse_object(QXmlStreamReader& xml, Geodata* geodata)
{
  QString name;
  if (ggv_xml_debug_level() > 1) {
    qDebug() << "xml: parse_object";
  }

  while (xml.readNextStartElement()) {
    if (ggv_xml_debug_level() > 1) {
      qDebug() << "xml: parse_object start:" << xml.name();
    }

    if (xml.name() == QLatin1String("base")) {
      name = ggv_xml_parse_base(xml);
    } else if (xml.name() == QLatin1String("attributeList")) {
      ggv_xml_parse_attributelist(xml, geodata, name);
    }

    xml.skipCurrentElement();
  }
}

static void
ggv_xml_parse_objectlist(QXmlStreamReader& xml, Geodata* geodata)
{
  if (ggv_xml_debug_level() > 1) {
    qDebug() << "xml: parse_objectlist";
  };

  while (xml.readNextStartElement()) {
    if (ggv_xml_debug_level() > 1) {
      qDebug() << "xml: parse_objectlist start:" << xml.name();
    }

    if (xml.name() == QLatin1String("object")) {
      QStringView clsname = xml.attributes().value(QLatin1String("clsName"));
      if (ggv_xml_debug_level() > 1) {
        qDebug() << "xml: parse_objectlist clsname:  " << clsname;
      }
      if (clsname == QLatin1String("CLSID_GraphicLine")) {
        ggv_xml_parse_object(xml, geodata);
      }
    }
    xml.skipCurrentElement();
  }
}

static void
ggv_xml_parse_geogrid(QXmlStreamReader& xml, Geodata* geodata)
{
  if (ggv_xml_debug_level() > 1) {
    qDebug() << "xml: parse_geogrid";
  };

  while (xml.readNextStartElement()) {
    if (ggv_xml_debug_level() > 1) {
      qDebug() << "xml: parse_geogrid start:" << xml.name();
    }
    if (xml.name() == QLatin1String("objectList")) {
      ggv_xml_parse_objectlist(xml, geodata);
    }
    xml.skipCurrentElement();
  }
}

static void
ggv_xml_parse_document(QXmlStreamReader& xml, Geodata* geodata)
{
  if (ggv_xml_debug_level() > 1) {
    qDebug() << "xml: parse_document";
  };

  while (xml.readNextStartElement()) {
    if (ggv_xml_debug_level() > 1) {
      qDebug() << "xml: parse_document start:" << xml.name();
    }
    if (xml.name() == QLatin1String("geogridOvl")) {
      ggv_xml_parse_geogrid(xml, geodata);
    }
    xml.skipCurrentElement();
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

  QXmlStreamReader xml(filebuf);
  ggv_xml_parse_document(xml, geodata);

  zip_fclose(zip_file);
  zip_close(zip);
  zip_source_free(source);
}

const QString GgvXmlFormat::getName()
{
  return "ggv_xml";
};
