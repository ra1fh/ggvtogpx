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

#include <QDebug>
#include <QString>

#include <utility>

#include "geodata.h"

/**********************************************************************/

void
WaypointList::addWaypoint(std::unique_ptr<Waypoint>& waypoint)
{
  waypoint_list.push_back(std::move(waypoint));
};

std::unique_ptr<Waypoint>
WaypointList::extractFirstWaypoint()
{
  auto ret = std::move(waypoint_list.front());
  waypoint_list.pop_front();
  return ret;
};

const std::list<std::unique_ptr<Waypoint>>&
                                        WaypointList::getWaypoints() const
{
  return waypoint_list;
}

/**********************************************************************/

const std::list<std::unique_ptr<Waypoint>>&
                                        Geodata::getWaypoints() const
{
  return waypoints;
};

const std::list<std::unique_ptr<WaypointList>>&
    Geodata::getRoutes() const
{
  return routes;
};

const std::list<std::unique_ptr<WaypointList>>&
    Geodata::getTracks() const
{
  return tracks;
};

void
Geodata::setDebugLevel(int _debuglevel)
{
  debuglevel = _debuglevel;
};

int
Geodata::getDebugLevel()
{
  return debuglevel;
};

void
Geodata::addWaypoint(std::unique_ptr<Waypoint>& waypoint)
{
  if (getDebugLevel() > 2) {
    qDebug() << "waypt_add()";
  }
  waypoints.push_back(std::move(waypoint));
};


void
Geodata::addTrack(std::unique_ptr<WaypointList>& track)
{
  if (getDebugLevel() > 2) {
    qDebug() << "track_add_head()";
  }
  tracks.push_back(std::move(track));
};

void
Geodata::addRoute(std::unique_ptr<WaypointList>& route)
{
  if (getDebugLevel() > 2) {
    qDebug() << "route_add_head()";
  }
  routes.push_back(std::move(route));
};

std::pair<Waypoint,Waypoint>
Geodata::getBounds() const
{
  const double kMinLat = 0.0;
  const double kMaxLat = 90.0;
  const double kMinLon = -180.0;
  const double kMaxLon = 180.0;

  Waypoint min(kMaxLat, kMaxLon);
  Waypoint max(kMinLat, kMinLon);

  for (auto&& route : std::as_const(getRoutes())) {
    for (auto&& waypoint : std::as_const(route->getWaypoints())) {
      if (waypoint->latitude > max.latitude) {
        max.latitude = waypoint->latitude;
      }
      if (waypoint->latitude < min.latitude) {
        min.latitude = waypoint->latitude;
      }
      if (waypoint->longitude > max.longitude) {
        max.longitude = waypoint->longitude;
      }
      if (waypoint->longitude < min.longitude) {
        min.longitude = waypoint->longitude;
      }
    }
  }
  for (auto&& track : std::as_const(getTracks())) {
    for (auto&& waypoint : std::as_const(track->getWaypoints())) {
      if (waypoint->latitude > max.latitude) {
        max.latitude = waypoint->latitude;
      }
      if (waypoint->latitude < min.latitude) {
        min.latitude = waypoint->latitude;
      }
      if (waypoint->longitude > max.longitude) {
        max.longitude = waypoint->longitude;
      }
      if (waypoint->longitude < min.longitude) {
        min.longitude = waypoint->longitude;
      }
    }
  }
  for (auto&& waypoint : std::as_const(getWaypoints())) {
    if (waypoint->latitude > max.latitude) {
      max.latitude = waypoint->latitude;
    }
    if (waypoint->latitude < min.latitude) {
      min.latitude = waypoint->latitude;
    }
    if (waypoint->longitude > max.longitude) {
      max.longitude = waypoint->longitude;
    }
    if (waypoint->longitude < min.longitude) {
      min.longitude = waypoint->longitude;
    }
  }
  return std::make_pair(min,max);
}
