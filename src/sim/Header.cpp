#include "Header.h"

#include "Directory.h"
#include "Layout.h"
#include "Logging.h"
#include "Param.h"
#include "SimUtilities.h"
#include "State.h"
#include "Time.h"

namespace sim {

Header::Header(Model* model) :
        m_model(model),
        m_windowWidth(0),
        m_windowHeight(0),
        m_textHeight(P()->headerTextHeight()),
        m_rowSpacing(P()->headerRowSpacing()),
        m_columnSpacing(P()->headerColumnSpacing()) {

    // Check to make sure that the font file exists
    std::string fontPath = Directory::getResFontsDirectory() + P()->headerTextFont();
    if (!SimUtilities::isFile(fontPath)) {
        // If the font doesn't exist, we simply draw no text whatsoever
        L()->warn(
            "\"%v\" is not a valid font file; it's very possible that the file"
            " does not exist. No header will drawn.", fontPath);
    }

    // Create the text drawer object
    m_textDrawer = new TextDrawer(fontPath, m_textHeight);

    // Populate the lines with initial values
    updateLines();
}

int Header::getHeight() const {
    int numRows = getNumRows(m_lines.size(), m_columnStartingPositions.size());
    return P()->windowBorderWidth() + numRows * m_textHeight + (numRows - 1) * m_rowSpacing;
}

void Header::updateWindowSize(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
    m_columnStartingPositions = getColumnStartingPositions();
}

void Header::draw() {

    // Update the lines of text to be drawn each and every frame
    updateLines();

    // Get the current number of rows (based on current lines and columnStartingPositions)
    int numRows = getNumRows(m_lines.size(), m_columnStartingPositions.size());

    // Draw all of the text for the frame
    m_textDrawer->commenceDrawingTextForFrame();
    for (int i = 0; i < m_columnStartingPositions.size(); i += 1) {
        for (int j = 0; j < numRows && i * numRows + j < m_lines.size(); j += 1) {
            m_textDrawer->drawText(
                m_columnStartingPositions.at(i),
                m_windowHeight - P()->windowBorderWidth() - m_textHeight - j * (m_textHeight + m_rowSpacing),
                m_windowWidth,
                m_windowHeight,
                m_lines.at(i * numRows + j)
            );
        }
    }
    m_textDrawer->concludeDrawingTextForFrame();
}

std::vector<int> Header::getColumnStartingPositions() const {

    // This uses:
    // - m_windowWidth
    // - m_lines
    // - m_columnSpacing

    static auto willFit = [&](const std::vector<double>& columnWidths) {
        double sum = 0.0;
        for (double width : columnWidths) {
            sum += width;
        }
        sum += 2 * P()->windowBorderWidth();
        sum += (columnWidths.size() - 1) * m_columnSpacing;
        return sum <= m_windowWidth;
    };

    static auto getColumnWidths = [&](int numCols) {
        std::vector<double> columnWidths;
        int numRows = getNumRows(m_lines.size(), numCols);
        for (int i = 0; i < numCols; i += 1) {
            double maxLineWidth = 0.0;
            for (int j = 0; j < numRows && i * numRows + j < m_lines.size(); j += 1) {
                double lineWidth = m_textDrawer->getWidth(m_lines.at(i * numRows + j));
                if (maxLineWidth < lineWidth) {
                    maxLineWidth = lineWidth;
                }
            }
            columnWidths.push_back(maxLineWidth);
        }
        return columnWidths;
    };

    // Determine the optimal column widths
    std::vector<double> columnWidths;
    int numCols = 1;
    do {
        columnWidths = getColumnWidths(numCols);
        numCols += 1;
    }
    while (willFit(getColumnWidths(numCols)));

    // Calculate the starting positions based on the column widths
    std::vector<int> columnStartingPositions;
    int currentColumnStart = P()->windowBorderWidth();
    for (int i = 0; i < columnWidths.size(); i += 1) {
        columnStartingPositions.push_back(currentColumnStart);
        currentColumnStart += columnWidths.at(i) + m_columnSpacing;
    }
    return columnStartingPositions;
}

int Header::getNumRows(int numLines, int numCols) const {
    return numLines / numCols + (numLines % numCols == 0 ? 0 : 1);
}

void Header::updateLines() {
    m_lines = {
        std::string("Run ID: ") + S()->runId(),
        // std::string("Run Dir: ") + Directory::getRunDirectory(),
        (
            P()->useMazeFile() ?
            std::string("Maze File: ") + P()->mazeFile() :
            std::string("Maze Algo: ") + P()->mazeAlgorithm()
        ),
        std::string("Mouse Algo: ") + P()->mouseAlgorithm(),
        std::string("Random Seed: ") + std::to_string(P()->randomSeed()),
        std::string(""), // Separator
        std::string("Crashed: ") + (S()->crashed() ? "TRUE" : "FALSE"),
        std::string("Layout Type (l): ") + LAYOUT_TYPE_TO_STRING.at(S()->layoutType()),
        std::string("Rotate Zoomed Map (r): ") + (S()->rotateZoomedMap() ? "TRUE" : "FALSE"),
        std::string("Zoomed Map Scale (i, o): ") + std::to_string(S()->zoomedMapScale()),
        std::string("Wall Truth Visible (t): ") + (S()->wallTruthVisible() ? "TRUE" : "FALSE"),
        std::string("Tile Colors Visible (c): ") + (S()->tileColorsVisible() ? "TRUE" : "FALSE"),
        std::string("Tile Fog Visible (g): ") + (S()->tileFogVisible() ? "TRUE" : "FALSE"),
        std::string("Tile Text Visible (x): ") + (S()->tileTextVisible() ? "TRUE" : "FALSE"),
        std::string("Tile Distance Visible (d): ") + (S()->tileDistanceVisible() ? "TRUE" : "FALSE"),
        std::string("Wireframe Mode (w): ") + (S()->wireframeMode() ? "TRUE" : "FALSE"),
        std::string("Paused (p): ") + (S()->paused() ? "TRUE" : "FALSE"),
        std::string("Sim Speed (f, s): ") + std::to_string(S()->simSpeed()),
        std::string(""), // Separator
        std::string("Tiles Traversed: ") + std::to_string(m_model->getWorld()->getNumberOfTilesTraversed())
             + "/" + std::to_string(m_model->getMaze()->getWidth() * m_model->getMaze()->getHeight()),
        std::string("Closest Distance to Center: ") + std::to_string(m_model->getWorld()->getClosestDistanceToCenter()),
        std::string("Current X (m):          ") + std::to_string(m_model->getMouse()->getCurrentTranslation().getX().getMeters()),
        std::string("Current Y (m):          ") + std::to_string(m_model->getMouse()->getCurrentTranslation().getY().getMeters()),
        std::string("Current Rotation (deg): ") + std::to_string(m_model->getMouse()->getCurrentRotation().getDegreesZeroTo360()),
        std::string("Current X tile:         ") + std::to_string(m_model->getMouse()->getCurrentDiscretizedTranslation().first),
        std::string("Current Y tile:         ") + std::to_string(m_model->getMouse()->getCurrentDiscretizedTranslation().second),
        std::string("Current Direction:      ") + DIRECTION_TO_STRING.at(m_model->getMouse()->getCurrentDiscretizedRotation()),
        std::string("Elapsed Real Time:           ") + SimUtilities::formatSeconds(T()->elapsedRealTime().getSeconds()),
        std::string("Elapsed Sim Time:            ") + SimUtilities::formatSeconds(T()->elapsedSimTime().getSeconds()),
        std::string("Time Since Origin Departure: ") + (
            m_model->getWorld()->getTimeSinceOriginDeparture().getSeconds() < 0 ? "NONE" :
            SimUtilities::formatSeconds(m_model->getWorld()->getTimeSinceOriginDeparture().getSeconds())
        ),
        std::string("Best Time to Center:         ") + (
            m_model->getWorld()->getBestTimeToCenter().getSeconds() < 0 ? "NONE" :
            SimUtilities::formatSeconds(m_model->getWorld()->getBestTimeToCenter().getSeconds())
        ),
    };
}

} // namespace sim