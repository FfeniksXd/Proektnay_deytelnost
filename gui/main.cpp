#include <QApplication>
#include <QLabel>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    
    // Временная заглушка 
    QLabel label("TicTacToe skeleton works!");
    label.resize(400, 200);
    label.setAlignment(Qt::AlignCenter);
    label.show();
    
    return app.exec();
}