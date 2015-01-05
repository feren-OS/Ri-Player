#include "bakaengine.h"

#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"
#include "mpvhandler.h"
#include "util.h"

#include "ui/aboutdialog.h"

#include <QMessageBox>

BakaEngine::BakaEngine(QObject *parent):
    QObject(parent),
    CommandMap({
        {"about", &BakaEngine::BakaAbout},
        {"about_qt", &BakaEngine::BakaAboutQt},
        {"quit", &BakaEngine::BakaQuit}
    })
{
    window = static_cast<MainWindow*>(parent);
    mpv = new MpvHandler(window->ui->mpvFrame->winId(), this);
    settings = Util::InitializeSettings(this);

    connect(mpv, SIGNAL(messageSignal(QString)),
            this, SLOT(MpvPrint(QString)));
}

BakaEngine::~BakaEngine()
{
    delete mpv;
    delete settings;
}

void BakaEngine::LoadSettings()
{
    if(settings == nullptr)
        return;

    settings->Load();

    QString version;
    if(settings->isEmpty()) // empty settings
    {
        version = "2.0.2"; // current version

        // populate initially
#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
        settings->beginGroup("mpv");
        settings->setValueQStringList("vo", QStringList{"vdpau","opengl-hq"});
        settings->setValue("hwdec", "auto");
        settings->endGroup();
#endif
    }
    else
    {
        settings->beginGroup("baka-mplayer");
        version = settings->value("version", "1.9.9"); // defaults to the first version without version info in settings
        settings->endGroup();
    }

    if(version == "2.0.2") Load2_0_2();
    else if(version == "2.0.1") { Load2_0_1(); settings->clear(); SaveSettings(); }
    else if(version == "2.0.0") { Load2_0_0(); settings->clear(); SaveSettings(); }
    else if(version == "1.9.9") { Load1_9_9(); settings->clear(); SaveSettings(); }
    else
    {
        Load2_0_2();
        window->ui->action_Preferences->setEnabled(false);
        QMessageBox::information(window, tr("Settings version not recognized"), tr("The settings file was made by a newer version of baka-mplayer; please upgrade this version or seek assistance from the developers.\nSome features may not work and changed settings will not be saved."));
        delete settings;
        settings = nullptr;
    }
}

void BakaEngine::SaveSettings()
{
    if(settings == nullptr)
        return;

    settings->beginGroup("baka-mplayer");
    settings->setValue("onTop", window->onTop);
    settings->setValueInt("autoFit", window->autoFit);
    settings->setValueBool("trayIcon", window->sysTrayIcon->isVisible());
    settings->setValueBool("hidePopup", window->hidePopup);
    settings->setValueBool("remaining", window->remaining);
    settings->setValueInt("splitter", (window->ui->splitter->position() == 0 ||
                                    window->ui->splitter->position() == window->ui->splitter->max()) ?
                                    window->ui->splitter->normalPosition() :
                                    window->ui->splitter->position());
    settings->setValueBool("showAll", !window->ui->hideFilesButton->isChecked());
    settings->setValueBool("screenshotDialog", window->screenshotDialog);
    settings->setValueBool("debug", window->debug);
    settings->setValueQStringList("recent", window->recent);
    settings->setValueInt("maxRecent", window->maxRecent);
    settings->setValue("lang", window->lang);
    settings->setValueBool("gestures", window->gestures);
    settings->setValue("version", "2.0.2");
    settings->endGroup();

    settings->beginGroup("mpv");
    settings->setValueInt("volume", mpv->volume);
    settings->setValueDouble("speed", mpv->speed);
    if(mpv->screenshotFormat != "")
        settings->setValue("screenshot-format", mpv->screenshotFormat);
    if(mpv->screenshotTemplate != "")
        settings->setValue("screenshot-template", mpv->screenshotDir+"/"+mpv->screenshotTemplate);
    settings->endGroup();

    settings->Save();
}

void BakaEngine::Command(QString command)
{
    if(command == QString())
        return;

    QStringList cmdList = command.split(" ");
    if(cmdList[0] == "baka")
    {
        cmdList.pop_front();
        BakaCommand(cmdList);
    }
    else if(cmdList[0] == "mpv")
    {
        cmdList.pop_front();
        mpv->CommandString(cmdList.join(" "));
    }
    else
        InvalidCommand(command);
}

void BakaEngine::BakaCommand(QStringList cmdList)
{
    if(cmdList.length() == 1)
    {
        auto iter = CommandMap.find(cmdList[0]);
        if(iter != CommandMap.end())
            (this->*(*iter))(); // execute command
        else
            InvalidCommand(cmdList[0]);
    }
    else
        InvalidCommand(cmdList.join(' '));
}

void BakaEngine::BakaPrint(QString output)
{
    window->ui->outputTextEdit->moveCursor(QTextCursor::End);
    window->ui->outputTextEdit->insertPlainText(QString("[baka]: %0").arg(output));
}

void BakaEngine::MpvPrint(QString output)
{
    window->ui->outputTextEdit->moveCursor(QTextCursor::End);
    window->ui->outputTextEdit->insertPlainText(QString("[mpv]: %0").arg(output));
}

void BakaEngine::InvalidCommand(QString command)
{
    BakaPrint(tr("invalid command '%0'\n").arg(command));
}

void BakaEngine::InvalidParameter(QString parameter)
{
    BakaPrint(tr("invalid parameter '%0'\n").arg(parameter));
}

void BakaEngine::BakaAbout()
{
    AboutDialog::about(BAKA_MPLAYER_VERSION, window);
}

void BakaEngine::BakaAboutQt()
{
    qApp->aboutQt();
}

void BakaEngine::BakaQuit()
{
    qApp->quit();
}
