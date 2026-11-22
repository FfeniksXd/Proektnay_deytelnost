#include "gamewindow.h"
#include "gameboard.h"

#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <algorithm>

namespace {
constexpr int minSide = 3;
constexpr int maxSide = 20;

class Scanlines : public QWidget {
public:
    explicit Scanlines(QWidget* parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setPen(QColor(0, 0, 0, 54));
        for (int y = 2; y < height(); y += 3) {
            painter.drawLine(0, y, width(), y);
        }
    }
};

const char theme[] = R"(
    QWidget#terminal { background: #060e09; }
    QWidget { color: #99f5a9; font-family: "Consolas", "DejaVu Sans Mono", monospace; }
    QWidget#masthead { border-bottom: 1px solid #3f995b; }
    QWidget#settings { border-right: 1px solid #3f995b; }
    QLabel { font-size: 16px; }
    QLabel#title { font-size: 25px; font-weight: bold; }
    QLabel#sectionTitle, QLabel#status { font-size: 19px; font-weight: bold; }
    QLabel#hint { color: #7fc98d; font-size: 14px; }
    QLineEdit, QPushButton {
        min-height: 27px;
        padding: 7px 10px;
        background: #08150d;
        border: 1px solid #4ba66a;
        border-radius: 0;
        font-size: 16px;
    }
    QLineEdit { selection-background-color: #68e18b; selection-color: #060e09; }
    QPushButton { font-weight: bold; }
    QPushButton:hover, QLineEdit:focus, QPushButton:focus { border-color: #a1ffad; }
    QPushButton:hover, QPushButton:focus { background: #12301e; }
    QPushButton:pressed { background: #295b39; }
    QPushButton:disabled { color: #567b60; border-color: #334e3b; background: #09110c; }
    QRadioButton { spacing: 10px; font-size: 16px; padding: 3px 0; }
    QRadioButton::indicator {
        width: 14px;
        height: 14px;
        border: 1px solid #65cf83;
        border-radius: 0;
        background: #08150d;
    }
    QRadioButton::indicator:checked { background: #81ee99; }
    QRadioButton::indicator:hover, QRadioButton::indicator:focus { border-color: #c5ffd0; }
)";
}

GameWindow::GameWindow(QWidget* parent) : QWidget(parent) {
    setObjectName("terminal");
    setWindowTitle(tr("Крестики / нолики"));
    setStyleSheet(QString::fromLatin1(theme));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* masthead = new QWidget(this);
    masthead->setObjectName("masthead");
    auto* mastheadLayout = new QHBoxLayout(masthead);
    mastheadLayout->setContentsMargins(24, 14, 24, 14);
    auto* title = new QLabel(tr("КРЕСТИКИ / НОЛИКИ"), masthead);
    title->setObjectName("title");
    mastheadLayout->addWidget(title);
    mastheadLayout->addStretch();
    mastheadLayout->addWidget(new QLabel("X / O", masthead));
    layout->addWidget(masthead);

    auto* content = new QHBoxLayout;
    content->setSpacing(0);
    content->addWidget(createSettings());
    content->addWidget(createPlayArea(), 1);
    layout->addLayout(content, 1);

    scanlines_ = new Scanlines(this);
    scanlines_->setGeometry(rect());
    scanlines_->raise();
    updateSizePreview();
    resize(960, 660);
}

QWidget* GameWindow::createSettings() {
    auto* sidebar = new QWidget(this);
    sidebar->setObjectName("settings");
    sidebar->setFixedWidth(300);
    auto* layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(24, 22, 24, 24);
    layout->setSpacing(12);

    auto* heading = new QLabel(tr("НАСТРОЙКИ"), sidebar);
    heading->setObjectName("sectionTitle");
    layout->addWidget(heading);
    layout->addSpacing(6);
    layout->addWidget(new QLabel(tr("Размер поля"), sidebar));

    auto* dimensions = new QHBoxLayout;
    dimensions->setSpacing(16);
    auto addDimension = [&](const QString& title) {
        auto* column = new QVBoxLayout;
        column->setSpacing(5);
        auto* label = new QLabel(title, sidebar);
        auto* input = new QLineEdit("3", sidebar);
        input->setValidator(new QIntValidator(minSide, maxSide, input));
        input->setMaxLength(2);
        input->setToolTip(tr("От %1 до %2 клеток").arg(minSide).arg(maxSide));
        input->setAccessibleName(title);
        label->setBuddy(input);
        column->addWidget(label);
        column->addWidget(input);
        dimensions->addLayout(column);
        connect(input, &QLineEdit::textChanged, this, &GameWindow::updateSizePreview);
        return input;
    };
    widthInput_ = addDimension(tr("Ширина"));
    heightInput_ = addDimension(tr("Высота"));
    layout->addLayout(dimensions);
    winLengthLabel_ = new QLabel(sidebar);
    layout->addWidget(winLengthLabel_);

    layout->addSpacing(8);
    layout->addWidget(new QLabel(tr("Режим игры"), sidebar));
    auto* twoPlayers = new QRadioButton(tr("Два игрока"), sidebar);
    twoPlayers->setChecked(true);
    botMode_ = new QRadioButton(tr("С ботом"), sidebar);
    layout->addWidget(twoPlayers);
    layout->addWidget(botMode_);

    layout->addSpacing(12);
    newGameButton_ = new QPushButton(tr("НОВАЯ ИГРА"), sidebar);
    continueButton_ = new QPushButton(tr("ПРОДОЛЖИТЬ ИГРУ"), sidebar);
    setContinueAvailable(false);
    connect(newGameButton_, &QPushButton::clicked, this, &GameWindow::startNewGame);
    connect(continueButton_, &QPushButton::clicked, this, &GameWindow::continueRequested);
    layout->addWidget(newGameButton_);
    layout->addWidget(continueButton_);
    layout->addStretch();
    return sidebar;
}

QWidget* GameWindow::createPlayArea() {
    auto* playArea = new QWidget(this);
    auto* layout = new QVBoxLayout(playArea);
    layout->setContentsMargins(26, 22, 26, 20);
    layout->setSpacing(14);

    statusLabel_ = new QLabel(tr("ПОЛЕ ГОТОВО"), playArea);
    statusLabel_->setObjectName("status");
    layout->addWidget(statusLabel_);
    board_ = new GameBoard(playArea);
    connect(board_, &GameBoard::cellActivated, this, &GameWindow::cellActivated);
    layout->addWidget(board_, 1);
    auto* hint = new QLabel(tr("↑ ↓ ← →  Выбор клетки     Enter  Ход"), playArea);
    hint->setObjectName("hint");
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(hint);
    return playArea;
}

bool GameWindow::hasValidSize() const {
    return widthInput_->hasAcceptableInput() && heightInput_->hasAcceptableInput();
}

int GameWindow::winLength() const {
    // Предварительное правило для интерфейса, пока не подключена игровая сессия.
    const int shorterSide = std::min(widthInput_->text().toInt(), heightInput_->text().toInt());
    return std::clamp(shorterSide - 1, 3, 5);
}

void GameWindow::updateSizePreview() {
    const bool valid = hasValidSize();
    winLengthLabel_->setText(tr("В ряд для победы: %1")
                                 .arg(valid ? QString::number(winLength()) : QStringLiteral("—")));
    newGameButton_->setEnabled(valid);
}

void GameWindow::startNewGame() {
    if (!hasValidSize()) {
        return;
    }
    const int width = widthInput_->text().toInt();
    const int height = heightInput_->text().toInt();
    board_->reset(width, height);
    showStatus(tr("ПОЛЕ ГОТОВО"));
    board_->setFocus();
    emit newGameRequested(width, height, winLength(), botMode_->isChecked());
}

void GameWindow::showCell(int row, int column, Cell value) {
    board_->setCell(row, column, value);
}

void GameWindow::showStatus(const QString& status) {
    statusLabel_->setText(status);
}

void GameWindow::setContinueAvailable(bool available) {
    continueButton_->setEnabled(available);
    continueButton_->setToolTip(available ? QString() : tr("Нет сохранённой партии"));
}

void GameWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (scanlines_) {
        scanlines_->setGeometry(rect());
    }
}
