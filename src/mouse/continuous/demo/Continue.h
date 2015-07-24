#pragma once

#include "../../IMouseAlgorithm.h"

namespace demo {

class Continue : public IMouseAlgorithm {

public:
    std::string mouseFile() const;
    sim::InterfaceType interfaceType() const;
    void solve(int mazeWidth, int mazeHeight, sim::MouseInterface* mouse);

private:
    bool wallRight();
    bool wallFront();

    void turnRight();
    void turnLeft();
    void moveForward();
    void correctErrors();

    sim::MouseInterface* m_mouse;

};

} // namespace demo