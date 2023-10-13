/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2023 Zhennan Tu <zhennan.tu@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gui.h"

#include <QtWidgets/qmenu.h>
#include <QtWidgets/qsystemtrayicon.h>
#include <QtWidgets/qwidget.h>
#include <qapplication.h>
#include <qobject.h>
#include <qtranslator.h>

#include <ltlib/logging.h>

#include "mainwindow/mainwindow.h"

namespace lt {

class GUIImpl {
public:
    void init(const GUI::Params& params, int argc, char** argv);

    int exec();

    void setDeviceID(int64_t device_id);

    void setAccessToken(const std::string& token);

    void setLoginStatus(GUI::ErrCode err_code);

    void handleConfirmConnection(int64_t device_id);

private:
    void setLanguage();

private:
    std::unique_ptr<QApplication> qapp_;
    std::unique_ptr<MainWindow> main_window_;
    QTranslator translator_;
};

void GUIImpl::init(const GUI::Params& params, int argc, char** argv) {
    qapp_ = std::make_unique<QApplication>(argc, argv);
    setLanguage();
    QIcon icon(":/icons/icons/pc.png");
    QApplication::setWindowIcon(icon);
    QApplication::setQuitOnLastWindowClosed(false);

    main_window_ = std::make_unique<MainWindow>(params, nullptr);

    QSystemTrayIcon sys_tray_icon;
    QMenu* menu = new QMenu();
    QAction* a0 = new QAction(QObject::tr("Main Page"));
    QAction* a1 = new QAction(QObject::tr("Settings"));
    QAction* a2 = new QAction(QObject::tr("Exit"));
    QObject::connect(a0, &QAction::triggered, [this]() {
        main_window_->switchToMainPage();
        main_window_->show();
    });
    QObject::connect(a1, &QAction::triggered, [this]() {
        main_window_->switchToSettingPage();
        main_window_->show();
    });
    QObject::connect(a2, &QAction::triggered, []() { QApplication::exit(0); });
    QObject::connect(&sys_tray_icon, &QSystemTrayIcon::activated,
                     [this](QSystemTrayIcon::ActivationReason reason) {
                         switch (reason) {
                         case QSystemTrayIcon::Unknown:
                             break;
                         case QSystemTrayIcon::Context:
                             break;
                         case QSystemTrayIcon::DoubleClick:
                             main_window_->show();
                             break;
                         case QSystemTrayIcon::Trigger:
                             main_window_->show();
                             break;
                         case QSystemTrayIcon::MiddleClick:
                             break;
                         default:
                             break;
                         }
                     });
    menu->addAction(a0);
    menu->addAction(a1);
    menu->addAction(a2);
    sys_tray_icon.setContextMenu(menu);
    sys_tray_icon.setIcon(icon);

    sys_tray_icon.show();
    main_window_->show();
}

int GUIImpl::exec() {
    return qapp_->exec();
}

void GUIImpl::setDeviceID(int64_t device_id) {
    main_window_->setDeviceID(device_id);
}

void GUIImpl::setAccessToken(const std::string& token) {
    main_window_->setAccessToken(token);
}

void GUIImpl::setLoginStatus(GUI::ErrCode err_code) {
    main_window_->setLoginStatus(err_code);
}

void GUIImpl::handleConfirmConnection(int64_t device_id) {
    main_window_->handleConfirmConnection(device_id);
}

void GUIImpl::setLanguage() {
    QLocale locale;
    switch (locale.language()) {
    case QLocale::Chinese:
        if (translator_.load(":/i18n/lt-zh_CN")) {
            LOG(INFO) << "Language Chinese";
            qapp_->installTranslator(&translator_);
        }
        return;
    default:
        break;
    }
    LOG(INFO) << "Language English";
}

GUI::GUI()
    : impl_{std::make_shared<GUIImpl>()} {}

void GUI::init(const Params& params, int argc, char** argv) {
    impl_->init(params, argc, argv);
}

int GUI::exec() {
    return impl_->exec();
}

void GUI::setDeviceID(int64_t device_id) {
    impl_->setDeviceID(device_id);
}

void GUI::setAccessToken(const std::string& token) {
    impl_->setAccessToken(token);
}

void GUI::setLoginStatus(ErrCode err_code) {
    impl_->setLoginStatus(err_code);
}

void GUI::handleConfirmConnection(int64_t device_id) {
    impl_->handleConfirmConnection(device_id);
}

} // namespace lt