#pragma once

#include <QObject>

#include "Maze.h"
#include "Mouse.h"
#include "MouseStats.h"

namespace mms {

// TODO: MACK - rename to model
class World : public QObject {

    Q_OBJECT

public:
    World();

    void setMaze(const Maze* maze);
    void addMouse(const QString& name, Mouse* mouse);

    MouseStats getMouseStats(const QString& name) const;

    // TODO: MACK - call this init (World is actually a singleton)
    void simulate();

signals:
    void newTileLocationTraversed(int x, int y);

private:
    const Maze* m_maze;
    QMap<QString, Mouse*> m_mice;
    QMap<QString, MouseStats> m_stats;

    void checkCollision();
};

} // namespace mms
