#pragma once

#include <mutex>
#include <vector>

#include <Cartesian.h>
#include <RadiansPerSecond.h>

#include "Maze.h"
#include "Polygon.h"
#include "Sensor.h"
#include "Wheel.h"

namespace sim {

class Mouse {

public:
    Mouse(Maze* maze);

    // Retrieves the polygon comprised of all parts of the mouse that could collide with walls
    Polygon getCollisionPolygon() const;

    // Retrieves the polygon of just the body of the mouse
    Polygon getBodyPolygon() const;

    // Retrieves the polygons of the wheels of the robot
    std::vector<Polygon> getWheelPolygons() const;

    // Retrieves the polygons of the sensors of the robot
    std::vector<Polygon> getSensorPolygons() const;

    // Retrieve the polygons corresponding to the views of the sensors
    std::vector<Polygon> getViewPolygons() const;

    // Instruct the mouse to update its own position based on how much simulation time has elapsed
    void update(const Time& elapsed);

    // An atomic interface for setting the wheel speeds
    void setWheelSpeeds(const AngularVelocity& leftWheelSpeed, const AngularVelocity& rightWheelSpeed);

    // Read a sensor, and returns a value from 0.0 (completely free) to 1.0 (completely blocked)
    float read(std::string name) const;

    // Get the read time of a particular sensor
    Seconds getReadTime(std::string name) const;

private:
    // Used for the sensor readings
    const Maze* m_maze;

    // The mouse, as it's positioned at the start execution
    Cartesian m_initialTranslation;
    Polygon m_initialBodyPolygon; // The polygon of strictly the body of the mouse
    Polygon m_initialCollisionPolygon; // The polygon containing all collidable parts of the mouse

    // The rotation and translation of the mouse, which change throughout execution
    Radians m_rotation;
    Cartesian m_translation;

    // By assumption, the mouse is differential drive.
    Wheel m_leftWheel;
    Wheel m_rightWheel;
    std::mutex m_wheelMutex; // Ensures the wheel speeds are accessed atomically w.r.t. each other

    // The sensors on the mouse
    std::map<std::string, Sensor> m_sensors;
};

} // namespace sim
