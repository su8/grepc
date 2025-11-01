/*
 * 10/31/2025 https://github.com/su8/grepc
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <cctype>
#include <iostream>
#include <string>
#include <filesystem>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>

#include <QApplication>
#include <QCompleter>
#include <QStringList>
#include <QColor>
#include <QPalette>
#include <QAbstractItemView>
#include <QString>
#include <QTextEdit>
#include "mainwindow.h"
#include "./ui_mainwindow.h"

static void walkMultipleDirs(const char *folder, const char opt);

namespace fs = std::filesystem;
std::mutex mtx;
static std::unordered_map<std::string, uintmax_t> curDirNum;
static QString oldText = "";
static QStringList wordList = {"-m", "-b"};
static QCompleter *completer = new QCompleter(wordList, nullptr);

Ui::MainWindow *UI;

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    UI = ui;

    QPalette p = ui->textEdit->palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::green);
    ui->textEdit->setPalette(p);

    ui->lineEdit->setCompleter(completer);
    completer->popup()->setStyleSheet("background-color:rgb(54, 57, 63); color:white;");
    ui->lineEdit->setClearButtonEnabled(true);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::on_pushButton_clicked);
    connect(ui->lineEdit, &QLineEdit::textChanged, this, &MainWindow::setText);
}

MainWindow::~MainWindow()
{
    delete completer;
    //delete UI;
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    std::string userInput = static_cast<std::string>(ui->lineEdit->text().toStdString());
    if (userInput.empty()) { return; }

    unsigned short int y = 0U;
    char folders[256][4096] = {'\0'};
    char *allptr = userInput.data();
    char *token;
    std::vector<std::thread> threads;
    while ((token = strtok_r(allptr, " ", &allptr))) { if (!strcmp(token, "-m") || !strcmp(token, "-b")) { continue; } snprintf(folders[y++], 4096, "%s", token); }
    for (int x = 0; x < y; x++) { threads.emplace_back(walkMultipleDirs, folders[x], userInput.c_str()[1]); curDirNum.emplace(folders[x], 0U); }
    for (auto &thread : threads) { if (thread.joinable()) { thread.join(); } }

    QString inputStr = ui->lineEdit->text();
    ui->textEdit->setText(inputStr + static_cast<QString>('\n') + oldText);
    ui->lineEdit->setText(static_cast<QString>(""));
}

void MainWindow::setText()
{
    ui->textEdit->setText(oldText);
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEdit->setTextCursor(cursor);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

static void walkMultipleDirs(const char *folder, const char opt) {
    static QString byts = " bytes\n";
    try {
        for (const auto &entry : fs::directory_iterator(folder)) {
            std::lock_guard<std::mutex> lock(mtx);
            fs::current_path(folder);
            if (opt == 'b') {
                std::string fileStr = entry.path().filename().string();
                if (fs::exists(fileStr) && fs::is_directory(fileStr)) { continue; }
                //std::cout << fileStr << " " << fs::file_size(fileStr) << " bytes " << '\n' << std::flush;
                oldText += QString::fromStdString(fileStr) + static_cast<QString>(" ") + QString::number(fs::file_size(fileStr)) + byts;
            }
            curDirNum[folder]++;
        }
        fs::path curFolder = (folder[0] == '.') ? fs::current_path() : static_cast<fs::path>(folder);
        //std::cout << curFolder.string() << ' ' << curDirNum[folder] << " items" << '\n' << std::flush;
        QString countEm = static_cast<QString>(curFolder.string().c_str()) + static_cast<QString>(' ') + QString::number(curDirNum[folder]) + static_cast<QString>(" items") + static_cast<QString>('\n');
        oldText += countEm;
        //UI->textEdit->setText(countEm);
        curDirNum[folder] = 0U;
    } catch (const fs::filesystem_error &e) { std::lock_guard<std::mutex> lock(mtx); std::cerr << "Error: " << e.what() << std::endl; }
}
