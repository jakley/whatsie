#include "settingswidget.h"
#include "ui_settingswidget.h"

#include <QDateTime>
#include <QMessageBox>
#include "mainwindow.h"

#include "automatictheme.h"


extern QString defaultUserAgentStr;

SettingsWidget::SettingsWidget(QWidget *parent, QString engineCachePath, QString enginePersistentStoragePath) :
    QWidget(parent),
    ui(new Ui::SettingsWidget)
{
    ui->setupUi(this);

    this->engineCachePath             = engineCachePath;
    this->enginePersistentStoragePath = enginePersistentStoragePath;

    ui->zoomFactorSpinBox->setRange(0.25,5.0);
    ui->zoomFactorSpinBox->setValue(settings.value("zoomFactor",1.0).toDouble());
    //emit zoomChanged();

    ui->closeButtonActionComboBox->setCurrentIndex(settings.value("closeButtonActionCombo",0).toInt());
    ui->notificationCheckBox->setChecked(settings.value("disableNotificationPopups",false).toBool());
    ui->muteAudioCheckBox->setChecked(settings.value("muteAudio",false).toBool());
    ui->autoPlayMediaCheckBox->setChecked(settings.value("autoPlayMedia",false).toBool());
    ui->themeComboBox->setCurrentText(utils::toCamelCase(settings.value("windowTheme","light").toString()));
    ui->userAgentLineEdit->setText(settings.value("useragent",defaultUserAgentStr).toString());
    ui->enableSpellCheck->setChecked(settings.value("sc_enabled",true).toBool());
    ui->notificationTimeOutspinBox->setValue(settings.value("notificationTimeOut",9000).toInt()/1000);
    ui->notificationCombo->setCurrentIndex(settings.value("notificationCombo",1).toInt());
    ui->useNativeFileDialog->setChecked(settings.value("useNativeFileDialog",false).toBool());

    ui->automaticThemeCheckBox->blockSignals(true);
    bool automaticThemeSwitching = settings.value("automaticTheme",false).toBool();
    ui->automaticThemeCheckBox->setChecked(automaticThemeSwitching);
    ui->automaticThemeCheckBox->blockSignals(false);

    themeSwitchTimer = new QTimer(this);
    themeSwitchTimer->setInterval(60000); // 1 min
    connect(themeSwitchTimer,&QTimer::timeout,[=](){
        themeSwitchTimerTimeout();
    });

    //instantly call the timeout slot if automatic theme switching enabled
    if(automaticThemeSwitching)
        themeSwitchTimerTimeout();
    //start regular timer to update theme
    updateAutomaticTheme();

    this->setCurrentPasswordText("Current Password: <i>"
            +QByteArray::fromBase64(settings.value("asdfg").toString().toUtf8())+"</i>");

    applyThemeQuirks();

    ui->setUserAgent->setEnabled(false);
}

inline bool inRange(unsigned low, unsigned high, unsigned x)
{
    return  ((x-low) <= (high-low));
}

void SettingsWidget::themeSwitchTimerTimeout()
{
    if(settings.value("automaticTheme",false).toBool())
    {
        //start time
        QDateTime sunrise; sunrise.setSecsSinceEpoch(settings.value("sunrise").toLongLong());
        //end time
        QDateTime sunset;  sunset.setSecsSinceEpoch(settings.value("sunset").toLongLong());
        QDateTime currentTime = QDateTime::currentDateTime();

        int sunsetSeconds  = QTime(0,0).secsTo(sunset.time());
        int sunriseSeconds = QTime(0,0).secsTo(sunrise.time());
        int currentSeconds = QTime(0,0).secsTo(currentTime.time());

        if(inRange(sunsetSeconds,sunriseSeconds,currentSeconds))
        {
            qDebug()<<"is night: ";
            ui->themeComboBox->setCurrentText("Dark");
        }else{
            qDebug()<<"is morn: ";
            ui->themeComboBox->setCurrentText("Light");
        }
    }
}



void SettingsWidget::updateAutomaticTheme()
{
    bool automaticThemeSwitching = settings.value("automaticTheme",false).toBool();
    if(automaticThemeSwitching && !themeSwitchTimer->isActive()){
        themeSwitchTimer->start();
    }else if(!automaticThemeSwitching){
        themeSwitchTimer->stop();
    }
}

SettingsWidget::~SettingsWidget()
{
    delete ui;
}

void SettingsWidget::loadDictionaries(QStringList dictionaries)
{
       // set up supported spellcheck dictionaries
       QStringList ui_dictionary_names;
       foreach(QString dictionary_name, dictionaries) {
           ui_dictionary_names.append(dictionary_name);
       }

       ui_dictionary_names.removeDuplicates();
       ui_dictionary_names.sort();

       //add to ui
       ui->dictComboBox->blockSignals(true);
       foreach(const QString dict_name, ui_dictionary_names)
       {
           QString short_name = QString(dict_name).split("_").last();
           short_name = (short_name.isEmpty() || short_name.contains("-")) ? QString(dict_name).split("-").last() : short_name;
           short_name = short_name.isEmpty() ? "XX" : short_name;
           short_name = short_name.length() > 2  ? short_name.left(2) : short_name;
           QIcon icon(QString(":/icons/flags/%1.png").arg(short_name.toLower()));
           if(icon.isNull() == false)
            ui->dictComboBox->addItem(icon,dict_name);
           else
            ui->dictComboBox->addItem(QIcon(":/icons/flags/xx.png"),dict_name);
       }
       ui->dictComboBox->blockSignals(false);

       // load settings for spellcheck dictionary
       QString dictionary_name = settings.value("sc_dict","en-US").toString();
       int pos = ui->dictComboBox->findText(dictionary_name);
       if (pos == -1) {
          pos = ui->dictComboBox->findText("en-US");
          if (pos == -1) {
              pos = 0;
          }
       }
       ui->dictComboBox->setCurrentIndex(pos);
}

void SettingsWidget::refresh()
{
    ui->themeComboBox->setCurrentText(utils::toCamelCase(settings.value("windowTheme","light").toString()));

    ui->cacheSize->setText(utils::refreshCacheSize(cachePath()));
    ui->cookieSize->setText(utils::refreshCacheSize(persistentStoragePath()));

    //update dict settings at runtime
    // load settings for spellcheck dictionary
    QString dictionary_name = settings.value("sc_dict","en-US").toString();
    int pos = ui->dictComboBox->findText(dictionary_name);
    if (pos == -1) {
       pos = ui->dictComboBox->findText("en-US");
       if (pos == -1) {
           pos = 0;
       }
    }
    ui->dictComboBox->setCurrentIndex(pos);

    //enable disable spell check
    ui->enableSpellCheck->setChecked(settings.value("sc_enabled",true).toBool());


}

void SettingsWidget::updateDefaultUAButton(const QString engineUA)
{
    bool isDefault = QString::compare(engineUA,defaultUserAgentStr,Qt::CaseInsensitive) == 0;
    ui->defaultUserAgentButton->setEnabled(!isDefault);

    if(ui->userAgentLineEdit->text().trimmed().isEmpty()){
        ui->userAgentLineEdit->setText(engineUA);
    }
}


QString SettingsWidget::cachePath()
{
    return engineCachePath;
}

QString SettingsWidget::persistentStoragePath()
{
    return enginePersistentStoragePath;
}

void SettingsWidget::on_deleteCache_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("This will delete app cache! Application cache makes application load faster.");
          msgBox.setIconPixmap(QPixmap(":/icons/information-line.png").scaled(42,42,Qt::KeepAspectRatio,Qt::SmoothTransformation));

    msgBox.setInformativeText("Delete Application cache ?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    switch (ret) {
      case QMessageBox::Yes:{
        utils::delete_cache(this->cachePath());
        refresh();
        break;
        }
      case  QMessageBox::No:
        break;
    }
}

void SettingsWidget::on_deletePersistentData_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("This will delete app Persistent Data ! Persistent data includes persistent cookies, HTML5 local storage, and visited links.");
          msgBox.setIconPixmap(QPixmap(":/icons/information-line.png").scaled(42,42,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    msgBox.setInformativeText("Delete Application Cookies ?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    switch (ret) {
      case QMessageBox::Yes:{
        utils::delete_cache(this->persistentStoragePath());
        refresh();
        break;
        }
      case  QMessageBox::No:
        break;
    }
}

void SettingsWidget::on_notificationCheckBox_toggled(bool checked)
{
    settings.setValue("disableNotificationPopups",checked);
}

void SettingsWidget::on_themeComboBox_currentTextChanged(const QString &arg1)
{
    applyThemeQuirks();
    settings.setValue("windowTheme",QString(arg1).toLower());
    emit updateWindowTheme();
    emit updatePageTheme();
}


void SettingsWidget::applyThemeQuirks()
{
    //little quirks
    if(QString::compare(ui->themeComboBox->currentText(),"dark",Qt::CaseInsensitive)==0)
    {
        ui->bottomLine->setStyleSheet("background-color: rgb(5, 97, 98);");
        ui->label_7->setStyleSheet("color:#c2c5d1;padding: 0px 8px 0px 8px;background:transparent;");
    }else{
        ui->bottomLine->setStyleSheet("background-color: rgb(74, 223, 131);");
        ui->label_7->setStyleSheet("color:#1e1f21;padding: 0px 8px 0px 8px;background:transparent;");
    }
}


void SettingsWidget::on_muteAudioCheckBox_toggled(bool checked)
{
    settings.setValue("muteAudio",checked);
    emit muteToggled(checked);
}

void SettingsWidget::on_autoPlayMediaCheckBox_toggled(bool checked)
{
    settings.setValue("autoPlayMedia",checked);
    emit autoPlayMediaToggled(checked);
}

void SettingsWidget::on_defaultUserAgentButton_clicked()
{
    ui->userAgentLineEdit->setText(defaultUserAgentStr);
    emit userAgentChanged(ui->userAgentLineEdit->text());
}

void SettingsWidget::on_userAgentLineEdit_textChanged(const QString &arg1)
{
    bool isDefault = QString::compare(arg1.trimmed(),defaultUserAgentStr,Qt::CaseInsensitive) == 0;
    bool isPrevious= QString::compare(arg1.trimmed(),settings.value("useragent",defaultUserAgentStr).toString(),Qt::CaseInsensitive) == 0;

    if(isDefault == false && arg1.trimmed().isEmpty()==false)
    {
        ui->defaultUserAgentButton->setEnabled(false);
        ui->setUserAgent->setEnabled(false);
    }
    if(isPrevious == false && arg1.trimmed().isEmpty() == false)
    {
         ui->setUserAgent->setEnabled(true);
         ui->defaultUserAgentButton->setEnabled(true);
    }
    if(isPrevious){
        ui->defaultUserAgentButton->setEnabled(true);
        ui->setUserAgent->setEnabled(false);
    }
}

void SettingsWidget::on_setUserAgent_clicked()
{
    if(ui->userAgentLineEdit->text().trimmed().isEmpty()){
        QMessageBox::information(this,QApplication::applicationName()+"| Error",
                              "Cannot set an empty UserAgent String.");
        return;
    }
    emit userAgentChanged(ui->userAgentLineEdit->text());
}


void SettingsWidget::on_closeButtonActionComboBox_currentIndexChanged(int index)
{
    settings.setValue("closeButtonActionCombo",index);
}

void SettingsWidget::appLockSetChecked(bool checked)
{
    ui->applock_checkbox->setChecked(checked);
}

void SettingsWidget::setCurrentPasswordText(QString str)
{
    ui->current_password->setText(str);
}

void SettingsWidget::on_applock_checkbox_toggled(bool checked)
{
    if(settings.value("asdfg").isValid()){
        settings.setValue("lockscreen",checked);
    }else{
        settings.setValue("lockscreen",false);
    }
    if(checked){
        emit init_lock();
    }
}

void SettingsWidget::on_dictComboBox_currentIndexChanged(const QString &arg1)
{
    settings.setValue("sc_dict",arg1);
    emit dictChanged(arg1);
}

void SettingsWidget::on_enableSpellCheck_toggled(bool checked)
{
    settings.setValue("sc_enabled",checked);
    emit spellCheckChanged(checked);
}

void SettingsWidget::on_showShortcutsButton_clicked()
{
    QWidget *sheet = new QWidget(this);
    sheet->setWindowTitle(QApplication::applicationName()+" | Global shortcuts");

    sheet->setWindowFlags(Qt::Popup |Qt::FramelessWindowHint);
    sheet->move(this->geometry().center()-sheet->geometry().center());

    QVBoxLayout *layout = new QVBoxLayout(sheet);
    sheet->setLayout(layout);
    auto *w = qobject_cast<MainWindow*>(parent());
    if(w != 0){
        foreach (QAction *action, w->actions()) {
            QString shortcutStr = action->shortcut().toString();
            if(shortcutStr.isEmpty()==false){
                QLabel *label = new QLabel(action->text().remove("&")+"  |  "+shortcutStr,sheet);
                label->setAlignment(Qt::AlignHCenter);
                layout->addWidget(label);
            }
        }
    }
    sheet->setAttribute(Qt::WA_DeleteOnClose);
    sheet->show();
}

void SettingsWidget::on_showPermissionsButton_clicked()
{
    PermissionDialog *permissionDialog = new PermissionDialog(this);
    permissionDialog->setWindowTitle(QApplication::applicationName()+" | "+tr("Feature permissions"));
    permissionDialog->setWindowFlag(Qt::Dialog);
    permissionDialog->setAttribute(Qt::WA_DeleteOnClose,true);
    permissionDialog->move(this->geometry().center()-permissionDialog->geometry().center());
    permissionDialog->setMinimumSize(485,310);
    permissionDialog->adjustSize();
    permissionDialog->show();
}


void SettingsWidget::on_notificationTimeOutspinBox_valueChanged(int arg1)
{
    settings.setValue("notificationTimeOut",arg1*1000);
    emit notificationPopupTimeOutChanged();
}

void SettingsWidget::on_notificationCombo_currentIndexChanged(int index)
{
    settings.setValue("notificationCombo",index);
}

void SettingsWidget::on_tryNotification_clicked()
{
    emit notify("Test Notification");
}

void SettingsWidget::on_automaticThemeCheckBox_toggled(bool checked)
{
    if(checked)
    {
        AutomaticTheme *automaticTheme = new AutomaticTheme(this);
        automaticTheme->setWindowTitle(QApplication::applicationName()+" | Automatic theme switcher setup");
        automaticTheme->setWindowFlag(Qt::Dialog);
        automaticTheme->setAttribute(Qt::WA_DeleteOnClose,true);
        connect(automaticTheme,&AutomaticTheme::destroyed,[=](){
            bool automaticThemeSwitching = settings.value("automaticTheme",false).toBool();
            ui->automaticThemeCheckBox->setChecked(automaticThemeSwitching);
            if(automaticThemeSwitching)
                themeSwitchTimerTimeout();
            updateAutomaticTheme();
        });
        automaticTheme->show();
    }else{
       settings.setValue("automaticTheme",false);
       updateAutomaticTheme();
    }
}

void SettingsWidget::on_useNativeFileDialog_toggled(bool checked)
{
    settings.setValue("useNativeFileDialog",checked);
}

void SettingsWidget::on_zoomPlus_clicked()
{
    double currentFactor = settings.value("zoomFactor",1.0).toDouble();
    double newFactor = currentFactor + 0.25;
    ui->zoomFactorSpinBox->setValue(newFactor);

    settings.setValue("zoomFactor",ui->zoomFactorSpinBox->value());
    emit zoomChanged();
}

void SettingsWidget::on_zoomMinus_clicked()
{
    double currentFactor = settings.value("zoomFactor",1.0).toDouble();
    double newFactor = currentFactor - 0.25;
    ui->zoomFactorSpinBox->setValue(newFactor);

    settings.setValue("zoomFactor",ui->zoomFactorSpinBox->value());
    emit zoomChanged();
}

void SettingsWidget::on_zoomReset_clicked()
{
    double newFactor = 1.0;
    ui->zoomFactorSpinBox->setValue(newFactor);

    settings.setValue("zoomFactor",ui->zoomFactorSpinBox->value());
    emit zoomChanged();
}
