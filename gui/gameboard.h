#pragma once

#include "Board.h"

#include <QWidget>
#include <vector>

class GameBoard : public QWidget {
    Q_OBJECT

public:
    explicit GameBoard(QWidget* parent = nullptr);

    void reset(int width, int height);
    void setCell(int row, int column, Cell value);

signals:
    void cellActivated(int row, int column);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    QRect gridRect() const;
    bool contains(int row, int column) const;
    void activateSelection();

    int columns_ = 3;
    int rows_ = 3;
    int selectedRow_ = 0;
    int selectedColumn_ = 0;
    std::vector<Cell> cells_;
};
