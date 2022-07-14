/*

    Geodata storage for Waypoints, Routes and Tracks

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

#ifndef GEODATA_H_INCLUDED_
#define GEODATA_H_INCLUDED_

#include <QString>

#include <list>
#include <memory>

class Waypoint
{
public:
  Waypoint() : latitude(0.0), longitude(0.0) {};
  Waypoint(double lat, double lon) : latitude(lat), longitude(lon) {};
  Waypoint(double lat, double lon, QString& n) : latitude(lat), longitude(lon), name(n) {}
  double latitude;
  double longitude;
  QString name;
};

class WaypointList
{
public:
  WaypointList() = default;

  void addWaypoint(std::unique_ptr<Waypoint>& waypoint);
  const std::list<std::unique_ptr<Waypoint>>& getWaypoints() const;

  QString name;
private:
  std::list<std::unique_ptr<Waypoint>> waypoint_list;
};

class Geodata
{
public:
  Geodata() : debuglevel(0) {};
  void addWaypoint(std::unique_ptr<Waypoint>& waypoint);
  void addRoute(std::unique_ptr<WaypointList>& route);
  void addTrack(std::unique_ptr<WaypointList>& route);

  const std::list<std::unique_ptr<Waypoint>>& getWaypoints() const;
  const std::list<std::unique_ptr<WaypointList>>& getRoutes() const;
  const std::list<std::unique_ptr<WaypointList>>& getTracks() const;

  std::pair<Waypoint,Waypoint> getBounds() const;

  void setDebugLevel(int _debuglevel);
  int getDebugLevel();
private:
  std::list<std::unique_ptr<Waypoint>> waypoints;
  std::list<std::unique_ptr<WaypointList>> routes;
  std::list<std::unique_ptr<WaypointList>> tracks;
  int debuglevel;
};

#endif
