#include <QApplication>
#include "../UI/MainWindow.hpp"
#include "../Controller/AppController.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 1️⃣ Tạo MainWindow
    MainWindow mainWindow;

    // 2️⃣ Tạo AppController, truyền MainWindow
    AppController controller(&mainWindow);

    // 3️⃣ Hiển thị MainWindow
    mainWindow.show();

    // 4️⃣ Chạy ứng dụng
    return app.exec();
}
