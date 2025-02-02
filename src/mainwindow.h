#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QIcon>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QRegExp>
#include <QSettings>
#include <QStatusBar>
#include <QStyle>
#include <QStyleFactory>
#include <QSystemTrayIcon>
#include <QWebEngineCookieStore>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QWebEngineSettings>

#include <QRadioButton>
#include <QWebEngineContextMenuData>

#include "notificationpopup.h"
#include "requestinterceptor.h"
#include "settingswidget.h"
#include "webenginepage.h"
#include "lock.h"

#include "downloadmanagerwidget.h"
#include "about.h"
#include "dictionaries.h"
#include "webview.h"
#include "rateapp.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
public slots:
    void updateWindowTheme();
    void updatePageTheme();

    void handleWebViewTitleChanged(QString title);
    void handleLoadFinished(bool loaded);
    void handleDownloadRequested(QWebEngineDownloadItem *download);
    void loadAppWithArgument(const QString &arg);


protected slots:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event);
private:
    QPalette lightPalette;
    void createActions();
    void createTrayIcon();
    void createWebEngine();

    QSettings settings;

    QRegExp notificationsTitleRegExp;
    QIcon trayIconRead;
    QIcon trayIconUnread;

    QAction *reloadAction;
    QAction *minimizeAction;
    QAction *restoreAction;
    QAction *aboutAction;
    QAction *settingsAction;
    QAction *quitAction;
    QAction *lockAction;
    QAction *fullscreenAction;
    QAction *openUrlAction;

    QMenu *trayIconMenu;
    QSystemTrayIcon *trayIcon;

    QWebEngineView *webEngine;
    //QStatusBar *statusBar;


    SettingsWidget * settingsWidget = nullptr;

    //void reload();

    DownloadManagerWidget m_downloadManagerWidget;
    QScopedPointer<QWebEngineProfile> m_otrProfile;

    Lock *lockWidget = nullptr;

    int correctlyLoaderRetries = 4;

    QStringList m_dictionaries;

private slots:

    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void messageClicked();
    void doReload();
    void showAbout();
    void notify(QString title, QString message);
    void showSettings();
    void handleCookieAdded(const QNetworkCookie &cookie);

    QString getPageTheme();
    void toggleMute(const bool &checked);
    void doAppReload();
    void askToReloadPage();
    void updateSettingsUserAgentWidget();
    void fullScreenRequested(QWebEngineFullScreenRequest request);

    void createWebPage(bool offTheRecord =false);
    void init_settingWidget();
    void init_globalWebProfile();
    void check_window_state();
    void init_lock();
    void lockApp();


    void checkLoadedCorrectly();
    void loadingQuirk(QString test);
    void setNotificationPresenter(QWebEngineProfile *profile);
    void newChat();
    bool isPhoneNumber(const QString &phoneNumber);
    void quitApp();
};

#endif // MAINWINDOW_H
