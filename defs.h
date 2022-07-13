/*

    simplified defs.h

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

#ifndef DEFS_H_INCLUDED_
#define DEFS_H_INCLUDED_

#include <QList>
#include <QString>

[[noreturn]] void fatal(const char* fmt, ...);
void warning(const char* fmt, ...);

class Format
{
public:
  Format() = default;
  virtual ~Format() = default;

  Format(const Format&) = delete;
  Format& operator=(const Format&) = delete;
  Format(Format&&) = delete;
  Format& operator=(Format&&) = delete;

  virtual void rd_init(const QString&) {};
  virtual void rd_deinit() {};
  virtual void read() {};

  virtual void wr_init(const QString&) {};
  virtual void wr_deinit() {};
  virtual void write() {};

  QString get_name() const
  {
    return name;
  };
  void set_name(const QString& _name)
  {
    name = _name;
  };

private:
  QString name;
};

struct Waypoint {
  double latitude;
  double longitude;
  double altitude;
  double geoidheight;
  QString description;
};

class Route
{
public:
  QList<Waypoint*> waypoint_list;
  QString route_name;
  QString route_desc;
  Route() = default;
  Route(const Route& other) = delete;
  Route& operator=(const Route& rhs) = delete;
  ~Route();
};

int get_debug_level();
void waypt_add(Waypoint*);
void track_add_head(Route*);
void track_add_wpt(Route*, Waypoint*);
void route_add_head(Route*);
void route_add_wpt(Route*, Waypoint*);

#endif
