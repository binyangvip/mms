#include "Mouse.h"

#include <cmath>
#include <iostream>

#include <Meters.h>
#include <MetersPerSecond.h>
#include <Polar.h>

#include "Assert.h"
#include "GeometryUtilities.h"
#include "MouseParser.h"
#include "Param.h"
#include "SimUtilities.h"

namespace sim {

Mouse::Mouse(Maze* maze) : m_maze(maze), m_rotation(Radians(0.0)) {

    // Create the mouse parser object
    MouseParser parser(getProjectDirectory() + "res/mouse.xml");

    // Initialize the body of the mouse
    m_initialBodyPolygon = parser.getBody();

    // Initialize the wheels
    m_leftWheel = parser.getLeftWheel();
    m_rightWheel = parser.getRightWheel();

    // TODO: Validate the contents of the mouse file (like valid mouse starting position)
    // Note: The position of the wheels must be the exact same at the start of execution
    ASSERT(m_leftWheel.getInitialTranslation().getY() == m_rightWheel.getInitialTranslation().getY());

    // Reassign the translation to be the midpoint of the axis connecting the two wheels
    m_initialTranslation = Cartesian(
        Meters((m_leftWheel.getInitialTranslation().getX() + m_rightWheel.getInitialTranslation().getX()) / 2.0),
        Meters((m_leftWheel.getInitialTranslation().getY() + m_rightWheel.getInitialTranslation().getY()) / 2.0));
    m_translation = m_initialTranslation;

    // Initialize the sensors
    m_sensors = parser.getSensors();

    // Initialize the collision polygon
    std::vector<Polygon> polygons;
    polygons.push_back(m_initialBodyPolygon);
    polygons.push_back(m_rightWheel.getInitialPolygon());
    polygons.push_back(m_leftWheel.getInitialPolygon());
    for (std::pair<std::string, Sensor> pair : m_sensors) {
        polygons.push_back(pair.second.getInitialPolygon());
    }
    // TODO: SOM: This should be changed to getUnion instead of convexHull, once it's ready
    m_initialCollisionPolygon = convexHull(polygons);
}

Polygon Mouse::getCollisionPolygon() const {
    return m_initialCollisionPolygon.translate(m_translation - m_initialTranslation).rotateAroundPoint(m_rotation, m_translation);
}

Polygon Mouse::getBodyPolygon() const {
    return m_initialBodyPolygon.translate(m_translation - m_initialTranslation).rotateAroundPoint(m_rotation, m_translation);
}

std::vector<Polygon> Mouse::getWheelPolygons() const {

    // Get the initial wheel polygons
    std::vector<Polygon> initialPolygons;
    initialPolygons.push_back(m_rightWheel.getInitialPolygon());
    initialPolygons.push_back(m_leftWheel.getInitialPolygon());

    // Translate and rotate the polygons appropriately
    std::vector<Polygon> adjustedPolygons;
    for (Polygon polygon : initialPolygons) {
        adjustedPolygons.push_back(
            polygon.translate(m_translation - m_initialTranslation).rotateAroundPoint(m_rotation, m_translation));
    }

    return adjustedPolygons;
}

std::vector<Polygon> Mouse::getSensorPolygons() const {

    // Get the initial sensor polygons
    std::vector<Polygon> initialPolygons;
    for (std::pair<std::string, Sensor> pair : m_sensors) {
        initialPolygons.push_back(pair.second.getInitialPolygon());
    }

    // Translate and rotate the polygons appropriately
    std::vector<Polygon> adjustedPolygons;
    for (Polygon polygon : initialPolygons) {
        adjustedPolygons.push_back(
            polygon.translate(m_translation - m_initialTranslation).rotateAroundPoint(m_rotation, m_translation));
    }

    return adjustedPolygons;
}

std::vector<Polygon> Mouse::getViewPolygons() const {

    // Get the current view for the all of the sensors
    std::vector<Polygon> polygons;
    for (std::pair<std::string, Sensor> pair : m_sensors) {
        Polygon adjusted = pair.second.getInitialView().translate(m_translation - m_initialTranslation)
            .rotateAroundPoint(m_rotation, m_translation);
        polygons.push_back(pair.second.getCurrentView(
            adjusted.getVertices().at(0), m_rotation + pair.second.getInitialRotation(), *m_maze));
    }
    return polygons;
}

void Mouse::update(const Time& elapsed) {

    /*
     *  In a differential drive system (two-wheeled drive system), the angular velocities of
     *  each of the two wheels completely determine the movement of the robot. The equations
     *  for the instantaneous change in rotation and translation (with respect to the robot)
     *  are as follows:
     *
     *      dx/dt = (rightWheelSpeed - leftWheelSpeed) / 2
     *      dy/dt = 0
     *      d0/dt = (rightWheelSpeed + leftWheelSpeed) / base
     *
     *  where the coordinate axes with respect to the robot are as follows:
     *
     *               x
     *               ^
     *               |
     *               |
     *              / \
     *      y <----0---0
     *
     *  Note that dx/dy = 0 since it's impossible for the robot to move laterally. Also note
     *  that since the left and right wheels are oriented oppositely, a positive wheel speed
     *  of the right wheel means that the wheel moves in the positive y direction (with
     *  respect to the robot) while a positive wheel speed of the left wheel means that the
     *  wheel moves in the negative y directions (again, with respect to the robot). Given
     *  these few equations, we can easily approximate the motion of the robot with respect
     *  to the maze by multiplying the instantaneous rate of change in the translation and
     *  rotation with the elapsed time. This is certainly an approximation because the rotation
     *  of the robot is not constant throughout the duration of the motion. Thus, while the
     *  rate of change of rotation is not a function of time, the rate of change of the
     *  translation of robot (with respect the the maze) is a function of time. While there
     *  is a closed form solution for the translation of the robot given a non-zero rate of
     *  rotation of the robot, it's unnecessary to use it here. Our elapsed times should be
     *  small and thus the change in rotation should be mostly negligible.
     */

    // First get the speed of each wheel (atomically)
    m_wheelMutex.lock();
    MetersPerSecond rightWheelSpeed(
        m_rightWheel.getAngularVelocity().getRadiansPerSecond() * m_rightWheel.getRadius().getMeters());
    MetersPerSecond leftWheelSpeed(
        m_leftWheel.getAngularVelocity().getRadiansPerSecond() * m_leftWheel.getRadius().getMeters());
    m_wheelMutex.unlock();

    // Then get the distance between the two wheels
    Meters base(m_rightWheel.getInitialTranslation().getX() - m_leftWheel.getInitialTranslation().getX());

    // Update the rotation
    m_rotation += Radians((rightWheelSpeed + leftWheelSpeed).getMetersPerSecond() / base.getMeters() * elapsed.getSeconds());

    // Update the translation
    Meters distance((rightWheelSpeed - leftWheelSpeed).getMetersPerSecond() / 2.0 * elapsed.getSeconds());
    m_translation += Polar(distance, Wheel().getInitialRotation() + m_rotation); // This could be optimized

    // TODO: Update the cached sensor values with each update

    // -----------------------------------------------------------------------------------------------------

    // TODO: This is the technically corect implementation...
    /*
    float BASELINE(m_rightWheel.getInitialTranslation().getX().getMeters() - m_leftWheel.getInitialTranslation().getX().getMeters());

    if (fabs(rightWheelSpeed.getMetersPerSecond() == -1*leftWheelSpeed.getMetersPerSecond())) { // TODO
        Meters distance((-0.5 * leftWheelSpeed.getMetersPerSecond() + 0.5 * rightWheelSpeed.getMetersPerSecond()) * elapsed.getSeconds());
        m_translation = m_translation + Polar(distance, Radians(M_PI / 2.0) + m_rotation); // TODO
        static int i = 0;
        std::cout << i++ << std::endl;
    }
    else {
        leftWheelSpeed = MetersPerSecond(-1*leftWheelSpeed.getMetersPerSecond());

        // TODO
        float x_0 = m_translation.getY().getMeters();
        float y_0 = -1*m_translation.getX().getMeters();

        float x = x_0 +

                  (BASELINE*(rightWheelSpeed+leftWheelSpeed).getMetersPerSecond())
                  /(2.0*(rightWheelSpeed-leftWheelSpeed).getMetersPerSecond())

                  *(sin((rightWheelSpeed-leftWheelSpeed).getMetersPerSecond() * elapsed.getSeconds() / BASELINE + m_rotation.getRadians())
                   -sin(m_rotation.getRadians()));

        float y = y_0 -

                  (BASELINE*(rightWheelSpeed+leftWheelSpeed).getMetersPerSecond())
                  /(2.0*(rightWheelSpeed-leftWheelSpeed).getMetersPerSecond())

                  *(cos((rightWheelSpeed-leftWheelSpeed).getMetersPerSecond() * elapsed.getSeconds() / BASELINE + m_rotation.getRadians())
                   -cos(m_rotation.getRadians()));

        m_translation = Cartesian(Meters(-y), Meters(x));

        leftWheelSpeed = MetersPerSecond(-1*leftWheelSpeed.getMetersPerSecond());
    }

    // TODO: This is correct
    Radians theta((rightWheelSpeed.getMetersPerSecond() - (-leftWheelSpeed.getMetersPerSecond())) / BASELINE * elapsed.getSeconds());
    m_rotation = m_rotation + theta;
    */
}

void Mouse::setWheelSpeeds(const AngularVelocity& leftWheelSpeed, const AngularVelocity& rightWheelSpeed) {
    m_wheelMutex.lock();
    m_leftWheel.setAngularVelocity(leftWheelSpeed);
    m_rightWheel.setAngularVelocity(rightWheelSpeed);
    m_wheelMutex.unlock();
}

float Mouse::read(std::string name) const {
    // TODO: Ensure the mouse doesn't move while we're reading values??
    ASSERT(m_sensors.count(name) != 0);
    Sensor sensor = m_sensors.at(name);
    Polygon fullView = sensor.getInitialView().translate(m_translation - m_initialTranslation)
        .rotateAroundPoint(m_rotation, m_translation);
    Polygon currentView = sensor.getCurrentView(
        fullView.getVertices().at(0), m_rotation + sensor.getInitialRotation(), *m_maze);
    return 1.0 - polygonArea(currentView).getMetersSquared() / polygonArea(fullView).getMetersSquared();
}

Seconds Mouse::getReadTime(std::string name) const {
    ASSERT(m_sensors.count(name) != 0);
    return m_sensors.at(name).getReadTime();
}

} // namespace sim
