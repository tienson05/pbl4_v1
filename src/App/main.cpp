#include <QApplication>
#include "../UI/MainWindow.hpp"
#include "../Controller/AppController.hpp""
#include "../Common/MacResolver.hpp"

int main(int argc, char *argv[])

{
    QApplication app(argc, argv);
    MacResolver::instance().loadDatabase("/home/toan/Qt/Project/pbl4_v1/src/App/manuf");

    // 1️.Tạo MainWindow
    MainWindow mainWindow;

    // 2️. Tạo AppController, truyền MainWindow
    AppController controller(&mainWindow);
    // 3️. Hiển thị MainWindow
    mainWindow.show();

    // 4️. Chạy ứng dụng
    return app.exec();
}
