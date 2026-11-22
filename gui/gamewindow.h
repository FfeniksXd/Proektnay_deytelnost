#pragma once

#include "Board.h"

#include <QWidget>

class GameBoard;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;

class GameWindow : public QWidget {
    Q_OBJECT

public:
    explicit GameWindow(QWidget* parent = nullptr);

    void showCell(int row, int column, Cell value);
    void showStatus(const QString& status);
    void setContinueAvailable(bool available);

signals:
    void newGameRequested(int width, int height, int winLength, bool againstBot);
    void continueRequested();
    void cellActivated(int row, int column);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QWidget* createSettings();
    QWidget* createPlayArea();
    bool hasValidSize() const;
    int winLength() const;
    void updateSizePreview();
    void startNewGame();

    QLineEdit* widthInput_ = nullptr;
    QLineEdit* heightInput_ = nullptr;
    QLabel* winLengthLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QRadioButton* botMode_ = nullptr;
    QPushButton* newGameButton_ = nullptr;
    QPushButton* continueButton_ = nullptr;
    GameBoard* board_ = nullptr;
    QWidget* scanlines_ = nullptr;
};
