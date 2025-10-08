#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QDebug>
#include "include/navigation_main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Automotive Navigation System");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Navigation Systems Ltd");
    
    // Set modern style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Set dark theme palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    
    // Create and show main window
    nav::NavigationMainWindow window;
    window.show();
    
    qDebug() << "Automotive Navigation System started";
    qDebug() << "Qt version:" << QT_VERSION_STR;
    qDebug() << "Available styles:" << QStyleFactory::keys();
    
    return app.exec();
}