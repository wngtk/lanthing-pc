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

#if defined(LT_WINDOWS)
// TOO many windows headers inside
#include <wintoastlib.h>
#endif // LT_WINDOWS

#include <QtWidgets/qmenu.h>
#include <QtWidgets/qsystemtrayicon.h>
#include <QtWidgets/qwidget.h>
#include <qapplication.h>
#include <qfile.h>
#include <qobject.h>
#include <qtranslator.h>

#include <ltlib/logging.h>
#include <ltlib/strings.h>
#include <ltproto/service2app/accepted_connection.pb.h>

#include "friendly_error_code.h"
#include "mainwindow/mainwindow.h"

namespace {

#if defined(LT_WINDOWS)
class ToastHandler : public WinToastLib::IWinToastHandler {
public:
    ~ToastHandler() override {}
    void toastActivated() const override {}
    void toastActivated(int) const override {}
    void toastDismissed(WinToastDismissalReason) const override {}
    void toastFailed() const override {}
};
#endif // LT_WINDOWS

void ltQtOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    if (context.file == nullptr || context.function == nullptr) {
        std::string category;
        if (context.category) {
            category = context.category;
        }
        LOG(INFO) << "Qt logging, category: " << category
                  << ", message: " << msg.toStdString().c_str();
        return;
    }
    switch (type) {
    case QtDebugMsg:
        LOG_2(DEBUG, context.file, context.line, context.function) << msg.toStdString().c_str();
        break;
    case QtInfoMsg:
        LOG_2(INFO, context.file, context.line, context.function) << msg.toStdString().c_str();
        break;
    case QtWarningMsg:
        LOG_2(WARNING, context.file, context.line, context.function) << msg.toStdString().c_str();
        break;
    case QtCriticalMsg:
        LOG_2(ERR, context.file, context.line, context.function) << msg.toStdString().c_str();
        break;
    case QtFatalMsg:
        LOG_2(FATAL, context.file, context.line, context.function) << msg.toStdString().c_str();
    }
}

} // namespace

namespace lt {

class GUIImpl {
public:
    void init(const GUI::Params& params, int argc, char** argv);

    int exec();

    void setDeviceID(int64_t device_id);

    void setAccessToken(const std::string& token);

    void setLoginStatus(GUI::LoginStatus status);

    void onConfirmConnection(int64_t device_id);

    void onConnectionStatus(std::shared_ptr<google::protobuf::MessageLite> msg);

    void onAccptedConnection(std::shared_ptr<google::protobuf::MessageLite> msg);

    void onDisconnectedConnection(int64_t device_id);

    void onServiceStatus(GUI::ServiceStatus status);

    void errorMessageBox(const std::string& message);

    void infoMessageBox(const std::string& message);

    void errorCode(int32_t code);

    void onNewVersion(std::shared_ptr<google::protobuf::MessageLite> msg);

private:
    void setLanguage();

private:
    std::unique_ptr<QApplication> qapp_;
    std::unique_ptr<MainWindow> main_window_;
    QTranslator translator_;
    std::unique_ptr<QSystemTrayIcon> sys_tray_icon_;
    std::unique_ptr<QMenu> menu_;
};

void GUIImpl::init(const GUI::Params& params, int argc, char** argv) {
    qInstallMessageHandler(ltQtOutput);
    qapp_ = std::make_unique<QApplication>(argc, argv);
    setLanguage();
    QIcon icon(":/res/png_icons/pc2.png");
    QApplication::setWindowIcon(icon);
    QApplication::setApplicationName("Lanthing");
    QApplication::setQuitOnLastWindowClosed(false);

    main_window_ = std::make_unique<MainWindow>(params, nullptr);
    menu_ = std::make_unique<QMenu>(nullptr);
    sys_tray_icon_ = std::make_unique<QSystemTrayIcon>();
    sys_tray_icon_->setToolTip("Lanthing");
    QAction* a0 = new QAction(QObject::tr("Main Page"), menu_.get());
    QAction* a1 = new QAction(QObject::tr("Settings"), menu_.get());
    QAction* a2 = new QAction(QObject::tr("Exit"), menu_.get());
    QObject::connect(a0, &QAction::triggered, [this]() {
        main_window_->switchToMainPage();
        main_window_->show();
    });
    QObject::connect(a1, &QAction::triggered, [this]() {
        main_window_->switchToSettingPage();
        main_window_->show();
    });
    QObject::connect(a2, &QAction::triggered, []() { QApplication::exit(0); });
    QObject::connect(sys_tray_icon_.get(), &QSystemTrayIcon::activated,
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
    menu_->addAction(a0);
    menu_->addAction(a1);
    menu_->addAction(a2);
    sys_tray_icon_->setContextMenu(menu_.get());
    sys_tray_icon_->setIcon(icon);

    sys_tray_icon_->show();
    main_window_->show();

#if defined(LT_WINDOWS)
    // wintoast
    WinToastLib::WinToast::instance()->setAppName(L"Lanthing");
    WinToastLib::WinToast::instance()->setAppUserModelId(L"Lanthing");
    WinToastLib::WinToast::instance()->setShortcutPolicy(
        WinToastLib::WinToast::ShortcutPolicy::SHORTCUT_POLICY_IGNORE);
    if (!WinToastLib::WinToast::instance()->initialize()) {
        LOG(ERR) << "Initialize WinToastLib failed";
    }
#endif // LT_WINDOWS
}

int GUIImpl::exec() {
    int ret = qapp_->exec();
#if defined(LT_WINDOWS)
    WinToastLib::WinToast::instance()->clear();
#endif // LT_WINDOWS
    return ret;
}

void GUIImpl::setDeviceID(int64_t device_id) {
    main_window_->setDeviceID(device_id);
}

void GUIImpl::setAccessToken(const std::string& token) {
    main_window_->setAccessToken(token);
}

void GUIImpl::setLoginStatus(GUI::LoginStatus status) {
    main_window_->setLoginStatus(status);
}

void GUIImpl::onConfirmConnection(int64_t device_id) {
    main_window_->onConfirmConnection(device_id);
}

void GUIImpl::onConnectionStatus(std::shared_ptr<google::protobuf::MessageLite> msg) {
    main_window_->onConnectionStatus(msg);
}

void GUIImpl::onAccptedConnection(std::shared_ptr<google::protobuf::MessageLite> _msg) {
    auto msg = std::static_pointer_cast<ltproto::service2app::AcceptedConnection>(_msg);
    std::string device_id = std::to_string(msg->device_id());
    QString format = QObject::tr("%s connected to this machine");
    std::vector<char> buffer(128);
    snprintf(buffer.data(), buffer.size(), format.toStdString().c_str(), device_id.c_str());
    buffer.back() = '\0';
    std::wstring message = ltlib::utf8To16(buffer.data());

#if defined(LT_WINDOWS)
    WinToastLib::WinToastTemplate templ =
        WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text01);
    templ.setTextField(message, WinToastLib::WinToastTemplate::FirstLine);
    templ.setExpiration(1000 * 5);
    WinToastLib::WinToast::instance()->showToast(templ, new ToastHandler);
#endif // LT_WINDOWS

    main_window_->onAccptedConnection(_msg);
}

void GUIImpl::onDisconnectedConnection(int64_t device_id) {
    main_window_->onDisconnectedConnection(device_id);
}

void GUIImpl::onServiceStatus(GUI::ServiceStatus status) {
    main_window_->setServiceStatus(status);
}

void GUIImpl::errorMessageBox(const std::string& message) {
    main_window_->errorMessageBox(QString::fromStdString(message));
}

void GUIImpl::infoMessageBox(const std::string& message) {
    main_window_->infoMessageBox(QString::fromStdString(message));
}

void GUIImpl::errorCode(int32_t code) {
    QString error_msg = errorCode2FriendlyMessage(code);
    main_window_->errorMessageBox(error_msg);
}

void GUIImpl::onNewVersion(std::shared_ptr<google::protobuf::MessageLite> msg) {
    main_window_->onNewVersion(msg);
}

void GUIImpl::setLanguage() {
    QLocale locale;
    switch (locale.language()) {
    case QLocale::Chinese:
        if (translator_.load(":/i18n/lt-zh_CN")) {
            LOG(INFO) << "Language: Chinese";
            qapp_->installTranslator(&translator_);
            return;
        }
        else {
            LOG(WARNING) << "Locale setting is Chinese, but can't load translation file.";
        }
        break;
    default:
        break;
    }
    LOG(INFO) << "Language: English";
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

void GUI::setLoginStatus(LoginStatus status) {
    impl_->setLoginStatus(status);
}

void GUI::onConfirmConnection(int64_t device_id) {
    impl_->onConfirmConnection(device_id);
}

void GUI::onConnectionStatus(std::shared_ptr<google::protobuf::MessageLite> msg) {
    impl_->onConnectionStatus(msg);
}

void GUI::onAccptedConnection(std::shared_ptr<google::protobuf::MessageLite> msg) {
    impl_->onAccptedConnection(msg);
}

void GUI::onDisconnectedConnection(int64_t device_id) {
    impl_->onDisconnectedConnection(device_id);
}

void GUI::onServiceStatus(ServiceStatus status) {
    impl_->onServiceStatus(status);
}

void GUI::errorMessageBox(const std::string& message) {
    impl_->errorMessageBox(message);
}

void GUI::infoMessageBox(const std::string& message) {
    impl_->infoMessageBox(message);
}

void GUI::errorCode(int32_t code) {
    impl_->errorCode(code);
}

void GUI::onNewVersion(std::shared_ptr<google::protobuf::MessageLite> msg) {
    impl_->onNewVersion(msg);
}

} // namespace lt