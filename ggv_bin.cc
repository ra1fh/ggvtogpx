/*

    Handle Geogrid-Viewer binary overlay file format (.ovl)

    Copyright (C) 2016-2020 Ralf Horstmann <ralf@ackstorm.de>
    Copyright (C) 2016-2020 Robert Lipe, robertlipe+source@gpsbabel.org

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
#include <QDataStream>
#include <QDebug>
#include <QIODevice>

#include <memory>

#include "ggv_bin.h"

/***************************************************************************
 *           local helper functions                                        *
 ***************************************************************************/

static int
ggv_bin_debug_level(const int* update = nullptr)
{
  static int debug_level = 0;
  if (update) {
    debug_level = *update;
  }
  return debug_level;
}

static void
ggv_bin_read_bytes(QDataStream& stream, QByteArray& buf, int len, const char* descr)
{
  if (len < 0) {
    qCritical().noquote()
        << QString("bin: Read error, negative len (%1)")
        .arg(descr ? descr : "");
    exit(1);
  }
  buf.resize(len);
  if (stream.readRawData(buf.data(), len) != len || stream.status() != QDataStream::Ok) {
    qCritical().noquote()
        << QString("bin: Read error (%1)")
        .arg(descr ? descr : "");
    exit(1);
  }
}

static quint16
ggv_bin_read16(QDataStream& stream, const char* descr)
{
  quint16 res = 0;
  stream >> res;
  if (stream.status() != QDataStream::Ok) {
    qCritical().noquote()
        << QString("bin: Read error (%1)")
        .arg(descr ? descr : "");
    exit(1);
  }
  if (ggv_bin_debug_level() > 1) {
    qDebug().noquote()
        << QString("bin: %1 %2 (0x%3)")
        .arg(descr, -15)
        .arg(res, 5)
        .arg(res, 4, 16, QChar('0'));
  }
  return res;
}

static quint32
ggv_bin_read32(QDataStream& stream, const char* descr)
{
  quint32 res = 0;
  stream >> res;
  if (stream.status() != QDataStream::Ok) {
    qCritical().noquote()
        << QString("bin: Read error (%1)")
        .arg(descr ? descr : "");
    exit(1);
  }
  if (ggv_bin_debug_level() > 1) {
    if ((res & 0xFFFF0000) == 0) {
      qDebug().noquote()
          << QString("bin: %1 %2 (0x%3)")
          .arg(descr, -15)
          .arg(res, 5)
          .arg(res, 8, 16, QChar('0'));
    } else {
      qDebug().noquote()
          << QString("bin: %1       (0x%2)")
          .arg(descr, -15)
          .arg(res, 8, 16, QChar('0'));
    }
  }
  return res;
}

static void
ggv_bin_read_text16(QDataStream& stream, QByteArray& buf, const char* descr)
{
  quint16 len = ggv_bin_read16(stream, descr);
  ggv_bin_read_bytes(stream, buf, len, descr);
  buf.append('\0');
  if (ggv_bin_debug_level() > 1) {
    qDebug().noquote()
        << "bin: text ="
        << QString::fromLatin1(buf.constData()).simplified();
  }
}

static void
ggv_bin_read_text32(QDataStream& stream, QByteArray& buf, const char* descr)
{
  quint32 len = ggv_bin_read32(stream, descr);
  // The following check prevents passing an unsigned int with a
  // value greater than INT32_MAX to a signed int parameter in
  // ggv_bin_read_bytes later on. If this happens, the file is
  // almost certainly corrupted.
  if (len > INT32_MAX) {
    qCritical().noquote()
        << QString("bin: Read error, max len exceeded (%1)")
        .arg(descr ? descr : "");
    exit(1);
  }
  ggv_bin_read_bytes(stream, buf, static_cast<int>(len), descr);
  buf.append('\0');
  if (ggv_bin_debug_level() > 1) {
    qDebug().noquote()
        << "bin: text ="
        << QString::fromLatin1(buf.constData()).simplified();
  }
}

static double
ggv_bin_read_double(QDataStream& stream, const char* descr)
{
  double res = 0.0;
  stream >> res;
  if (stream.status() != QDataStream::Ok) {
    qCritical().noquote()
        << QString("bin: Read error (%1)")
        .arg(descr ? descr : "");
  }
  return res;
}

/***************************************************************************
 *            OVL Version 2.0                                              *
 ***************************************************************************/

static void
ggv_bin_read_v2(QDataStream& stream, Geodata* geodata)
{
  QByteArray buf;
  QString track_name;
  QString waypt_name;
  quint16 line_points = 0;

  // header length is usually either 0x90 or 0x00
  quint16 header_len = ggv_bin_read16(stream, "map name len");
  if (header_len > 0) {
    ggv_bin_read_bytes(stream, buf, header_len, "map name");
    buf.remove(0,4);
    buf.append('\0');
    if (ggv_bin_debug_level() > 1) {
      qDebug().noquote() << "bin: name =" << buf.constData();
    }
  }

  while (!stream.atEnd()) {
    track_name.clear();

    if (ggv_bin_debug_level() > 1) {
      qDebug().noquote()
          << QString("------------------------------------ 0x%%1")
          .arg(stream.device()->pos(), 0, 16);
    }

    auto entry_pos = stream.device()->pos();
    quint16 entry_type = ggv_bin_read16(stream, "entry type");
    ggv_bin_read16(stream, "entry group");
    ggv_bin_read16(stream, "entry zoom");
    quint16 entry_subtype = ggv_bin_read16(stream, "entry subtype");

    if (entry_subtype != 1) {
      ggv_bin_read_text32(stream, buf, "text len");
      track_name = QString::fromLatin1(buf.constData()).simplified();
    }


    switch (entry_type) {
    case 0x02: {
      // text
      auto wpt = std::make_unique<Waypoint>();
      ggv_bin_read16(stream, "text color");
      ggv_bin_read16(stream, "text size");
      ggv_bin_read16(stream, "text trans");
      ggv_bin_read16(stream, "text font");
      ggv_bin_read16(stream, "text angle");
      wpt->longitude = ggv_bin_read_double(stream, "text lon");
      wpt->latitude = ggv_bin_read_double(stream, "text lat");
      ggv_bin_read_text16(stream, buf, "text label");
      wpt->name = QString::fromLatin1(buf.constData()).simplified();
      geodata->addWaypoint(wpt);
    }
    break;
    case 0x03:
    // line
    case 0x04: {
      // area
      auto ggv_bin_track = std::make_unique<WaypointList>();
      ggv_bin_read16(stream, "line color");
      ggv_bin_read16(stream, "line width");
      ggv_bin_read16(stream, "line type");
      line_points = ggv_bin_read16(stream, "line points");
      if (! track_name.isEmpty()) {
        ggv_bin_track->name = track_name;
      }

      for (int i = 1; i <= line_points; i++) {
        auto wpt = std::make_unique<Waypoint>();
        wpt->longitude = ggv_bin_read_double(stream, "line lon");
        wpt->latitude = ggv_bin_read_double(stream, "line lat");
        ggv_bin_track->addWaypoint(wpt);
      }
      geodata->addTrack(ggv_bin_track);
    }
    break;
    case 0x05:
    // rectangle
    case 0x06:
    // circle
    case 0x07:
      // triangle
      ggv_bin_read16(stream, "geom color");
      ggv_bin_read16(stream, "geom prop1");
      ggv_bin_read16(stream, "geom prop2");
      ggv_bin_read16(stream, "geom angle");
      ggv_bin_read16(stream, "geom stroke");
      ggv_bin_read16(stream, "geom area");
      ggv_bin_read_double(stream, "geom lon");
      ggv_bin_read_double(stream, "geom lat");
      break;
    case 0x09:
      ggv_bin_read16(stream, "bmp color");
      ggv_bin_read16(stream, "bmp prop1");
      ggv_bin_read16(stream, "bmp prop2");
      ggv_bin_read16(stream, "bmp prop3");
      ggv_bin_read_double(stream, "bmp lon");
      ggv_bin_read_double(stream, "bmp lat");
      ggv_bin_read_text32(stream, buf, "bmp data");
      break;
    default:
      qCritical().noquote()
          << QString("bin: Unknown entry type (0x%1, pos=0x%2)")
          .arg(entry_type, 0, 16)
          .arg(entry_pos, 0, 16);
      exit(1);
    }
  }
}

/***************************************************************************
 *           OVL Version 3.0 and 4.0                                       *
 ***************************************************************************/

static void
ggv_bin_read_v34_header(QDataStream& stream, quint32& number_labels, quint32& number_records)
{
  QByteArray buf;

  ggv_bin_read_bytes(stream, buf, 8, "unknown");
  number_labels = ggv_bin_read32(stream, "num labels");
  number_records = ggv_bin_read32(stream, "num records");
  ggv_bin_read_text16(stream, buf, "text label");
  ggv_bin_read16(stream, "unknown");
  ggv_bin_read16(stream, "unknown");
  // 8 bytes ending with 1E 00, contains len of header block
  ggv_bin_read16(stream, "unknown");
  quint16 header_len = ggv_bin_read16(stream, "header len");
  ggv_bin_read16(stream, "unknown");
  ggv_bin_read16(stream, "unknown");
  if (header_len > 0) {
    ggv_bin_read_bytes(stream, buf, header_len, "map name");
    buf.remove(0,4);
    buf.append('\0');
    if (ggv_bin_debug_level() > 1) {
      qDebug().noquote() << "bin: name =" << buf.constData();
    }
  }
}

static void
ggv_bin_read_v34_label(QDataStream& stream)
{
  QByteArray buf;

  if (ggv_bin_debug_level() > 1) {
    qDebug().noquote()
        << QString("------------------------------------ 0x%1")
        .arg(stream.device()->pos(), 0, 16);
  }
  ggv_bin_read_bytes(stream, buf, 0x08, "label header");
  ggv_bin_read_bytes(stream, buf, 0x14, "label number");
  ggv_bin_read_text16(stream, buf, "label text");
  ggv_bin_read16(stream, "label flag1");
  ggv_bin_read16(stream, "label flag2");
}

static QString
ggv_bin_read_v34_common(QDataStream& stream)
{
  QByteArray buf;

  ggv_bin_read16(stream, "entry group");
  ggv_bin_read16(stream, "entry prop2");
  ggv_bin_read16(stream, "entry prop3");
  ggv_bin_read16(stream, "entry prop4");
  ggv_bin_read16(stream, "entry prop5");
  ggv_bin_read16(stream, "entry prop6");
  ggv_bin_read16(stream, "entry prop7");
  ggv_bin_read16(stream, "entry prop8");
  ggv_bin_read16(stream, "entry zoom");
  ggv_bin_read16(stream, "entry prop10");
  ggv_bin_read_text16(stream, buf, "entry txt");
  QString res = QString::fromLatin1(buf.constData()).simplified();
  quint16 type1 = ggv_bin_read16(stream, "entry type1");
  if (type1 != 1) {
    ggv_bin_read_text32(stream, buf, "entry object");
  }
  quint16 type2 = ggv_bin_read16(stream, "entry type2");
  if (type2 != 1) {
    ggv_bin_read_text32(stream, buf, "entry object");
  }
  return res;
}

static void
ggv_bin_read_v34_record(QDataStream& stream, Geodata* geodata)
{
  QByteArray buf;
  quint32 bmp_len = 0;
  quint16 line_points = 0;

  if (ggv_bin_debug_level() > 1) {
    qDebug().noquote()
        << QString("------------------------------------ 0x%1")
        .arg(stream.device()->pos(), 0, 16);
  }

  quint16 entry_type = ggv_bin_read16(stream, "entry type");
  QString label = ggv_bin_read_v34_common(stream);

  switch (entry_type) {
  case 0x02: {
    // text
    auto wpt = std::make_unique<Waypoint>();
    ggv_bin_read16(stream, "text prop1");
    ggv_bin_read32(stream, "text prop2");
    ggv_bin_read16(stream, "text prop3");
    ggv_bin_read32(stream, "text prop4");
    ggv_bin_read16(stream, "text ltype");
    ggv_bin_read16(stream, "text angle");
    ggv_bin_read16(stream, "text size");
    ggv_bin_read16(stream, "text area");
    wpt->longitude = ggv_bin_read_double(stream, "text lon");
    wpt->latitude = ggv_bin_read_double(stream, "text lat");
    ggv_bin_read_double(stream, "text unk");
    ggv_bin_read_text16(stream, buf, "text label");
    wpt->name = QString::fromLatin1(buf.constData()).simplified();
    geodata->addWaypoint(wpt);
  }
  break;

  case 0x03:
  case 0x04:
  // area
  case 0x17: {
    // line
    auto ggv_bin_track = std::make_unique<WaypointList>();

    if (! label.isEmpty()) {
      ggv_bin_track->name = label;
    }

    ggv_bin_read16(stream, "line prop1");
    ggv_bin_read32(stream, "line prop2");
    ggv_bin_read16(stream, "line prop3");
    ggv_bin_read32(stream, "line color");
    ggv_bin_read16(stream, "line size");
    ggv_bin_read16(stream, "line stroke");
    line_points = ggv_bin_read16(stream, "line points");
    if (entry_type == 0x04) {
      // found in example.ovl generated by Geogrid-Viewer 1.0
      ggv_bin_read16(stream, "line pad");
    }

    for (int i=1; i <= line_points; i++) {
      auto wpt = std::make_unique<Waypoint>();
      wpt->longitude = ggv_bin_read_double(stream, "line lon");
      wpt->latitude = ggv_bin_read_double(stream, "line lat");
      ggv_bin_read_double(stream, "line unk");
      ggv_bin_track->addWaypoint(wpt);
    }

    geodata->addTrack(ggv_bin_track);
  }
  break;

  case 0x05:
  case 0x06:
  case 0x07:
    // circle
    ggv_bin_read16(stream, "circle prop1");
    ggv_bin_read32(stream, "circle prop2");
    ggv_bin_read16(stream, "circle prop3");
    ggv_bin_read32(stream, "circle color");
    ggv_bin_read32(stream, "circle prop5");
    ggv_bin_read32(stream, "circle prop6");
    ggv_bin_read16(stream, "circle ltype");
    ggv_bin_read16(stream, "circle angle");
    ggv_bin_read16(stream, "circle size");
    ggv_bin_read16(stream, "circle area");
    ggv_bin_read_double(stream, "circle lon");
    ggv_bin_read_double(stream, "circle lat");
    ggv_bin_read_double(stream, "circle unk");
    break;
  case 0x09:
    // bmp
    ggv_bin_read16(stream, "bmp prop1");
    ggv_bin_read32(stream, "bmp prop2");
    ggv_bin_read16(stream, "bmp prop3");
    ggv_bin_read32(stream, "bmp prop4");
    ggv_bin_read32(stream, "bmp prop5");
    ggv_bin_read32(stream, "bmp prop6");
    ggv_bin_read_double(stream, "bmp lon");
    ggv_bin_read_double(stream, "bmp lat");
    ggv_bin_read_double(stream, "bmp unk");
    bmp_len = ggv_bin_read32(stream, "bmp len");
    // The following check prevents passing an unsigned int with a
    // value greater than INT32_MAX to a signed int parameter in
    // ggv_bin_read_bytes later on. If this happens, the file is
    // almost certainly corrupted.
    if (bmp_len > INT32_MAX) {
      qCritical().noquote()
          << QString("bin: Read error, max bmp_len exceeded");
      exit(1);
    }
    ggv_bin_read16(stream, "bmp prop");
    ggv_bin_read_bytes(stream, buf, static_cast<int>(bmp_len), "bmp data");
    break;
  default:
    qCritical().noquote()
        << QString("bin: Unsupported type: %1")
        .arg(entry_type, 0, 16);
    exit(1);
  }
}

static void
ggv_bin_read_v34(QDataStream& stream, Geodata* geodata)
{
  QByteArray buf;
  quint32 label_count = 0;
  quint32 record_count = 0;

  while (!stream.atEnd()) {
    ggv_bin_read_v34_header(stream, label_count, record_count);

    if (label_count && !stream.atEnd()) {
      if (ggv_bin_debug_level() > 1) {
        qDebug().noquote()
            << QString("-----labels------------------------- 0x%1x")
            .arg(stream.device()->pos(), 0, 16);
      }
      for (unsigned int i = 0; i < label_count; i++) {
        ggv_bin_read_v34_label(stream);
      }
    }

    if (record_count && !stream.atEnd()) {
      if (ggv_bin_debug_level() > 1) {
        qDebug().noquote()
            << QString("-----records------------------------ 0x%1")
            .arg(stream.device()->pos(), 0, 16);
      }
      for (unsigned int i = 0; i < record_count; i++) {
        ggv_bin_read_v34_record(stream, geodata);
      }
    }

    if (!stream.atEnd()) {
      if (ggv_bin_debug_level() > 1) {
        qDebug().noquote()
            << QString("------------------------------------ 0x%1")
            .arg(stream.device()->pos(), 0, 16);
      }
      // we just skip over the next magic bytes without checking they
      // contain the correct string. This is consistent with what I
      // believe GGV does
      ggv_bin_read_bytes(stream, buf, 23, "magicbytes");
      if (ggv_bin_debug_level() > 1) {
        qDebug().noquote() << "bin: header = " << buf.constData();
      }
    }
  }

  if (ggv_bin_debug_level() > 1) {
    qDebug().noquote()
        << QString("fpos: 0x%1")
        .arg(stream.device()->pos(), 0, 16);
    qDebug().noquote()
        << QString("size: 0x%1")
        .arg(stream.device()->size(), 0, 16);
  }
}

/***************************************************************************
 *              entry points called by ggvtogpx main process               *
 ***************************************************************************/

bool
GgvBinFormat::probe(QIODevice* io)
{
  int debug_level = getDebugLevel();
  ggv_bin_debug_level(&debug_level);

  io->reset();
  QDataStream stream(io);
  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
  stream.setByteOrder(QDataStream::LittleEndian);

  QByteArray buf;
  ggv_bin_read_bytes(stream, buf, 0x17, "magic");
  buf.append('\0');
  if (ggv_bin_debug_level() > 1) {
    qDebug().noquote() << "bin: header =" << buf.constData();
  }

  if (buf.startsWith("DOMGVCRD Ovlfile V2.0")) {
    return true;
  } else if (buf.startsWith("DOMGVCRD Ovlfile V3.0")) {
    return true;
  } else if (buf.startsWith("DOMGVCRD Ovlfile V4.0")) {
    return true;
  } else {
    return false;
  }
}

void
GgvBinFormat::read(QIODevice* io, Geodata* geodata)
{
  int debug_level = getDebugLevel();
  ggv_bin_debug_level(&debug_level);

  io->reset();
  QDataStream stream(io);
  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
  stream.setByteOrder(QDataStream::LittleEndian);

  QByteArray buf;
  ggv_bin_read_bytes(stream, buf, 0x17, "magic");
  buf.append('\0');
  if (ggv_bin_debug_level() > 1) {
    qDebug().noquote() << "bin: header =" << buf.constData();
  }

  if (buf.startsWith("DOMGVCRD Ovlfile V2.0")) {
    ggv_bin_read_v2(stream, geodata);
  } else if (buf.startsWith("DOMGVCRD Ovlfile V3.0")) {
    ggv_bin_read_v34(stream, geodata);
  } else if (buf.startsWith("DOMGVCRD Ovlfile V4.0")) {
    ggv_bin_read_v34(stream, geodata);
  } else {
    qCritical().noquote() << QString("bin: Unsupported file format");
    exit(1);
  }
}

const QString GgvBinFormat::getName()
{
  return "ggv_bin";
};
