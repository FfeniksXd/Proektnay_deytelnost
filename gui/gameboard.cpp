#include "gameboard.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>

GameBoard::GameBoard(QWidget* parent) : QWidget(parent), cells_(9, Cell::Empty) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(220, 220);
    setAccessibleName(tr("Игровое поле"));
    setAccessibleDescription(tr("Стрелки выбирают клетку, Enter подтверждает выбор."));
}

void GameBoard::reset(int width, int height) {
    columns_ = width;
    rows_ = height;
    selectedRow_ = 0;
    selectedColumn_ = 0;
    cells_.assign(columns_ * rows_, Cell::Empty);
    update();
}

bool GameBoard::contains(int row, int column) const {
    return row >= 0 && row < rows_ && column >= 0 && column < columns_;
}

void GameBoard::setCell(int row, int column, Cell value) {
    if (contains(row, column)) {
        cells_[row * columns_ + column] = value;
        update();
    }
}

QRect GameBoard::gridRect() const {
    const int cellSize = std::min((width() - 4) / columns_, (height() - 4) / rows_);
    const QSize gridSize(columns_ * cellSize, rows_ * cellSize);
    return QRect(QPoint((width() - gridSize.width()) / 2,
                        (height() - gridSize.height()) / 2), gridSize);
}

void GameBoard::paintEvent(QPaintEvent*) {
    const QRect grid = gridRect();
    const int cellSize = grid.width() / columns_;
    QPainter painter(this);
    painter.fillRect(grid, QColor("#031109"));
    painter.setPen(QColor("#3f995b"));

    for (int col = 0; col <= columns_; ++col) {
        const int x = grid.x() + col * cellSize;
        painter.drawLine(x, grid.y(), x, grid.y() + grid.height());
    }
    for (int row = 0; row <= rows_; ++row) {
        const int y = grid.y() + row * cellSize;
        painter.drawLine(grid.x(), y, grid.x() + grid.width(), y);
    }

    QFont markFont = font();
    markFont.setBold(true);
    markFont.setPixelSize(std::max(7, cellSize * 3 / 5));
    painter.setFont(markFont);

    for (int row = 0; row < rows_; ++row) {
        for (int column = 0; column < columns_; ++column) {
            const Cell mark = cells_[row * columns_ + column];
            if (mark != Cell::Empty) {
                const QRect cell(grid.x() + column * cellSize, grid.y() + row * cellSize,
                                 cellSize, cellSize);
                painter.setPen(mark == Cell::X ? QColor("#a1ffad") : QColor("#69de87"));
                painter.drawText(cell, Qt::AlignCenter, mark == Cell::X ? "X" : "O");
            }
        }
    }

    if (hasFocus()) {
        const int inset = std::clamp(cellSize / 10, 2, 7);
        const QRect selection(grid.x() + selectedColumn_ * cellSize + inset,
                              grid.y() + selectedRow_ * cellSize + inset,
                              cellSize - 2 * inset, cellSize - 2 * inset);
        painter.setPen(QPen(QColor("#a1ffad"), 1, Qt::DashLine));
        painter.drawRect(selection);
    }
}

void GameBoard::activateSelection() {
    if (cells_[selectedRow_ * columns_ + selectedColumn_] == Cell::Empty) {
        emit cellActivated(selectedRow_, selectedColumn_);
    }
}

void GameBoard::mousePressEvent(QMouseEvent* event) {
    const QRect grid = gridRect();
    const QPoint point = event->position().toPoint();
    if (event->button() != Qt::LeftButton || !grid.contains(point)) {
        QWidget::mousePressEvent(event);
        return;
    }

    const int cellSize = grid.width() / columns_;
    selectedColumn_ = (point.x() - grid.x()) / cellSize;
    selectedRow_ = (point.y() - grid.y()) / cellSize;
    setFocus(Qt::MouseFocusReason);
    update();
    activateSelection();
}

void GameBoard::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Left:
        selectedColumn_ = std::max(0, selectedColumn_ - 1);
        break;
    case Qt::Key_Right:
        selectedColumn_ = std::min(columns_ - 1, selectedColumn_ + 1);
        break;
    case Qt::Key_Up:
        selectedRow_ = std::max(0, selectedRow_ - 1);
        break;
    case Qt::Key_Down:
        selectedRow_ = std::min(rows_ - 1, selectedRow_ + 1);
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        if (!event->isAutoRepeat()) {
            activateSelection();
        }
        return;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    update();
}

void GameBoard::focusInEvent(QFocusEvent* event) {
    QWidget::focusInEvent(event);
    update();
}

void GameBoard::focusOutEvent(QFocusEvent* event) {
    QWidget::focusOutEvent(event);
    update();
}
